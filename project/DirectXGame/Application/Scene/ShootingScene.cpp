#include "ShootingScene.h"
#include "DX12Context.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "BlendMode.h"
#include "Input.h"
#include "Win32Window.h"
#include "MathUtils.h"
#include <cmath>
#include <numbers>
#include "MyGame.h"

using namespace MathUtils;
using namespace BlendMode;

void ShootingScene::Initialize() {
    // カメラの初期化
    camera_ = std::make_unique<Camera>();
    camera_->Initialize();
    camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
    camera_->SetRotate({ 0.0f, 0.0f, 0.0f });

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

            // カメラを初期位置に戻す
            camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
            camera_->SetRotate({ 0.0f, 0.0f, 0.0f });

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

    // A/Dキーでカメラ回転
    if (Input::GetInstance()->IsKeyDown(DIK_A)) {
        cameraYaw_ -= kHalfPI / 100.0f;
    }
    if (Input::GetInstance()->IsKeyDown(DIK_D)) {
        cameraYaw_ += kHalfPI / 100.0f;
    }
    camera_->SetRotate({ 0.0f, cameraYaw_, 0.0f });

    // ターゲットの移動
    orbitAngle_ += 0.005f;
    targetTimer_ += 0.05f;

    Vector3 camPos = camera_->GetTranslate();
    Vector3 pos;
    pos.x = camPos.x + std::sin(orbitAngle_) * orbitRadius_;
    pos.z = camPos.z + std::cos(orbitAngle_) * orbitRadius_;
    pos.y = targetBasePos_.y + std::sin(targetTimer_) * 2.0f;
    target_->SetTranslate(pos);

    // ヒットフィードバック
    if (hitFeedbackTimer_ > 0) {
        hitFeedbackTimer_ -= kDeltaTime;
        target_->SetScale({ 1.5f, 1.5f, 1.5f });
    }
    else {
        target_->SetScale({ 1.0f, 1.0f, 1.0f });
    }

    // オブジェクトの更新 (マルチビュー対応)
    target_->Update(0, camera_.get());
    target_->Update(1, leftCamera_.get());
    target_->Update(2, rightCamera_.get());

    skybox_->Update(0, camera_.get());
    skybox_->Update(1, leftCamera_.get());
    skybox_->Update(2, rightCamera_.get());

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
        p->Update(0, camera_.get());
        p->Update(1, leftCamera_.get());
        p->Update(2, rightCamera_.get());
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
    if (isShowMaterial_) target_->Draw(1);
    for (auto& p : projectiles_) p->Draw(1);
    if (isShowSkybox_) skybox_->Draw(1);
    leftRT_->PostDraw(cmd);

    // 右側カメラの描画 (ViewIndex 2)
    rightRT_->PreDraw(cmd);
    Object3dCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));
    if (isShowMaterial_) target_->Draw(2);
    for (auto& p : projectiles_) p->Draw(2);
    if (isShowSkybox_) skybox_->Draw(2);
    rightRT_->PostDraw(cmd);
}

void ShootingScene::Draw() {
    // 描画設定
    Object3dCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));

    // メインカメラ (ViewIndex 0)
    if (isShowMaterial_) target_->Draw(0);
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
}