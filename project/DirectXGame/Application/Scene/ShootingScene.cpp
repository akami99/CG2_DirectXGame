#include "ShootingScene.h"
#include "BlendMode.h"
#include "DX12Context.h"
#include "Input.h"
#include "MathUtils.h"
#include "Model.h"
#include "ModelManager.h"
#include "MyGame.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "Win32Window.h"
#include <cmath>
#include <numbers>

using namespace MathUtils;
using namespace BlendMode;

ShootingScene::ShootingScene() = default;
ShootingScene::~ShootingScene() = default;

void ShootingScene::Initialize() {
  // レベルデータのロード
  levelData_ = LevelLoader::LoadFile("testScene");

  // レールデータの抽出
  railPoints_.clear();
  if (levelData_) {
    for (const auto &obj : levelData_->objects) {
      if (obj.type == "CURVE" && obj.name == "EventRail") {
        railPoints_ = obj.bezierPoints;
        break;
      }
    }
  }
  if (railPoints_.empty()) {
    // ダミーのレールを生成 (Z方向にまっすぐ進むなど)
    LevelData::BezierControlPoint pt0, pt1;
    pt0.co = {0.0f, 2.0f, -15.0f};
    pt0.handleLeft = pt0.co;
    pt0.handleRight = pt0.co;

    pt1.co = {0.0f, 2.0f, 175.0f};
    pt1.handleLeft = pt1.co;
    pt1.handleRight = pt1.co;

    railPoints_.push_back(pt0);
    railPoints_.push_back(pt1);
  }

  // カメラの初期化
  camera_ = std::make_unique<Camera>();
  camera_->Initialize();

  if (levelData_ && !levelData_->players.empty()) {
    maxProgress_ = levelData_->players[0].distance;
    if (maxProgress_ <= 0.0f) {
      maxProgress_ = 190.0f;
    }
  } else {
    maxProgress_ = 190.0f;
  }

  cameraProgress_ = 0.0f;
  isMovementPaused_ = false;
  Vector3 startCamPos = CalculateRailPosition(cameraProgress_);
  camera_->SetTranslate(startCamPos);

  if (levelData_ && !levelData_->players.empty()) {
    const auto &spawn = levelData_->players[0];
    camera_->SetRotate(spawn.rotation);
    cameraYaw_ = spawn.rotation.y;
  } else {
    camera_->SetRotate({0.0f, 0.0f, 0.0f});
    cameraYaw_ = 0.0f;
  }

  Object3dCommon::GetInstance()->SetDefaultCamera(camera_.get());

  // サイドカメラの初期化
 /* leftCamera_ = std::make_unique<Camera>();
  leftCamera_->Initialize();
  rightCamera_ = std::make_unique<Camera>();
  rightCamera_->Initialize();*/

  // テクスチャの読み込み
  TextureManager::GetInstance()->LoadTexture(crosshairPath_);

  // モデル読み込み
  ModelManager::GetInstance()->LoadModel("enemy", enemyModel_);
  ModelManager::GetInstance()->LoadModel("bullet", "bullet.obj");

  // 環境マップを敵モデルに適用
  Model* enemyModel = ModelManager::GetInstance()->FindModel(enemyModel_);
  if (enemyModel) {
    enemyModel->SetEnvironmentCoefficient(1.0f); // 反射強度を最大（1.0f）に設定
  }

  // スカイボックスの初期化
  skybox_ = std::make_unique<Skybox>();
  skybox_->Initialize("skybox.dds");
  skybox_->SetCamera(camera_.get());

  Object3dCommon::GetInstance()->SetEnvironmentMap(
      TextureManager::GetInstance()->GetSrvIndex("skybox.dds"));

  // レベルデータから敵キャラを生成・配置
  enemies_.clear();
  if (levelData_) {
    for (const auto &enemyData : levelData_->enemies) {
      EnemyInfo enemy;
      enemy.object = std::make_unique<Object3d>();
      enemy.object->Initialize();
      enemy.object->SetModel(enemyModel_);
      enemy.object->SetTranslate(enemyData.translation);
      enemy.object->SetRotation(enemyData.rotation);
      enemy.object->SetCamera(camera_.get());
      enemy.distance = enemyData.distance;
      enemy.isActive = false;
      enemy.isDead = false;
      enemy.shootTimer = 0.0f;
      enemies_.push_back(std::move(enemy));
    }
  }

  // プリミティブ平面モデルの動的生成
  hitEffectPlaneModel_ = std::make_unique<Model>();
  PlaneSettings planeSettings;
  planeSettings.size = {3.0f, 3.0f};
  planeSettings.divisionX = 1;
  planeSettings.divisionY = 1;
  planeSettings.color = {1.0f, 1.0f, 1.0f, 1.0f};
  hitEffectPlaneModel_->CreatePlane(crosshairPath_, planeSettings);

  hitEffectPlane_ = std::make_unique<Object3d>();
  hitEffectPlane_->Initialize();
  hitEffectPlane_->SetModel(hitEffectPlaneModel_.get());
  hitEffectPlane_->SetCamera(camera_.get());

  // --- スプライト生成 ---
  crosshair_ = std::make_unique<Sprite>();
  crosshair_->Initialize(crosshairPath_);
  crosshair_->SetAnchorPoint({0.5f, 0.5f});
  crosshair_->SetScale({64.0f, 64.0f});

  // --- オフスクリーン用リソース ---
 /* leftRT_ = std::make_unique<RenderTexture>();
  leftRT_->Initialize(320, 180);
  rightRT_ = std::make_unique<RenderTexture>();
  rightRT_->Initialize(320, 180);

  leftSideSprite_ = std::make_unique<Sprite>();
  leftSideSprite_->Initialize(crosshairPath_);
  leftSideSprite_->SetTexture(leftRT_->GetSrvIndex());
  leftSideSprite_->SetTranslate({0.0f, 0.0f});
  leftSideSprite_->SetScale({320.0f, 180.0f});

  rightSideSprite_ = std::make_unique<Sprite>();
  rightSideSprite_->Initialize(crosshairPath_);
  rightSideSprite_->SetTexture(rightRT_->GetSrvIndex());
  rightSideSprite_->SetTranslate({Win32Window::kClientWidth - 320.0f, 0.0f});
  rightSideSprite_->SetScale({320.0f, 180.0f});*/

  // マガジンとカバーの初期化
  ammo_ = kMaxAmmo;
  isCovering_ = false;
  reloadTimer_ = 0.0f;

  // 初期フェーズの設定
  phase_ = Phase::Playing;
  hitCount_ = 0;
}

