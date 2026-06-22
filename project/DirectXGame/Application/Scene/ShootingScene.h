#pragma once
#include "BaseScene.h"
#include "Camera.h"
#include "LevelLoader.h"
#include "Object3d.h"
#include "Skybox.h"
#include "Sprite.h"
#include <memory>
#include <vector>

#include "EnemyProjectile.h"
#include "PostProcessManager.h"
#include "RenderTexture.h"

class Model;

class ShootingScene : public BaseScene {
public:
  ShootingScene();
  ~ShootingScene() override;

  void Initialize() override;
  void Update() override;

  void Draw() override;
  void Finalize() override;

  // ゲームオーバー演出フェーズ
  enum class Phase {
    Playing,          // 通常プレイ
    GameOverVignette, // ビネットで覆っていく
    GameOverWait,     // 暗転維持
    RestartSmoothing, // リセット後スムージングを徐々に消す
  };

private:
  // シーン描画のヘルパー
  void DrawScene(Camera *camera);

#ifdef USE_IMGUI
  // ImGui操作の更新
  void UpdateImGui();
  // ImGuiでグローバル設定のパラメータを調整するための関数
  void UpdateImGui_GlobalSettings();
  // ImGuiでゲームカメラのパラメータを調整するための関数
  void UpdateImGui_GameCamera();
  // ImGuiでObject3dのパラメータを調整するための関数
  void UpdateImGui_Object3d();
  // ImGuiでSkyboxのパラメータを調整するための関数
  void UpdateImGui_Skybox();
  // ImGuiでゲームの状態を確認するための関数
  void UpdateImGui_GameStatus();
#endif // USE_IMGUI

private:
  // カメラ
  std::unique_ptr<Camera> camera_;

  // スカイボックス
  std::unique_ptr<Skybox> skybox_;

  struct EnemyInfo {
    std::unique_ptr<Object3d> object;
    std::unique_ptr<Model> model; // 個別モデル
    Vector3 basePosition;
    float distance = 0.0f;
    bool isActive = false;
    bool isDead = false;
    float shootTimer = 0.0f;
    float spawnTimer = 0.0f; // 出現タイマー
  };

  // レベルから読み取った敵オブジェクト群
  std::vector<EnemyInfo> enemies_;

  // 照準（スプライト）
  std::unique_ptr<Sprite> crosshair_;

  // オフスクリーン用分岐戦闘の際には記述する

  // ビューのインデックス（0: Main）
  int mainViewIndex_ = 0;

  // ゲームロジック用変数
  float targetTimer_ = 0.0f;
  float orbitAngle_ = 0.0f;
  const float orbitRadius_ = 15.0f;
  Vector3 targetBasePos_ = {0.0f, 2.0f, 10.0f};
  bool isHit_ = false;
  float hitFeedbackTimer_ = 0.0f;
  float cameraYaw_ = 0.0f; // カメラのY軸回転（ラジアン、90度ずつ変化）

  // パーティクル設定など
  int currentBlendMode_ = 1; // NormalBlend
  bool isShowMaterial_ = true;
  bool isShowSprite_ = true;
  bool isShowSkybox_ = true;

  // プロジェクタイル管理
  std::vector<std::unique_ptr<EnemyProjectile>> projectiles_;
  float projectileSpawnTimer_ = 0.0f;
  const float kProjectileSpawnInterval = 120.0f; // 2秒おき

  // ダメージ効果（グレースケール）
  float damageEffectStrength_ = 0.0f;

  // ヒットポイントとゲームオーバー管理
  int hitCount_ = 0;
  static constexpr int kMaxHits = 3;

  Phase phase_ = Phase::Playing;
  float phaseTimer_ = 0.0f;

  // ビネット演出パラメータ（徐々に強化）
  float vignetteScale_ = 16.0f;                      // 小さいほど暗くなる
  float vignetteExponent_ = 0.8f;                    // 大きいほど急激に暗くなる
  static constexpr float kVignetteInDuration = 1.5f; // 覆うのにかかる時間(秒)
  static constexpr float kGameOverWaitDuration = 3.0f; // 暗転維持時間(秒)

  // スムージング演出パラメータ
  float smoothingKernel_ = 31.0f;                      // 最大カーネルサイズ
  static constexpr float kSmoothingOutDuration = 2.0f; // フェードアウト時間(秒)
  static constexpr float kDeltaTime = 1.0f / 60.0f;

  // パス定数
  // 3Dモデルのファイルパス
  const std::string enemyModel_ = "enemy.obj";
  // テクスチャファイルパスを保持
  const std::string crosshairPath_ = "crosshair.png";
  const std::string ringParticleGroupName_ = "RingShapeGroup";

  // レベルデータ
  std::unique_ptr<LevelData> levelData_;

  // レール移動用
  std::vector<LevelData::BezierControlPoint> railPoints_;
  float cameraProgress_ = 0.0f;
  float maxProgress_ = 190.0f;
  bool isMovementPaused_ = false;
  const float kCameraSpeed = 0.5f;

  // マガジン・隠れるアクション用
  int ammo_ = 9;
  static constexpr int kMaxAmmo = 9;
  bool isCovering_ = false;
  float reloadTimer_ = 0.0f;
  static constexpr float kReloadDuration = 1.0f;

  // レール座標計算用ヘルパー関数
  Vector3 CalculateRailPosition(float progress);

  // UI用スプライト
  // ライフUI
  std::unique_ptr<Sprite> lifeBg_;
  std::vector<std::unique_ptr<Sprite>> lifeUnits_;
  std::vector<float> lifeCurrentX_;
  std::vector<float> lifeTargetX_;

  // 弾薬UI
  std::unique_ptr<Sprite> ammoBg_;
  std::vector<std::unique_ptr<Sprite>> ammoUnits_;
  std::vector<float> ammoCurrentX_;
  std::vector<float> ammoTargetX_;
  int lastReloadCount_ = 0;

  // プログレスバーUI
  std::unique_ptr<Sprite> progressBg_;
  std::unique_ptr<Sprite> progressBar_;

  // カバー演出UI
  std::unique_ptr<Sprite> coverOverlay_;
};
