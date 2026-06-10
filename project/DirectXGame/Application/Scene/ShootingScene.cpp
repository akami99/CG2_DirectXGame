#include "ShootingScene.h"
#include "Model.h"
#include "DX12Context.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "ParticleManager.h"
#include "BlendMode.h"
#include "Input.h"
#include "Win32Window.h"
#include "MathUtils.h"
#include <cmath>
#include <numbers>
#include "MyGame.h"

using namespace MathUtils;
using namespace BlendMode;

ShootingScene::ShootingScene() = default;
ShootingScene::~ShootingScene() = default;

void ShootingScene::Initialize() {
    // レベルデータのロード
    levelData_ = LevelLoader::LoadFile("testScene");

    // カメラの初期化
    camera_ = std::make_unique<Camera>();
    camera_->Initialize();
    if (levelData_ && !levelData_->players.empty()) {
        const auto& spawn = levelData_->players[0];
        camera_->SetTranslate(spawn.translation);
        camera_->SetRotate(spawn.rotation);
        cameraYaw_ = spawn.rotation.y;
    } else {
        camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
        camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
        cameraYaw_ = 0.0f;
    }

    Object3dCommon::GetInstance()->SetDefaultCamera(camera_.get());

    // サイドカメラの初期化
    leftCamera_ = std::make_unique<Camera>();
    leftCamera_->Initialize();
    rightCamera_ = std::make_unique<Camera>();
    rightCamera_->Initialize();

    // テクスチャの読み込み
    TextureManager::GetInstance()->LoadTexture(crosshairPath_);

    // モデル読み込み
    ModelManager::GetInstance()->LoadModel("sphere", sphereModel_);

    // スカイボックスの初期化
    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize("skybox.dds");
    skybox_->SetCamera(camera_.get());

    Object3dCommon::GetInstance()->SetEnvironmentMap(TextureManager::GetInstance()->GetSrvIndex("skybox.dds"));

    // --- 3Dオブジェクト生成 ---
    target_ = std::make_unique<Object3d>();
    target_->Initialize();
    target_->SetModel(sphereModel_);
    target_->SetCamera(camera_.get());

    // レベルデータから敵キャラを生成・配置
    if (levelData_) {
        for (const auto& enemyData : levelData_->enemies) {
            auto enemy = std::make_unique<Object3d>();
            enemy->Initialize();
            enemy->SetModel(sphereModel_);
            enemy->SetTranslate(enemyData.translation);
            enemy->SetRotation(enemyData.rotation);
            enemy->SetCamera(camera_.get());
            levelEnemies_.push_back(std::move(enemy));
        }
    }
    
    // プリミティブ平面モデルの動的生成
    hitEffectPlaneModel_ = std::make_unique<Model>();
    PlaneSettings planeSettings;
    planeSettings.size = { 3.0f, 3.0f };
    planeSettings.divisionX = 1;
    planeSettings.divisionY = 1;
    planeSettings.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    hitEffectPlaneModel_->CreatePlane(crosshairPath_, planeSettings);

    hitEffectPlane_ = std::make_unique<Object3d>();
    hitEffectPlane_->Initialize();
    hitEffectPlane_->SetModel(hitEffectPlaneModel_.get());
    hitEffectPlane_->SetCamera(camera_.get());

    // --- スプライト生成 ---
    crosshair_ = std::make_unique<Sprite>();
    crosshair_->Initialize(crosshairPath_);
    crosshair_->SetAnchorPoint({ 0.5f, 0.5f });
    crosshair_->SetScale({ 64.0f, 64.0f });

    // --- オフスクリーン用リソース ---
    leftRT_ = std::make_unique<RenderTexture>();
    leftRT_->Initialize(320, 180);
    rightRT_ = std::make_unique<RenderTexture>();
    rightRT_->Initialize(320, 180);

    leftSideSprite_ = std::make_unique<Sprite>();
    leftSideSprite_->Initialize(crosshairPath_);
    leftSideSprite_->SetTexture(leftRT_->GetSrvIndex());
    leftSideSprite_->SetTranslate({ 0.0f, 0.0f });
    leftSideSprite_->SetScale({ 320.0f, 180.0f });

    rightSideSprite_ = std::make_unique<Sprite>();
    rightSideSprite_->Initialize(crosshairPath_);
    rightSideSprite_->SetTexture(rightRT_->GetSrvIndex());
    rightSideSprite_->SetTranslate({ Win32Window::kClientWidth - 320.0f, 0.0f });
    rightSideSprite_->SetScale({ 320.0f, 180.0f });

    // 初期フェーズの設定
    phase_ = Phase::Playing;
    hitCount_ = 0;
}

