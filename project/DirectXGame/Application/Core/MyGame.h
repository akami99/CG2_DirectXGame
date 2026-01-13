#pragma once
#include <Windows.h>
#include <cassert>
#include <dbghelp.h> // dump
#include <dxgi1_6.h> // dump
#include <strsafe.h> // dump

// #include <vector>
#include <numbers> //particleに使用
#include <random>  //particleに使用

// エンジン
#include "Camera.h"
#include "D3DResourceLeakChecker.h"
#include "DX12Context.h"
#include "DebugCamera.h"
#include "Input.h"
#include "Logger.h"
#include "Model.h"
#include "ModelCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "ParticleEmitter.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Win32Window.h"
// 管理系
#include "AudioManager.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "PipelineManager.h"
#include "SrvManager.h"
#include "TextureManager.h"

// グラフィック関連の構造体
// #include "GraphicsTypes.h"
#include "LightTypes.h"
// #include "ModelTypes.h"
#include "ParticleTypes.h"

// 計算用関数など
#include "MathTypes.h"
#include "MathUtils.h"
#include "MatrixGenerators.h"

// ゲーム内設定用
#include "ApplicationConfig.h"

class MyGame {
public: // メンバ関数
  void Initialize();
  void Run();
  void Finalize();

private: // メンバ関数
  void Update();
  void Draw();

private: // メンバ変数(エンジン系)
  // --- エンジン基盤 (new/delete で管理するもの) ---
  Win32Window *window_ = nullptr;
  DX12Context *dxBase_ = nullptr;
  SrvManager *srvManager_ = nullptr;
  Input *input_ = nullptr;
  PipelineManager *pipelineManager_ = nullptr;
  ImGuiManager *imGuiManager_ = nullptr;

  // --- シングルトン系 (GetInstanceをキャッシュしておく) ---
  TextureManager *textureManager_ = nullptr;
  ModelManager *modelManager_ = nullptr;
  ParticleManager *particleManager_ = nullptr;

  // --- 音声管理 ---
  AudioManager audioManager_; // これは実体なのでそのまま

  // --- 描画共通系 ---
  SpriteCommon *spriteCommon_ = nullptr;
  Object3dCommon *object3dCommon_ = nullptr;

private: // メンバ変数(ゲーム内)
  // --- ゲームオブジェクト ---
  DebugCamera debugCamera_;
  Camera *camera_ = nullptr;
  std::vector<Object3d *> object3ds_;
  std::vector<Sprite *> sprites_;

  // --- ゲーム内変数 (フラグや設定値) ---
  Vector3 gameCameraRotate_;
  Vector3 gameCameraTranslate_;
  bool useDebugCamera_ = false;


  Vector3 particleRotation{};
  bool isUpdateParticle_ = true;
  bool useBillboard_ = true;
  
  // ブレンドモード
  int currentBlendMode_ = BlendMode::BlendState::kBlendModeNormal;
  int particleBlendMode_ = BlendMode::BlendState::kBlendModeAdd;

  // 表示
  bool showMaterial = true;
  bool showSprite = false;

  // モデルのパスを保持
  const std::string planeModel_ = ("plane.obj");
  // const std::string axisModel_ = ("axis.obj");
  const std::string teapotModel_ = ("teapot.obj");

  // パーティクルグループの作成
  const std::string particleTexturePath_ = "circle.png";
  const std::string particleGroupName_ = "default";
  // エミッターの設定と登録
  ParticleEmitter defaultEmitter_ = ParticleEmitter(
      Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}},
      3,   // count: 1回で3個生成
      0.2f // frequency: 0.2秒ごとに発生
  );

  // テクスチャファイルパスを保持
  const std::string uvCheckerPath_ = "uvChecker.png";
  const std::string monsterBallPath_ = "monsterBall.png";
};