void ShootingScene::Update()
{
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
        if (t > 1.0f)
            t = 1.0f;

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
        /*leftSideSprite_->Update();
        rightSideSprite_->Update();*/
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
            enemies_.clear();
            ammo_ = kMaxAmmo;
            isCovering_ = false;
            reloadTimer_ = 0.0f;
            if (levelData_) {
                for (const auto& enemyData : levelData_->enemies) {
                    EnemyInfo enemy;
                    enemy.object = std::make_unique<Object3d>();
                    enemy.object->Initialize();
                    enemy.object->SetModel(enemyModel_);
                    enemy.object->SetTranslate(enemyData.translation);
                    enemy.object->SetRotation(enemyData.rotation);
                    enemy.object->SetCamera(camera_.get());
                    enemy.distance = enemyData.distance;
                    enemy.isActive = false;
                    enemy.isDead = false;
                    enemy.shootTimer = 0.0f;
                    enemies_.push_back(std::move(enemy));
                }
            }

            // カメラを初期位置に戻す
            cameraProgress_ = 0.0f;
            isMovementPaused_ = false;
            Vector3 startCamPos = CalculateRailPosition(cameraProgress_);
            camera_->SetTranslate(startCamPos);
            if (levelData_ && !levelData_->players.empty()) {
                const auto& spawn = levelData_->players[0];
                camera_->SetRotate(spawn.rotation);
                cameraYaw_ = spawn.rotation.y;
            }
            else {
                camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
                cameraYaw_ = 0.0f;
            }

            // スムージングを最大限（31）適用してリスタート
            smoothingKernel_ = 31.0f;
            PostProcessManager::SetMode(PostProcessManager::kModeSmoothing);
            PostProcessManager::SetSmoothingParams(
                static_cast<int>(smoothingKernel_));

            phase_ = Phase::RestartSmoothing;
            phaseTimer_ = 0.0f;
        }

        /*leftSideSprite_->Update();
        rightSideSprite_->Update();*/
        return;
    }

    // =====================================================
    // フェーズ: リスタートスムージングフェードアウト
    // =====================================================
    if (phase_ == Phase::RestartSmoothing) {
        phaseTimer_ += kDeltaTime;
        float t = phaseTimer_ / kSmoothingOutDuration; // 0.0 → 1.0
        if (t > 1.0f)
            t = 1.0f;

        // カーネルサイズを 31 → 1 に線形補間
        smoothingKernel_ = 31.0f + (1.0f - 31.0f) * t;
        int kernel = static_cast<int>(smoothingKernel_);
        if (kernel % 2 == 0)
            kernel++; // ぼかしフィルタの仕様上、奇数に補正
        if (kernel < 1)
            kernel = 1;

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

    // 隠れるアクションの更新
    isCovering_ = Input::GetInstance()->IsKeyDown(DIK_SPACE);
    if (isCovering_) {
        if (ammo_ < kMaxAmmo) {
            reloadTimer_ += kDeltaTime;
            if (reloadTimer_ >= kReloadDuration) {
                ammo_ = kMaxAmmo;
                reloadTimer_ = 0.0f;
            }
        }
    } else {
        reloadTimer_ = 0.0f;
    }

    // --- カメラの移動 ---
    if (!isMovementPaused_) {
        cameraProgress_ += kCameraSpeed;
        if (cameraProgress_ > maxProgress_) {
            cameraProgress_ = maxProgress_;
        }
    }
    Vector3 camPos = CalculateRailPosition(cameraProgress_);
    if (isCovering_) {
        camPos.y -= 1.0f; // 遮蔽に隠れるイメージでカメラの高さを下げる
    }
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

    // エネミーの出現チェック (進行中のみ)
    if (!isMovementPaused_) {
        bool triggered = false;
        for (auto& enemy : enemies_) {
            if (!enemy.isActive && !enemy.isDead) {
                if (cameraProgress_ >= enemy.distance) {
                    enemy.isActive = true;
                    triggered = true;
                }
            }
        }
        if (triggered) {
            isMovementPaused_ = true;
        }
    }

    // 進行一時停止中なら、画面内の全アクティブエネミーが全滅したかチェック
    if (isMovementPaused_) {
        bool allDead = true;
        for (const auto& enemy : enemies_) {
            if (enemy.isActive && !enemy.isDead) {
                allDead = false;
                break;
            }
        }
        if (allDead) {
            isMovementPaused_ = false;
        }
    }

    // ヒットフィードバックのタイマー更新
    if (hitFeedbackTimer_ > 0) {
        hitFeedbackTimer_ -= kDeltaTime;

        // Planeの回転角を更新（Y軸/Z軸で回転）
        Vector3 rot = hitEffectPlane_->GetRotation();
        rot.y += 0.1f;
        rot.z += 0.1f;
        hitEffectPlane_->SetRotation(rot);
        // 位置は当たり判定時に設定済み
    }

    // 各エネミーの更新と射撃処理
    for (auto& enemy : enemies_) {
        if (!enemy.isActive || enemy.isDead)
            continue;

        // 射撃処理
        enemy.shootTimer += 1.0f;
        if (enemy.shootTimer >= kProjectileSpawnInterval) {
            enemy.shootTimer = 0.0f;
            Vector3 startPos = enemy.object->GetTranslate();
            Vector3 targetPos = camera_->GetTranslate();
            Vector3 dir = Normalize(Subtract(targetPos, startPos));
            float speed = 0.1f;
            Vector3 velocity = Multiply(speed, dir);
            auto newProjectile = std::make_unique<EnemyProjectile>();
            newProjectile->Initialize(startPos, velocity);
            projectiles_.push_back(std::move(newProjectile));
        }

        // オブジェクトの更新
        enemy.object->Update(mainViewIndex_, camera_.get());
       /* enemy.object->Update(leftViewIndex_, leftCamera_.get());
        enemy.object->Update(rightViewIndex_, rightCamera_.get());*/
    }

    if (hitFeedbackTimer_ > 0) {
        hitEffectPlane_->Update(mainViewIndex_, camera_.get());
       /* hitEffectPlane_->Update(leftViewIndex_, leftCamera_.get());
        hitEffectPlane_->Update(rightViewIndex_, rightCamera_.get());*/
    }

    skybox_->Update(mainViewIndex_, camera_.get());
    /*skybox_->Update(leftViewIndex_, leftCamera_.get());
    skybox_->Update(rightViewIndex_, rightCamera_.get());*/

    // プロジェクタイルの移動・行列更新
    for (auto& p : projectiles_) {
        p->Update();
        p->Update(mainViewIndex_, camera_.get());
        /*p->Update(leftViewIndex_, leftCamera_.get());
        p->Update(rightViewIndex_, rightCamera_.get());*/
    }

    // カメラとの当たり判定（プレイヤー被弾処理）
    if (!isCovering_) {
        for (auto& p : projectiles_) {
            if (p->IsDead())
                continue;
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
                    PostProcessManager::SetVignetteParams(vignetteScale_,
                        vignetteExponent_);
                    projectiles_.clear();

                  /*  leftSideSprite_->Update();
                    rightSideSprite_->Update();*/
                    return; // ここでUpdateを抜けてゲームを一時停止させる
                }
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
    projectiles_.erase(
        std::remove_if(projectiles_.begin(), projectiles_.end(),
            [](const std::unique_ptr<EnemyProjectile>& p) {
                return p->IsDead();
            }),
        projectiles_.end());

    // スプライト更新（照準をマウス位置に合わせる）
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(Win32Window::GetInstance()->GetHwnd(), &mousePos);
    crosshair_->SetTranslate({ (float)mousePos.x, (float)mousePos.y });
    crosshair_->Update();
    /*leftSideSprite_->Update();
    rightSideSprite_->Update();*/

    // カメラ更新
    /*leftCamera_->SetTranslate(camera_->GetTranslate());
    leftCamera_->SetRotate({ camera_->GetRotate().x,
                            camera_->GetRotate().y - kHalfPI,
                            camera_->GetRotate().z });
    leftCamera_->Update();
    rightCamera_->SetTranslate(camera_->GetTranslate());
    rightCamera_->SetRotate({ camera_->GetRotate().x,
                             camera_->GetRotate().y + kHalfPI,
                             camera_->GetRotate().z });
    rightCamera_->Update();*/
    camera_->Update();

    // 射撃（クリック）処理
    if (!isCovering_ && ammo_ > 0 && Input::GetInstance()->IsMouseButtonTriggered(0)) {
        ammo_--; // 弾数を減らす
        float x = (float)mousePos.x / Win32Window::kClientWidth * 2.0f - 1.0f;
        float y = 1.0f - (float)mousePos.y / Win32Window::kClientHeight * 2.0f;
        Matrix4x4 vp = camera_->GetViewProjectionMatrix();
        Matrix4x4 invVP = Inverse(vp);
        Vector3 nearPos = TransformPoint({ x, y, 0.0f }, invVP);
        Vector3 farPos = TransformPoint({ x, y, 1.0f }, invVP);
        Vector3 rayDir = Normalize(Subtract(farPos, nearPos));

        // エネミーへの当たり判定
        bool hitAny = false;
        Vector3 hitPos = { 0.0f, 0.0f, 0.0f };
        for (auto& enemy : enemies_) {
            if (!enemy.isActive || enemy.isDead)
                continue;

            Vector3 enemyPos = enemy.object->GetTranslate();
            Vector3 toEnemy = Subtract(enemyPos, nearPos);
            float t = Dot(toEnemy, rayDir);
            if (t > 0) {
                Vector3 closestPoint = Add(nearPos, Multiply(t, rayDir));
                float dist = Length(Subtract(enemyPos, closestPoint));
                if (dist < 1.0f) {
                    enemy.isDead = true;
                    enemy.isActive = false;
                    hitAny = true;
                    hitPos = enemyPos;
                    break; // 1回で1体倒す
                }
            }
        }

        if (hitAny) {
            isHit_ = true;
            hitFeedbackTimer_ = 0.2f;
            hitEffectPlane_->SetTranslate(hitPos);
            hitEffectPlane_->Update(mainViewIndex_, camera_.get());
            /*hitEffectPlane_->Update(leftViewIndex_, leftCamera_.get());
            hitEffectPlane_->Update(rightViewIndex_, rightCamera_.get());*/
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

    // クリア判定 (終点到達かつ敵が全滅)
    if (cameraProgress_ >= maxProgress_) {
        bool allDead = true;
        for (const auto& enemy : enemies_) {
            if (!enemy.isDead) {
                allDead = false;
                break;
            }
        }
        if (allDead) {
            // クリア時の暗転演出へ (ゲームオーバーの暗転処理を流用して初期状態からリスタート)
            phase_ = Phase::GameOverVignette;
            phaseTimer_ = 0.0f;
            vignetteScale_ = 16.0f;
            vignetteExponent_ = 0.8f;
            damageEffectStrength_ = 0.0f;

            MyGame::SetPostEffectMode(PostProcessManager::kModeVignette);
            MyGame::SetPostEffectStrength(1.0f);
            PostProcessManager::SetVignetteParams(vignetteScale_, vignetteExponent_);
            projectiles_.clear();
            return;
        }
    }
}
  void ShootingScene::DrawOffscreen() {
    auto *cmd = DX12Context::GetInstance()->GetCommandList();

    // 左側カメラの描画 (ViewIndex 1)
    //leftRT_->PreDraw(cmd);
    //Object3dCommon::GetInstance()->SetCommonDrawSettings(
    //    static_cast<BlendState>(currentBlendMode_));
    //if (isShowMaterial_) {
    //  if (hitFeedbackTimer_ > 0 && hitEffectPlane_)
    //    hitEffectPlane_->Draw(1);
    //  for (auto &enemy : enemies_) {
    //    if (enemy.isActive && !enemy.isDead) {
    //      enemy.object->Draw(1);
    //    }
    //  }
    //}
    //for (auto &p : projectiles_)
    //  p->Draw(1);
    //if (isShowSkybox_)
    //  skybox_->Draw(1);
    //leftRT_->PostDraw(cmd);

    //// 右側カメラの描画 (ViewIndex 2)
    //rightRT_->PreDraw(cmd);
    //Object3dCommon::GetInstance()->SetCommonDrawSettings(
    //    static_cast<BlendState>(currentBlendMode_));
    //if (isShowMaterial_) {
    //  if (hitFeedbackTimer_ > 0 && hitEffectPlane_)
    //    hitEffectPlane_->Draw(2);
    //  for (auto &enemy : enemies_) {
    //    if (enemy.isActive && !enemy.isDead) {
    //      enemy.object->Draw(2);
    //    }
    //  }
    //}
    //for (auto &p : projectiles_)
    //  p->Draw(2);
    //if (isShowSkybox_)
    //  skybox_->Draw(2);
    //rightRT_->PostDraw(cmd);
  }

#ifdef USE_IMGUI
  // ImGui操作の更新
  void ShootingScene::UpdateImGui() {
    // メイン設定ウィンドウの位置とサイズ
    ImGui::SetNextWindowPos(ImVec2(Win32Window::kClientWidth - 10.0f, 10.0f),
                            ImGuiCond_Once, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(450.0f, 600.0f), ImGuiCond_Once);

    if (ImGui::Begin("Settings")) {
      UpdateImGui_GlobalSettings();
      UpdateImGui_GameCamera();
      UpdateImGui_Object3d();
      UpdateImGui_Skybox();
    }
    ImGui::End();

    UpdateImGui_GameStatus();
  }

  // ImGuiでグローバル設定のパラメータを調整するための関数
  void ShootingScene::UpdateImGui_GlobalSettings() {
    ImGui::Separator();
    if (ImGui::TreeNode("Global Settings")) {
      ImGui::Separator();
      ImGui::Text("Blend Mode");
      const char *blendModeNames[] = {"None",        "Normal",   "Add",
                                      "Subtractive", "Multiply", "Screen"};
      int currentBlendModeInt = static_cast<int>(currentBlendMode_);
      if (ImGui::Combo("##blendMode", &currentBlendModeInt, blendModeNames,
                       IM_ARRAYSIZE(blendModeNames))) {
        currentBlendMode_ = static_cast<BlendState>(currentBlendModeInt);
      }
      ImGui::Separator();
      // ヒットポイントのパラメータを確認
      ImGui::Text("Hit Count: %d / %d", hitCount_, kMaxHits);
      if (ImGui::Button("Reset Hit Count")) {
        hitCount_ = 0;
      }
      if (hitCount_ < kMaxHits) {
        ImGui::Text("Player is Alive");
      } else {
        ImGui::Text("Player is Dead");
      }
      ImGui::Separator();
      ImGui::Text("Ammo: %d / %d", ammo_, kMaxAmmo);
      ImGui::Text("Covering (SPACE): %s", isCovering_ ? "YES (Invincible)" : "NO");
      if (isCovering_ && ammo_ < kMaxAmmo) {
        ImGui::ProgressBar(reloadTimer_ / kReloadDuration, ImVec2(0.0f, 0.0f), "Reloading...");
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
        enemies_.clear();
        if (levelData_) {
          for (const auto &enemyData : levelData_->enemies) {
            EnemyInfo enemy;
            enemy.object = std::make_unique<Object3d>();
            enemy.object->Initialize();
            enemy.object->SetModel(enemyModel_);
            enemy.object->SetTranslate(enemyData.translation);
            enemy.object->SetRotation(enemyData.rotation);
            enemy.object->SetCamera(camera_.get());
            enemy.distance = enemyData.distance;
            enemy.isActive = false;
            enemy.isDead = false;
            enemy.shootTimer = 0.0f;
            enemies_.push_back(std::move(enemy));
          }
        }
        // カメラを初期位置に戻す
        cameraProgress_ = 0.0f;
        isMovementPaused_ = false;
        Vector3 startCamPos = CalculateRailPosition(cameraProgress_);
        camera_->SetTranslate(startCamPos);
        if (levelData_ && !levelData_->players.empty()) {
          const auto &spawn = levelData_->players[0];
          camera_->SetRotate(spawn.rotation);
          cameraYaw_ = spawn.rotation.y;
        } else {
          camera_->SetRotate({0.0f, 0.0f, 0.0f});
          cameraYaw_ = 0.0f;
        }
        // スムージングを最大限（31）適用してリスタート
        smoothingKernel_ = 31.0f;
        PostProcessManager::SetMode(PostProcessManager::kModeSmoothing);
        PostProcessManager::SetSmoothingParams(
            static_cast<int>(smoothingKernel_));
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
    if (ImGui::TreeNode("Enemies Status")) {
      ImGui::Text("Movement Paused: %s", isMovementPaused_ ? "True" : "False");
      ImGui::Text("Progress: %.2f / %.2f", cameraProgress_, maxProgress_);

      for (size_t i = 0; i < enemies_.size(); ++i) {
        auto &enemy = enemies_[i];
        ImGui::Text("[%zu] Dist: %.1f, Active: %s, Dead: %s", i, enemy.distance,
                    enemy.isActive ? "Yes" : "No", enemy.isDead ? "Yes" : "No");
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

  // ImGuiでゲームの進行状況やプレイヤーの状態を表示するための関数
  void ShootingScene::UpdateImGui_GameStatus() {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(200.0f, 100.0f), ImGuiCond_Once);
    ImGui::Begin("Game Status", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
    if (phase_ == Phase::GameOverVignette || phase_ == Phase::GameOverWait) {
      ImGui::Text("GAME OVER");
    } else if (phase_ == Phase::RestartSmoothing) {
      ImGui::Text("Restarting...");
    }
    else {
        ImGui::ProgressBar(cameraProgress_ / maxProgress_, ImVec2(0.0f, 0.0f), "Progress");
    }
    ImGui::Separator();
    ImGui::Text("Player Status");
    ImGui::Separator();
    ImGui::Text("Hit Count: %d / %d", hitCount_, kMaxHits);
    ImGui::Text("Ammo: %d / %d", ammo_, kMaxAmmo);
    ImGui::Text("Covering (SPACE): %s", isCovering_ ? "YES (Invincible)" : "NO");
    if (isCovering_ && ammo_ < kMaxAmmo) {
        ImGui::ProgressBar(reloadTimer_ / kReloadDuration, ImVec2(0.0f, 0.0f), "Reloading...");
    }
    ImGui::Separator();
    ImGui::Text("Control");
    ImGui::Separator();
    ImGui::Text("Mouse: Aim");
    ImGui::Text("LMB: Shoot");
    ImGui::Text("SPACE: Cover (Reload)");
    ImGui::End();
  }
#endif // USE_IMGUI

  void ShootingScene::Draw() {
    // 描画設定
    Object3dCommon::GetInstance()->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode_));

    // メインカメラ (ViewIndex 0)
    if (isShowMaterial_) {
      if (hitFeedbackTimer_ > 0 && hitEffectPlane_)
        hitEffectPlane_->Draw(0);
      for (auto &enemy : enemies_) {
        if (enemy.isActive && !enemy.isDead) {
          enemy.object->Draw(0);
        }
      }
    }
    for (auto &p : projectiles_)
      p->Draw(0);
    if (isShowSkybox_)
      skybox_->Draw(0);

    // スプライトの描画
    SpriteCommon::GetInstance()->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode_));
    if (isShowSprite_) {
      if (crosshair_)
        crosshair_->Draw();
      /*leftSideSprite_->Draw();
      rightSideSprite_->Draw();*/
    }
  }

  void ShootingScene::Finalize() {
    Object3dCommon::GetInstance()->SetDefaultCamera(nullptr);
    enemies_.clear();
  }

  Vector3 ShootingScene::CalculateRailPosition(float progress) {
    if (railPoints_.empty()) {
      return {0.0f, 2.0f, -15.0f};
    }

    float p = progress / maxProgress_;
    if (p < 0.0f)
      p = 0.0f;
    if (p > 1.0f)
      p = 1.0f;

    size_t N = railPoints_.size() - 1;
    float scaledP = p * static_cast<float>(N);
    size_t i = static_cast<size_t>(scaledP);
    if (i >= N) {
      i = N - 1;
    }
    float t = scaledP - static_cast<float>(i);
    if (t < 0.0f)
      t = 0.0f;
    if (t > 1.0f)
      t = 1.0f;

    const auto &p0 = railPoints_[i].co;
    const auto &p1 = railPoints_[i].handleRight;
    const auto &p2 = railPoints_[i + 1].handleLeft;
    const auto &p3 = railPoints_[i + 1].co;

    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    Vector3 pos;
    pos.x =
        uuu * p0.x + 3.0f * uu * t * p1.x + 3.0f * u * tt * p2.x + ttt * p3.x;
    pos.y =
        uuu * p0.y + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.y + ttt * p3.y;
    pos.z =
        uuu * p0.z + 3.0f * uu * t * p1.z + 3.0f * u * tt * p2.z + ttt * p3.z;
    return pos;
  }