void ShootingScene::Update() {
    // 入力の更新は常に行う
    Input::GetInstance()->Update();

#ifdef USE_IMGUI
    // UI処理 (ImGuiの定義)
    UpdateImGui();
#endif // USE_IMGUI

    const float kHalfPI = std::numbers::pi_v<float> / 2.0f;

    // =====================================================
    // フェーズ: ゲームオーバービネット（更新停止・ビネット強化）
    // =====================================================
    if (phase_ == Phase::GameOverVignette) {
        phaseTimer_ += kDeltaTime;
        float t = phaseTimer_ / kVignetteInDuration; // 0.0 → 1.0
        if (t > 1.0f) t = 1.0f;

        // ビネットを徐々に強化: scale 16→0.5, exponent 0.8→6.0
        vignetteScale_ = 16.0f + (0.5f - 16.0f) * t;
        vignetteExponent_ = 0.8f + (6.0f - 0.8f) * t;
        PostProcessManager::SetMode(PostProcessManager::kModeVignette);
        PostProcessManager::SetVignetteParams(vignetteScale_, vignetteExponent_);

        if (phaseTimer_ >= kVignetteInDuration) {
            // 暗転維持フェーズへ
            phase_ = Phase::GameOverWait;
            phaseTimer_ = 0.0f;
        }

        // 2Dスプライトの更新のみ実施（ゲームのメイン更新はスキップして一時停止）
        leftSideSprite_->Update();
        rightSideSprite_->Update();
        return;
    }

    // =====================================================
    // フェーズ: ゲームオーバー暗転維持（3秒間停止）
    // =====================================================
    if (phase_ == Phase::GameOverWait) {
        phaseTimer_ += kDeltaTime;

        if (phaseTimer_ >= kGameOverWaitDuration) {
            // --- シーンを初期化（論理リセット） ---
            hitCount_ = 0;
            orbitAngle_ = 0.0f;
            targetTimer_ = 0.0f;
            cameraYaw_ = 0.0f;
            hitFeedbackTimer_ = 0.0f;
            isHit_ = false;
            damageEffectStrength_ = 0.0f;
            projectileSpawnTimer_ = 0.0f;
            projectiles_.clear();
            levelEnemies_.clear();
            if (levelData_) {
                for (const auto& enemyData : levelData_->enemies) {
                    auto enemy = std::make_unique<Object3d>();
                    enemy->Initialize();
                    enemy->SetModel(sphereModel_);
                    enemy->SetTranslate(enemyData.translation);
                    enemy->SetRotation(enemyData.rotation);
                    enemy->SetCamera(camera_.get());
                    levelEnemies_.push_back(std::move(enemy));
                }
            }

            // カメラを初期位置に戻す
            if (levelData_ && !levelData_->players.empty()) {
                const auto& spawn = levelData_->players[0];
                camera_->SetTranslate(spawn.translation);
                camera_->SetRotate(spawn.rotation);
                cameraYaw_ = spawn.rotation.y;
            } else {
                camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
                camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
                cameraYaw_ = 0.0f;
            }

            // スムージングを最大限（31）適用してリスタート
            smoothingKernel_ = 31.0f;
            PostProcessManager::SetMode(PostProcessManager::kModeSmoothing);
            PostProcessManager::SetSmoothingParams(static_cast<int>(smoothingKernel_));

            phase_ = Phase::RestartSmoothing;
            phaseTimer_ = 0.0f;
        }

        leftSideSprite_->Update();
        rightSideSprite_->Update();
        return;
    }

    // =====================================================
    // フェーズ: リスタートスムージングフェードアウト
    // =====================================================
    if (phase_ == Phase::RestartSmoothing) {
        phaseTimer_ += kDeltaTime;
        float t = phaseTimer_ / kSmoothingOutDuration; // 0.0 → 1.0
        if (t > 1.0f) t = 1.0f;

        // カーネルサイズを 31 → 1 に線形補間
        smoothingKernel_ = 31.0f + (1.0f - 31.0f) * t;
        int kernel = static_cast<int>(smoothingKernel_);
        if (kernel % 2 == 0) kernel++; // ぼかしフィルタの仕様上、奇数に補正
        if (kernel < 1) kernel = 1;

        PostProcessManager::SetMode(PostProcessManager::kModeSmoothing);
        PostProcessManager::SetSmoothingParams(kernel);

        if (phaseTimer_ >= kSmoothingOutDuration) {
            // 通常画面になったら完全に通常コピーモードへ戻す
            PostProcessManager::SetMode(PostProcessManager::kModeCopy);
            phase_ = Phase::Playing;
        }
        // スムージング中もゲームは再開しているため、このまま下の通常ロジックを走らせる
    }

    // =====================================================
    // 通常プレイ時のゲームロジック (Playing / RestartSmoothing共通)
    // =====================================================

	// W/Sキーでカメラ前後移動
	Vector3 camPos = camera_->GetTranslate();
    if (Input::GetInstance()->IsKeyDown(DIK_W)) {
		camPos.x += std::sin(cameraYaw_) * 0.1f;
		camPos.z += std::cos(cameraYaw_) * 0.1f;
    }
    if (Input::GetInstance()->IsKeyDown(DIK_S)) {
		camPos.x -= std::sin(cameraYaw_) * 0.1f;
		camPos.z -= std::cos(cameraYaw_) * 0.1f;
    }
	// カメラの位置を更新
	camera_->SetTranslate(camPos);

    // A/Dキーでカメラ回転
    if (Input::GetInstance()->IsKeyDown(DIK_A)) {
        cameraYaw_ -= kHalfPI / 100.0f;
    }
    if (Input::GetInstance()->IsKeyDown(DIK_D)) {
        cameraYaw_ += kHalfPI / 100.0f;
    }
	Vector3 currentRot = camera_->GetRotate();
    camera_->SetRotate({ currentRot.x, cameraYaw_, currentRot.z });

    // ターゲットの移動
    orbitAngle_ += 0.005f;
    targetTimer_ += 0.05f;

    Vector3 pos;
    pos.x = camPos.x + std::sin(orbitAngle_) * orbitRadius_;
    pos.z = camPos.z + std::cos(orbitAngle_) * orbitRadius_;
    pos.y = targetBasePos_.y + std::sin(targetTimer_) * 2.0f;
    target_->SetTranslate(pos);

    // ヒットフィードバック
    if (hitFeedbackTimer_ > 0) {
        hitFeedbackTimer_ -= kDeltaTime;
        
        // Planeの回転角を更新（Y軸/Z軸で回転）
        Vector3 rot = hitEffectPlane_->GetRotation();
        rot.y += 0.1f;
        rot.z += 0.1f;
        hitEffectPlane_->SetRotation(rot);
        hitEffectPlane_->SetTranslate(target_->GetTranslate());
    }
    target_->SetScale({ 1.0f, 1.0f, 1.0f });

    // オブジェクトの更新 (マルチビュー対応)
    target_->Update(mainViewIndex_, camera_.get());
    target_->Update(leftViewIndex_, leftCamera_.get());
    target_->Update(rightViewIndex_, rightCamera_.get());

    for (auto& enemy : levelEnemies_) {
        enemy->Update(mainViewIndex_, camera_.get());
        enemy->Update(leftViewIndex_, leftCamera_.get());
        enemy->Update(rightViewIndex_, rightCamera_.get());
    }
    
    if (hitFeedbackTimer_ > 0) {
        hitEffectPlane_->Update(mainViewIndex_, camera_.get());
        hitEffectPlane_->Update(leftViewIndex_, leftCamera_.get());
        hitEffectPlane_->Update(rightViewIndex_, rightCamera_.get());
    }

    skybox_->Update(mainViewIndex_, camera_.get());
    skybox_->Update(leftViewIndex_, leftCamera_.get());
    skybox_->Update(rightViewIndex_, rightCamera_.get());

    // --- プロジェクタイルの生成 ---
    projectileSpawnTimer_ += 1.0f;
    if (projectileSpawnTimer_ >= kProjectileSpawnInterval) {
        projectileSpawnTimer_ = 0.0f;
        Vector3 startPos = target_->GetTranslate();
        Vector3 targetPos = camera_->GetTranslate();
        Vector3 dir = Normalize(Subtract(targetPos, startPos));
        float speed = 0.1f;
        Vector3 velocity = Multiply(speed, dir);
        auto newProjectile = std::make_unique<EnemyProjectile>();
        newProjectile->Initialize(startPos, velocity);
        projectiles_.push_back(std::move(newProjectile));
    }

    // プロジェクタイルの移動・行列更新
    for (auto& p : projectiles_) {
        p->Update();
        p->Update(mainViewIndex_, camera_.get());
        p->Update(leftViewIndex_, leftCamera_.get());
        p->Update(rightViewIndex_, rightCamera_.get());
    }

    // カメラとの当たり判定（プレイヤー被弾処理）
    for (auto& p : projectiles_) {
        if (p->IsDead()) continue;
        float dist = Length(Subtract(p->GetPosition(), camera_->GetTranslate()));
        if (dist < 1.0f) {
            p->Kill();
            hitCount_++;
            damageEffectStrength_ = 1.0f;
            MyGame::SetPostEffectMode(PostProcessManager::kModeGrayscale);

            // 3回ヒットしたらゲームオーバー演出開始
            if (hitCount_ >= kMaxHits) {
                phase_ = Phase::GameOverVignette;
                phaseTimer_ = 0.0f;
                vignetteScale_ = 16.0f;
                vignetteExponent_ = 0.8f;
                damageEffectStrength_ = 0.0f;

                MyGame::SetPostEffectMode(PostProcessManager::kModeVignette);
                MyGame::SetPostEffectStrength(1.0f);
                PostProcessManager::SetVignetteParams(vignetteScale_, vignetteExponent_);
                projectiles_.clear();

                leftSideSprite_->Update();
                rightSideSprite_->Update();
                return; // ここでUpdateを抜けてゲームを一時停止させる
            }
        }
    }

    // グレースケールダメージエフェクトの減衰（プレイ中のみ）
    if (damageEffectStrength_ > 0.0f) {
        damageEffectStrength_ -= 0.01f;
        if (damageEffectStrength_ < 0.0f) {
            damageEffectStrength_ = 0.0f;
            MyGame::SetPostEffectMode(PostProcessManager::kModeCopy);
        }
    }
    MyGame::SetPostEffectStrength(damageEffectStrength_);

    // 死んだプロジェクタイルを削除
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
        [](const std::unique_ptr<EnemyProjectile>& p) { return p->IsDead(); }),
        projectiles_.end());

    // スプライト更新（照準をマウス位置に合わせる）
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(Win32Window::GetInstance()->GetHwnd(), &mousePos);
    crosshair_->SetTranslate({ (float)mousePos.x, (float)mousePos.y });
    crosshair_->Update();
    leftSideSprite_->Update();
    rightSideSprite_->Update();

    // カメラ更新
    leftCamera_->SetTranslate(camera_->GetTranslate());
    leftCamera_->SetRotate({ camera_->GetRotate().x, camera_->GetRotate().y - kHalfPI, camera_->GetRotate().z });
    leftCamera_->Update();
    rightCamera_->SetTranslate(camera_->GetTranslate());
    rightCamera_->SetRotate({ camera_->GetRotate().x, camera_->GetRotate().y + kHalfPI, camera_->GetRotate().z });
    rightCamera_->Update();
    camera_->Update();

    // 射撃（クリック）処理
    if (Input::GetInstance()->IsMouseButtonTriggered(0)) {
        float x = (float)mousePos.x / Win32Window::kClientWidth * 2.0f - 1.0f;
        float y = 1.0f - (float)mousePos.y / Win32Window::kClientHeight * 2.0f;
        Matrix4x4 vp = camera_->GetViewProjectionMatrix();
        Matrix4x4 invVP = Inverse(vp);
        Vector3 nearPos = TransformPoint({ x, y, 0.0f }, invVP);
        Vector3 farPos = TransformPoint({ x, y, 1.0f }, invVP);
        Vector3 rayDir = Normalize(Subtract(farPos, nearPos));

        // 的への当たり判定
        Vector3 targetPos = target_->GetTranslate();
        Vector3 toTarget = Subtract(targetPos, nearPos);
        float t = Dot(toTarget, rayDir);
        if (t > 0) {
            Vector3 closestPoint = Add(nearPos, Multiply(t, rayDir));
            float distance = Length(Subtract(targetPos, closestPoint));
            if (distance < 1.0f) {
                isHit_ = true;
                hitFeedbackTimer_ = 0.2f;

                // 当たった瞬間に、その場で位置を合わせて行列を更新する
                hitEffectPlane_->SetTranslate(targetPos);

                // マルチビュー（メイン、左、右）のすべてのカメラに対して即座に更新をかける
                hitEffectPlane_->Update(mainViewIndex_, camera_.get());
                hitEffectPlane_->Update(leftViewIndex_, leftCamera_.get());
                hitEffectPlane_->Update(rightViewIndex_, rightCamera_.get());
            }
        }

        // プロジェクタイルへの当たり判定（撃ち落とし）
        for (auto& p : projectiles_) {
            Vector3 pPos = p->GetPosition();
            Vector3 toProjectile = Subtract(pPos, nearPos);
            float tp = Dot(toProjectile, rayDir);
            if (tp > 0) {
                Vector3 closestPoint = Add(nearPos, Multiply(tp, rayDir));
                float distance = Length(Subtract(pPos, closestPoint));
                if (distance < p->GetRadius() * 2.0f) {
                    p->Kill();
                }
            }
        }
    }
}

