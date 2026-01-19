#pragma once
#include "RAFramework.h" // RAFrameworkをインクルード

// 必要な前方宣言やインクルード
#include "AudioManager.h"
#include "DebugCamera.h"
#include "MathTypes.h"
#include <vector>

class Camera;
class Object3d;
class Sprite;
class LightManager;

// Frameworkを継承する
class MyGame : public RAFramework {
public: // メンバ関数
    // オーバーライドすることを明示
    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;

private: // メンバ変数(ゲーム内)
    // --- ゲーム固有の変数だけを残す ---

    // カメラ
  Camera *camera_ = nullptr;
  DebugCamera debugCamera_;
  bool useDebugCamera_ = false;
  Vector3 gameCameraRotate_{};
  Vector3 gameCameraTranslate_{};

  // ライト
  LightManager *lightManager_ = nullptr;

  // ゲームオブジェクト
  std::vector<Object3d *> object3ds_;
  std::vector<Sprite *> sprites_;

  // 音声
  AudioManager audioManager_; 

  // パーティクル設定など
  Vector3 particleRotation_{};
  bool isUpdateParticle_ = true;
  bool useBillboard_ = true;
  int currentBlendMode_ = 1;  // NormalBlend
  int particleBlendMode_ = 2; // AddBlend
  bool showMaterial_ = true;
  bool showSprite_ = false;

  /// ファイルパスなどの定数
  // 3Dモデルのファイルパス
  const std::string planeModel_ = ("plane.obj");
  const std::string teapotModel_ = ("teapot.obj");
  const std::string terrainModel_ = ("terrain.obj");
  // テクスチャファイルパスを保持
  const std::string uvCheckerPath_ = "uvChecker.png";
  const std::string monsterBallPath_ = "monsterBall.png";
  const std::string grassPath_ = "grass.png";
  // パーティクルグループの作成
  const std::string particleTexturePath_ = "circle.png";
  const std::string particleGroupName_ = "default";

};
