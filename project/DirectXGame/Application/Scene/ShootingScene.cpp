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


  // テクスチャの読み込み
  TextureManager::GetInstance()->LoadTexture(crosshairPath_);
  TextureManager::GetInstance()->LoadTexture("Particles/circle.png");

  // モデル読み込み
  ModelManager::GetInstance()->LoadModel("enemy", enemyModel_);
  ModelManager::GetInstance()->LoadModel("bullet", "bullet.obj");

  // 環境マップを敵モデルに適用
  Model *enemyModel = ModelManager::GetInstance()->FindModel(enemyModel_);
  if (enemyModel) {
    enemyModel->SetEnvironmentCoefficient(1.0f); // 反射強度を最大（1.0f）に設定
  }

  // パーティクルグループの作成
  ParticleManager::GetInstance()->CreateParticleGroup(ringParticleGroupName_, "Particles/circle.png");

  // エミッター初期化（RingShapeEffect）
  Transform ringEmitterTransform = {
      { 1.000f, 1.000f, 1.000f }, // scale
      { 0.300f, 0.000f, 0.000f }, // rotate
      { -0.060f, 2.320f, 0.000f } // translate
  };
  ParticleEmitter ringEmitter(ringEmitterTransform, 32, 0.400f);
  ringEmitter.isEmit = false;
  ringEmitter.isEffectMode = true;
  ringEmitter.isLoop = false;
  
  ringEmitter.generateSettings.isRandomScale = false;
  ringEmitter.generateSettings.fixedScale = { 0.250f, 0.500f, 1.000f };
  
  ringEmitter.generateSettings.isRandomRotate = true;
  ringEmitter.generateSettings.rotateMin = { 0.000f, 0.000f, -3.142f };
  ringEmitter.generateSettings.rotateMax = { 0.000f, 0.000f, 3.142f };
  
  ringEmitter.generateSettings.isRandomVelocity = false;
  ringEmitter.generateSettings.fixedVelocity = { 0.000f, -2.000f, 15.000f };
  
  ringEmitter.generateSettings.isRandomLifeTime = false;
  ringEmitter.generateSettings.fixedLifeTime = 1.000f;
  
  ringEmitter.generateSettings.isRandomColor = true;
  ringEmitter.generateSettings.colorMin = { 0.0f, 0.0f, 0.0f, 1.0f };
  ringEmitter.generateSettings.colorMax = { 1.0f, 1.0f, 1.0f, 1.0f };
  
  ringEmitter.fieldSettings.isAccelerationFieldActive = false;
  ringEmitter.fieldSettings.isGravityFieldActive = false;
  
  ringEmitter.uvAnimationSettings.isActive = true;
  ringEmitter.uvAnimationSettings.isIndividual = false;
  ringEmitter.uvAnimationSettings.scrollSpeed = { 2.000f, 0.000f };
  ringEmitter.uvAnimationSettings.rotateSpeed = 0.180f;
  ringEmitter.uvAnimationSettings.scaleSpeed = { 1.500f, -1.200f };
  
  ringEmitter.SetShapeType(ParticleShapeType::Ring);
  if (auto* rs = ringEmitter.GetRingShape()) {
      rs->settings.innerRadius = 0.010f;
      rs->settings.startOuterRadius = 1.000f;
      rs->settings.midOuterRadius = 1.200f;
      rs->settings.endOuterRadius = 1.500f;
      rs->settings.startAngle = 0.000f;
      rs->settings.endAngle = 360.000f;
      rs->settings.division = 32;
      rs->settings.isUvSwap = false;
      rs->settings.innerColor = { 1.000f, 1.000f, 1.000f, 1.000f };
      rs->settings.outerColor = { 1.000f, 1.000f, 1.000f, 1.000f };
      rs->settings.fadeStartAlpha = 1.000f;
      rs->settings.fadeEndAlpha = 1.000f;
      rs->settings.fadeRange = 0.000f;
  }
  ParticleManager::GetInstance()->SetEmitter(ringParticleGroupName_, ringEmitter);

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

      // 個別のモデルインスタンスを生成・ロード
      enemy.model = std::make_unique<Model>();
      enemy.model->Initialize("Resources/Assets/Models/enemy", enemyModel_);
      // 初期状態として完全に消去された状態（Threshold = 1.0f）を設定
      enemy.model->SetDissolveMaskTexture("masks/noise0.png");
      enemy.model->SetDissolveParams(1, 1.0f, 0.05f, Vector3(1.0f, 0.4f, 0.3f));

      enemy.object->SetModel(enemy.model.get());
      enemy.object->SetTranslate(enemyData.translation);
      enemy.object->SetRotation(enemyData.rotation);
      enemy.object->SetCamera(camera_.get());
      enemy.distance = enemyData.distance;
      enemy.isActive = false;
      enemy.isDead = false;
      enemy.shootTimer = 0.0f;
      enemy.spawnTimer = 0.0f;
      enemies_.push_back(std::move(enemy));
    }
  }

  // プリミティブ平面モデルの動的生成
  /*hitEffectPlaneModel_ = std::make_unique<Model>();
  PlaneSettings planeSettings;
  planeSettings.size = {3.0f, 3.0f};
  planeSettings.divisionX = 1;
  planeSettings.divisionY = 1;
  planeSettings.color = {1.0f, 1.0f, 1.0f, 1.0f};
  hitEffectPlaneModel_->CreatePlane(crosshairPath_, planeSettings);

  hitEffectPlane_ = std::make_unique<Object3d>();
  hitEffectPlane_->Initialize();
  hitEffectPlane_->SetModel(hitEffectPlaneModel_.get());
  hitEffectPlane_->SetCamera(camera_.get());*/

  // --- スプライト生成 ---
  crosshair_ = std::make_unique<Sprite>();
  crosshair_->Initialize(crosshairPath_);
  crosshair_->SetAnchorPoint({0.5f, 0.5f});
  crosshair_->SetScale({64.0f, 64.0f});


  // マガジンとカバーの初期化
  ammo_ = kMaxAmmo;
  isCovering_ = false;
  reloadTimer_ = 0.0f;

  // 初期フェーズの設定
  phase_ = Phase::Playing;
  hitCount_ = 0;

  // -----------------------------------------------------------------
  // UIスプライトの初期化
  // -----------------------------------------------------------------
  TextureManager::GetInstance()->LoadTexture("white.png");

  // 1. ライフUI
  lifeBg_ = std::make_unique<Sprite>();
  lifeBg_->Initialize("white.png");
  lifeBg_->SetTranslate({ 1140.0f, 30.0f });
  lifeBg_->SetScale({ 110.0f, 50.0f });
  lifeBg_->SetColor({ 0.2f, 0.2f, 0.2f, 0.8f }); // 半透明グレー

  lifeUnits_.clear();
  lifeCurrentX_.clear();
  lifeTargetX_.clear();
  for (int i = 0; i < kMaxHits; ++i) {
    auto unit = std::make_unique<Sprite>();
    unit->Initialize("white.png");
    unit->SetScale({ 16.0f, 36.0f });
    unit->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色

    float initX = 1214.0f - (kMaxHits - 1 - i) * 24.0f;
    lifeUnits_.push_back(std::move(unit));
    lifeCurrentX_.push_back(initX);
    lifeTargetX_.push_back(initX);
  }

  // 2. 弾薬UI
  ammoBg_ = std::make_unique<Sprite>();
  ammoBg_->Initialize("white.png");
  ammoBg_->SetTranslate({ 30.0f, 640.0f });
  ammoBg_->SetScale({ 200.0f, 40.0f });
  ammoBg_->SetColor({ 0.2f, 0.2f, 0.2f, 0.8f }); // 半透明グレー

  ammoUnits_.clear();
  ammoCurrentX_.clear();
  ammoTargetX_.clear();
  for (int i = 0; i < kMaxAmmo; ++i) {
    auto unit = std::make_unique<Sprite>();
    unit->Initialize("white.png");
    unit->SetScale({ 12.0f, 24.0f });
    unit->SetColor({ 183.0f/255.0f, 132.0f/255.0f, 48.0f/255.0f, 1.0f }); // 茶色系

    float initX = 202.0f - i * 20.0f;
    ammoUnits_.push_back(std::move(unit));
    ammoCurrentX_.push_back(initX);
    ammoTargetX_.push_back(initX);
  }

  // 3. プログレスバーUI
  progressBg_ = std::make_unique<Sprite>();
  progressBg_->Initialize("white.png");
  progressBg_->SetTranslate({ 930.0f, 660.0f });
  progressBg_->SetScale({ 300.0f, 20.0f });
  progressBg_->SetColor({ 0.1f, 0.2f, 0.8f, 0.6f }); // 半透明青色

  progressBar_ = std::make_unique<Sprite>();
  progressBar_->Initialize("white.png");
  progressBar_->SetTranslate({ 930.0f, 660.0f });
  progressBar_->SetScale({ 0.0f, 20.0f }); // 最初は幅0
  progressBar_->SetColor({ 1.0f, 0.9f, 0.0f, 1.0f }); // 黄色

  // 4. カバー演出UI
  coverOverlay_ = std::make_unique<Sprite>();
  coverOverlay_->Initialize("white.png");
  coverOverlay_->SetTranslate({ 0.0f, 470.0f });
  coverOverlay_->SetScale({ 1280.0f, 250.0f });
  coverOverlay_->SetColor({ 0.0f, 0.0f, 0.0f, 0.6f }); // 半透明黒
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
        for (const auto &enemyData : levelData_->enemies) {
          EnemyInfo enemy;
          enemy.object = std::make_unique<Object3d>();
          enemy.object->Initialize();

          // 個別のモデルインスタンスを生成・ロード
          enemy.model = std::make_unique<Model>();
          enemy.model->Initialize("Resources/Assets/Models/enemy", enemyModel_);
          // 初期状態として完全に消去された状態（Threshold = 1.0f）を設定
          enemy.model->SetDissolveMaskTexture("masks/noise0.png");
          enemy.model->SetDissolveParams(1, 1.0f, 0.05f,
                                         Vector3(1.0f, 0.4f, 0.3f));

          enemy.object->SetModel(enemy.model.get());
          enemy.object->SetTranslate(enemyData.translation);
          enemy.object->SetRotation(enemyData.rotation);
          enemy.object->SetCamera(camera_.get());
          enemy.distance = enemyData.distance;
          enemy.isActive = false;
          enemy.isDead = false;
          enemy.shootTimer = 0.0f;
          enemy.spawnTimer = 0.0f;
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

#ifdef USE_IMGUI
  // A/Dキーでカメラ回転
  if (Input::GetInstance()->IsKeyDown(DIK_A)) {
    cameraYaw_ -= kHalfPI / 100.0f;
  }
  if (Input::GetInstance()->IsKeyDown(DIK_D)) {
    cameraYaw_ += kHalfPI / 100.0f;
  }
  Vector3 currentRot = camera_->GetRotate();
  camera_->SetRotate({currentRot.x, cameraYaw_, currentRot.z});
#endif // USE_IMGUI

  // エネミーの出現チェック (進行中のみ)
  if (!isMovementPaused_) {
    bool triggered = false;
    for (auto &enemy : enemies_) {
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
    for (const auto &enemy : enemies_) {
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
    /*Vector3 rot = hitEffectPlane_->GetRotation();
    rot.y += 0.1f;
    rot.z += 0.1f;
    hitEffectPlane_->SetRotation(rot);*/
    // 位置は当たり判定時に設定済み
  }

  // 各エネミーの更新と射撃処理
  for (auto &enemy : enemies_) {
    if (!enemy.isActive || enemy.isDead)
      continue;

    // 出現（Dissolve In）のアニメーション更新
    if (enemy.spawnTimer < 1.0f) {
      enemy.spawnTimer += kDeltaTime;
      if (enemy.spawnTimer > 1.0f) {
        enemy.spawnTimer = 1.0f;
      }
      float threshold = 1.0f - enemy.spawnTimer; // 1.0 -> 0.0

      // 出現完了ならDissolveを無効化
      int enableDissolve = (threshold > 0.0f) ? 1 : 0;
      enemy.model->SetDissolveParams(enableDissolve, threshold, 0.05f,
                                     Vector3(1.0f, 0.4f, 0.3f));
    }

    // 出現完了するまでは射撃を行わない
    if (enemy.spawnTimer >= 1.0f) {
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
    }

    // オブジェクトの更新
    enemy.object->Update(mainViewIndex_, camera_.get());
  }

  /*if (hitFeedbackTimer_ > 0) {
    hitEffectPlane_->Update(mainViewIndex_, camera_.get());
  }*/

  skybox_->Update(mainViewIndex_, camera_.get());

  // プロジェクタイルの移動・行列更新
  for (auto &p : projectiles_) {
    p->Update();
    p->Update(mainViewIndex_, camera_.get());
  }

  // カメラとの当たり判定（プレイヤー被弾処理）
  if (!isCovering_) {
    for (auto &p : projectiles_) {
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
                     [](const std::unique_ptr<EnemyProjectile> &p) {
                       return p->IsDead();
                     }),
      projectiles_.end());

  // スプライト更新（照準をマウス位置に合わせる）
  POINT mousePos;
  GetCursorPos(&mousePos);
  ScreenToClient(Win32Window::GetInstance()->GetHwnd(), &mousePos);
  crosshair_->SetTranslate({(float)mousePos.x, (float)mousePos.y});
  crosshair_->Update();
  camera_->Update();

  // 射撃（クリック）処理
  if (!isCovering_ && ammo_ > 0 &&
      Input::GetInstance()->IsMouseButtonTriggered(0)) {
    ammo_--; // 弾数を減らす
    float x = (float)mousePos.x / Win32Window::kClientWidth * 2.0f - 1.0f;
    float y = 1.0f - (float)mousePos.y / Win32Window::kClientHeight * 2.0f;
    Matrix4x4 vp = camera_->GetViewProjectionMatrix();
    Matrix4x4 invVP = Inverse(vp);
    Vector3 nearPos = TransformPoint({x, y, 0.0f}, invVP);
    Vector3 farPos = TransformPoint({x, y, 1.0f}, invVP);
    Vector3 rayDir = Normalize(Subtract(farPos, nearPos));

    // エネミーへの当たり判定
    bool hitAny = false;
    Vector3 hitPos = {0.0f, 0.0f, 0.0f};
    for (auto &enemy : enemies_) {
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
          // 敵撃破時にリングパーティクルのUVアニメーション設定を初期化して放出
          if (auto* emitter = ParticleManager::GetInstance()->GetEmitter(ringParticleGroupName_)) {
            emitter->uvAnimationSettings.currentTranslate = { 0.0f, 0.0f };
            emitter->uvAnimationSettings.currentRotate = 0.0f;
            emitter->uvAnimationSettings.currentScale = { 1.0f, 1.0f };
            emitter->isPlaying = true;
          }
          ParticleManager::GetInstance()->Emit(ringParticleGroupName_, enemyPos, 32);
          break; // 1回で1体倒す
        }
      }
    }
  }

  // クリア判定
  if (cameraProgress_ >= maxProgress_) {
    bool allEnemiesDead = true;
    for (const auto &enemy : enemies_) {
      if (!enemy.isDead) {
        allEnemiesDead = false;
        break;
      }
    }
    if (allEnemiesDead) {
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

  // -----------------------------------------------------------------
  // UIスプライトの更新処理
  // -----------------------------------------------------------------
  // 1. ライフUIの更新
  {
    int currentLife = kMaxHits - hitCount_;
    if (currentLife < 0) currentLife = 0;

    for (int i = 0; i < currentLife; ++i) {
      lifeTargetX_[i] = 1214.0f - (currentLife - 1 - i) * 24.0f;
      lifeCurrentX_[i] += (lifeTargetX_[i] - lifeCurrentX_[i]) * 0.15f;
      lifeUnits_[i]->SetTranslate({ lifeCurrentX_[i], 37.0f });
      lifeUnits_[i]->Update();
    }
    lifeBg_->Update();
  }

  // 2. 弾薬UIの更新
  {
    // リロード判定
    bool isReloading = isCovering_ && (ammo_ < kMaxAmmo);

    if (isReloading) {
      // リロード中の補充アニメーション（全体の85%の時間で装填を完了させ、スライドの余裕を作る）
      float t = reloadTimer_ / (kReloadDuration * 0.85f);
      if (t > 1.0f) t = 1.0f;
      int reloadCount = static_cast<int>(t * kMaxAmmo);

      float blinkAlpha = 0.4f + 0.6f * std::abs(std::sin(reloadTimer_ * 20.0f));

      // 新しく装填された弾の現在位置を右端(装填口)にする
      if (reloadCount > lastReloadCount_) {
        for (int i = 9 - reloadCount; i <= 9 - 1 - lastReloadCount_; ++i) {
          if (i >= 0 && i < kMaxAmmo) {
            ammoCurrentX_[i] = 202.0f;
          }
        }
      }
      lastReloadCount_ = reloadCount;

      for (int i = 0; i < kMaxAmmo; ++i) {
        if (i >= 9 - reloadCount) {
          // 装填済みの弾
          ammoTargetX_[i] = 202.0f - (i + reloadCount - 9) * 20.0f;
          ammoCurrentX_[i] += (ammoTargetX_[i] - ammoCurrentX_[i]) * 0.15f;
          ammoUnits_[i]->SetTranslate({ ammoCurrentX_[i], 648.0f });
          ammoUnits_[i]->SetColor({ 183.0f/255.0f, 132.0f/255.0f, 48.0f/255.0f, blinkAlpha });
        } else {
          // まだ装填されていない弾（非表示）
          ammoUnits_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
        }
        ammoUnits_[i]->Update();
      }
    } else {
      // 通常時
      lastReloadCount_ = 0; // 通常時はカウントをリセット

      int currentAmmo = ammo_;
      for (int i = 0; i < kMaxAmmo; ++i) {
        if (i >= 9 - currentAmmo) {
          // 残っている弾
          ammoTargetX_[i] = 202.0f - (i - 9 + currentAmmo) * 20.0f;
          ammoCurrentX_[i] += (ammoTargetX_[i] - ammoCurrentX_[i]) * 0.15f;
          ammoUnits_[i]->SetTranslate({ ammoCurrentX_[i], 648.0f });
          ammoUnits_[i]->SetColor({ 183.0f/255.0f, 132.0f/255.0f, 48.0f/255.0f, 1.0f });
        } else {
          // 消費された弾（非表示）
          ammoUnits_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
        }
        ammoUnits_[i]->Update();
      }
    }
    ammoBg_->Update();
  }

  // 3. プログレスバーの更新
  {
    float progressRatio = cameraProgress_ / maxProgress_;
    if (progressRatio > 1.0f) progressRatio = 1.0f;
    progressBar_->SetScale({ 300.0f * progressRatio, 20.0f });
    progressBar_->Update();
    progressBg_->Update();
  }

  // 4. カバー演出の更新
  if (isCovering_) {
    coverOverlay_->Update();
  }

  // パーティクルの更新
  ParticleManager::GetInstance()->SetIsUpdate(true);
  ParticleManager::GetInstance()->SetUseBillboard(false);
  ParticleManager::GetInstance()->Update(*camera_, kDeltaTime);
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
    ImGui::Text("Covering (SPACE): %s",
                isCovering_ ? "YES (Invincible)" : "NO");
    if (isCovering_ && ammo_ < kMaxAmmo) {
      ImGui::ProgressBar(reloadTimer_ / kReloadDuration, ImVec2(0.0f, 0.0f),
                         "Reloading...");
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
          
          // 個別のモデルインスタンスを生成・ロード
          enemy.model = std::make_unique<Model>();
          enemy.model->Initialize("Resources/Assets/Models/enemy", enemyModel_);
          // 初期状態として完全に消去された状態（Threshold = 1.0f）を設定
          enemy.model->SetDissolveMaskTexture("masks/noise0.png");
          enemy.model->SetDissolveParams(1, 1.0f, 0.05f, Vector3(1.0f, 0.4f, 0.3f));
          
          enemy.object->SetModel(enemy.model.get());
          enemy.object->SetTranslate(enemyData.translation);
          enemy.object->SetRotation(enemyData.rotation);
          enemy.object->SetCamera(camera_.get());
          enemy.distance = enemyData.distance;
          enemy.isActive = false;
          enemy.isDead = false;
          enemy.shootTimer = 0.0f;
          enemy.spawnTimer = 0.0f;
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
    if (hitCount_ >= kMaxHits) {
      ImGui::Text("GAME OVER");
    } else {
      ImGui::Text("CLEAR!");
    }
  } else if (phase_ == Phase::RestartSmoothing) {
    ImGui::Text("Restarting...");
  } else {
    ImGui::ProgressBar(cameraProgress_ / maxProgress_, ImVec2(0.0f, 0.0f),
                       "Progress");
  }
  ImGui::Separator();
  ImGui::Text("Player Status");
  ImGui::Separator();
  ImGui::Text("Hit Count: %d / %d", hitCount_, kMaxHits);
  ImGui::Text("Ammo: %d / %d", ammo_, kMaxAmmo);
  ImGui::Text("Covering (SPACE): %s", isCovering_ ? "YES (Invincible)" : "NO");
  if (isCovering_ && ammo_ < kMaxAmmo) {
    ImGui::ProgressBar(reloadTimer_ / kReloadDuration, ImVec2(0.0f, 0.0f),
                       "Reloading...");
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
    /*if (hitFeedbackTimer_ > 0 && hitEffectPlane_)
      hitEffectPlane_->Draw(0);*/
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

  // パーティクルの描画 (加算合成)
  ParticleManager::GetInstance()->Draw(BlendMode::BlendState::kBlendModeAdd);

  // スプライトの描画
  SpriteCommon::GetInstance()->SetCommonDrawSettings(
      static_cast<BlendState>(currentBlendMode_));

  // 1. カバー演出
  if (isCovering_ && coverOverlay_) {
    coverOverlay_->Draw();
  }

  // 2. UIの下地
  if (lifeBg_) lifeBg_->Draw();
  if (ammoBg_) ammoBg_->Draw();
  if (progressBg_) progressBg_->Draw();

  // 3. UIのゲージ（メモリやバー）
  // ライフメモリ
  {
    int currentLife = kMaxHits - hitCount_;
    if (currentLife < 0) currentLife = 0;
    for (int i = 0; i < currentLife; ++i) {
      if (lifeUnits_[i]) {
        lifeUnits_[i]->Draw();
      }
    }
  }

  // 弾薬メモリ
  {
    for (int i = 0; i < kMaxAmmo; ++i) {
      if (ammoUnits_[i]) {
        ammoUnits_[i]->Draw();
      }
    }
  }

  // プログレスバー
  if (progressBar_) {
    progressBar_->Draw();
  }

  // 4. 照準
  if (isShowSprite_) {
    if (crosshair_)
      crosshair_->Draw();
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
  pos.x = uuu * p0.x + 3.0f * uu * t * p1.x + 3.0f * u * tt * p2.x + ttt * p3.x;
  pos.y = uuu * p0.y + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.y + ttt * p3.y;
  pos.z = uuu * p0.z + 3.0f * uu * t * p1.z + 3.0f * u * tt * p2.z + ttt * p3.z;
  return pos;
}