void ShootingScene::DrawOffscreen() {
    auto* cmd = DX12Context::GetInstance()->GetCommandList();

    // 左側カメラの描画 (ViewIndex 1)
    leftRT_->PreDraw(cmd);
    Object3dCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));
    if (isShowMaterial_) {
        target_->Draw(1);
        if (hitFeedbackTimer_ > 0 && hitEffectPlane_) hitEffectPlane_->Draw(1);
        for (auto& enemy : levelEnemies_) enemy->Draw(1);
    }
    for (auto& p : projectiles_) p->Draw(1);
    if (isShowSkybox_) skybox_->Draw(1);
    leftRT_->PostDraw(cmd);

    // 右側カメラの描画 (ViewIndex 2)
    rightRT_->PreDraw(cmd);
    Object3dCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));
    if (isShowMaterial_) {
        target_->Draw(2);
        if (hitFeedbackTimer_ > 0 && hitEffectPlane_) hitEffectPlane_->Draw(2);
        for (auto& enemy : levelEnemies_) enemy->Draw(2);
    }
    for (auto& p : projectiles_) p->Draw(2);
    if (isShowSkybox_) skybox_->Draw(2);
    rightRT_->PostDraw(cmd);
}

#ifdef USE_IMGUI
// ImGui操作の更新
void ShootingScene::UpdateImGui() {
    // メイン設定ウィンドウの位置とサイズ
    ImGui::SetNextWindowPos(ImVec2(Win32Window::kClientWidth - 10.0f, 10.0f), ImGuiCond_Once, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(450.0f, 600.0f), ImGuiCond_Once);

    if (ImGui::Begin("Settings")) {
        UpdateImGui_GlobalSettings();
        UpdateImGui_GameCamera();
        UpdateImGui_Object3d();
        UpdateImGui_Skybox();
    }
	ImGui::End();
}

// ImGuiでグローバル設定のパラメータを調整するための関数
void ShootingScene::UpdateImGui_GlobalSettings() {
    ImGui::Separator();
    if (ImGui::TreeNode("Global Settings")) {
        ImGui::Separator();
        ImGui::Text("Blend Mode");
        const char* blendModeNames[] = { "None", "Normal", "Add", "Subtractive", "Multiply", "Screen" };
        int currentBlendModeInt = static_cast<int>(currentBlendMode_);
        if (ImGui::Combo("##blendMode", &currentBlendModeInt, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
            currentBlendMode_ = static_cast<BlendState>(currentBlendModeInt);
        }
        ImGui::Separator();
        // ヒットポイントのパラメータを確認
        ImGui::Text("Hit Points: %d / %d", hitCount_, kMaxHits);
        if (ImGui::Button("Reset Hit Count")) {
            hitCount_ = 0;
        }
        if (hitCount_ < kMaxHits) {
            ImGui::Text("Player is Alive");
        }
        else {
            ImGui::Text("Player is Dead");
        }
        ImGui::Separator();
        // --- シーンを初期化（論理リセット） ---
        if (ImGui::Button("Reset Game")) {
            hitCount_ = 0;
            orbitAngle_ = 0.0f;
            targetTimer_ = 0.0f;
            cameraYaw_ = 0.0f;
            hitFeedbackTimer_ = 0.0f;
            isHit_ = false;
            damageEffectStrength_ = 0.0f;
            projectileSpawnTimer_ = 0.0f;
            projectiles_.clear();
            levelEnemies_.clear();
            if (levelData_) {
                for (const auto& enemyData : levelData_->enemies) {
                    auto enemy = std::make_unique<Object3d>();
                    enemy->Initialize();
                    enemy->SetModel(sphereModel_);
                    enemy->SetTranslate(enemyData.translation);
                    enemy->SetRotation(enemyData.rotation);
                    enemy->SetCamera(camera_.get());
                    levelEnemies_.push_back(std::move(enemy));
                }
            }
            // カメラを初期位置に戻す
            if (levelData_ && !levelData_->players.empty()) {
                const auto& spawn = levelData_->players[0];
                camera_->SetTranslate(spawn.translation);
                camera_->SetRotate(spawn.rotation);
                cameraYaw_ = spawn.rotation.y;
            }
            else {
                camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
                camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
                cameraYaw_ = 0.0f;
            }
            // スムージングを最大限（31）適用してリスタート
            smoothingKernel_ = 31.0f;
            PostProcessManager::SetMode(PostProcessManager::kModeSmoothing);
            PostProcessManager::SetSmoothingParams(static_cast<int>(smoothingKernel_));
            phase_ = Phase::RestartSmoothing;
            phaseTimer_ = 0.0f;
        }
        ImGui::TreePop();
    }
}
// ImGuiでゲームカメラのパラメータを調整するための関数
void ShootingScene::UpdateImGui_GameCamera() {
    ImGui::Separator();
    if (ImGui::TreeNode("Game Camera")) {
        Vector3 camPos = camera_->GetTranslate();
        Vector3 camRot = camera_->GetRotate();
        if (ImGui::DragFloat3("Position", &camPos.x, 0.1f)) {
            camera_->SetTranslate(camPos);
        }
        if (ImGui::DragFloat3("Rotation", &camRot.x, 0.1f)) {
            camera_->SetRotate(camRot);
            cameraYaw_ = camRot.y;
        }
        ImGui::TreePop();
    }
}
// ImGuiでObject3dのパラメータを調整するための関数
void ShootingScene::UpdateImGui_Object3d() {
    if (ImGui::TreeNode("Target Object")) {
        Vector3 pos = target_->GetTranslate();
        Vector3 rot = target_->GetRotation();
        Vector3 scale = target_->GetScale();
        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
            target_->SetTranslate(pos);
        }
        if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f)) {
            target_->SetRotation(rot);
        }
        if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
            target_->SetScale(scale);
        }
        ImGui::TreePop();
    }
}

// ImGuiでSkyboxのパラメータを調整するための関数
void ShootingScene::UpdateImGui_Skybox() {
    ImGui::Separator();
    if (ImGui::TreeNode("Skybox")) {
        if (skybox_) {
            ImGui::DragFloat3("scale", &skybox_->GetScale().x, 0.1f);
            ImGui::DragFloat3("rotate", &skybox_->GetRotate().x, 0.01f);
            ImGui::DragFloat3("translate", &skybox_->GetTranslate().x, 0.1f);
        }
        ImGui::TreePop();
    }
}
#endif // USE_IMGUI

void ShootingScene::Draw() {
    // 描画設定
    Object3dCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));

    // メインカメラ (ViewIndex 0)
    if (isShowMaterial_) {
        target_->Draw(0);
        if (hitFeedbackTimer_ > 0 && hitEffectPlane_) hitEffectPlane_->Draw(0);
        for (auto& enemy : levelEnemies_) enemy->Draw(0);
    }
    for (auto& p : projectiles_) p->Draw(0);
    if (isShowSkybox_) skybox_->Draw(0);

    // スプライトの描画
    SpriteCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));
    if (isShowSprite_) {
        if (crosshair_) crosshair_->Draw();
        leftSideSprite_->Draw();
        rightSideSprite_->Draw();
    }
}

void ShootingScene::Finalize() {
    Object3dCommon::GetInstance()->SetDefaultCamera(nullptr);
    levelEnemies_.clear();
}