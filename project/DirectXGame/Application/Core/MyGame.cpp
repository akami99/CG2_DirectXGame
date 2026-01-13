#include "MyGame.h"

// .lidはヘッダに書いてはいけない
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace MathUtils;
using namespace MathGenerators;
using namespace BlendMode;

void MyGame::Initialize() {
  // --- 基盤の初期化 ---

  // Window
  window_ = new Win32Window();
  window_->Initialize();
  // DirectXBase
  dxBase_ = new DX12Context();
  dxBase_->Initialize(window_);
  // SRVManager
  srvManager_ = new SrvManager();
  srvManager_->Initialize(dxBase_);
  // DirectInput
  input_ = new Input();
  input_->Initialize(window_);
  // PipelineManager
  pipelineManager_ = new PipelineManager();
  pipelineManager_->Initialize(dxBase_);
  // ImGuiManager
  imGuiManager_ = new ImGuiManager();
  imGuiManager_->Initialize(dxBase_, window_, srvManager_);

  // --- シングルトンのインスタンスを取得して保持 ---

  // TextureManager
  textureManager_ = TextureManager::GetInstance();
  textureManager_->Initialize(dxBase_, srvManager_);
  // ModelManager
  modelManager_ = ModelManager::GetInstance();
  modelManager_->Initialize(dxBase_);
  // ParticleManager
  particleManager_ = ParticleManager::GetInstance();
  particleManager_->Initialize(dxBase_, srvManager_, pipelineManager_);

  // --- 音声系の初期化 ---
  audioManager_.Initialize();

  // --- 描画共通部の初期化 ---

  // SpriteCommon
  spriteCommon_ = new SpriteCommon();
  spriteCommon_->Initialize(dxBase_, pipelineManager_);

  // Object3dCommon
  object3dCommon_ = new Object3dCommon();
  object3dCommon_->Initialize(dxBase_, pipelineManager_);

  // --- ゲーム内オブジェクトの初期化 ---

  // カメラ
  camera_ = new Camera();
  camera_->Initialize(dxBase_);
  camera_->SetRotate({0.0f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 0.0f, -10.0f});
  object3dCommon_->SetDefaultCamera(camera_);

  gameCameraRotate_ = camera_->GetRotate();
  gameCameraTranslate_ = camera_->GetTranslate();

  // .objファイルからモデルを呼び込む
  modelManager_->LoadModel("Plane", planeModel_);
  // modelManager_->LoadModel("Axis", axisModel_);
  modelManager_->LoadModel("Teapot", teapotModel_);

  // パーティクルグループの作成
  textureManager_->LoadTexture(particleTexturePath_);
  particleManager_->CreateParticleGroup(particleGroupName_,
                                        particleTexturePath_);
  particleManager_->SetEmitter(particleGroupName_, defaultEmitter_);

  // テクスチャファイルパスを保持
  textureManager_->LoadTexture(uvCheckerPath_);

  textureManager_->LoadTexture(monsterBallPath_);

  // コマンド実行と完了待機
  dxBase_->ExecuteInitialCommandAndSync();
  textureManager_->ReleaseIntermediateResources();
  particleManager_->ReleaseIntermediateResources();

  // AudioManager を初期化
  if (!audioManager_.Initialize()) {
      // 初期化失敗時の処理
    assert(false && "AudioManager is not Initialize");
  }

  // 音声ファイルをロード
  if (!audioManager_.LoadSound("alarm1", "Alarm01.mp3")) {
      // ロード失敗時の処理
    assert(false && "Failed to load sound: Alarm01.mp3");
  }

  // マテリアル

  // 1つ目のオブジェクト
  Object3d *object3d_1 = new Object3d(); // object3dをobject3d_1に変更
  object3d_1->Initialize(object3dCommon_);

  // モデルの設定
  object3d_1->SetModel(planeModel_);
  // オブジェクトの位置を設定
  object3d_1->SetTranslate({ -2.0f, 0.0f, 0.0f });

  // 2つ目のオブジェクトを新規作成
  Object3d *object3d_2 = new Object3d();
  object3d_2->Initialize(object3dCommon_);

  // モデルの設定
  object3d_2->SetModel(teapotModel_);
  // オブジェクトの位置を設定
  object3d_2->SetTranslate({ 2.0f, 0.0f, 0.0f });

  // Object3dを格納する
  object3ds_.push_back(object3d_1);
  object3ds_.push_back(object3d_2);

  // 描画サイズ
  const float spriteCutSize = 64.0f;
  const float spriteScale = 64.0f;
  // 生成
  for (uint32_t i = 0; i < 10; ++i) {
      Sprite *newSprite = new Sprite();
      newSprite->Initialize(spriteCommon_, uvCheckerPath_);
      newSprite->SetAnchorPoint({ 0.5f, 0.5f });
      if (i == 7) {
          newSprite->SetFlipX(1);
          newSprite->SetFlipY(1);
      } else if (i == 8) {
          newSprite->SetFlipX(1);
          newSprite->SetTextureLeftTop({ 0.0f, 0.0f });
          newSprite->SetTextureSize({ spriteCutSize, spriteCutSize });
          newSprite->SetScale({ spriteScale, spriteScale });
      } else if (i == 9) {
          newSprite->SetTextureLeftTop({ 0.0f, 0.0f });
          newSprite->SetTextureSize({ spriteCutSize, spriteCutSize });
          newSprite->SetScale({ spriteScale, spriteScale });
      }
      newSprite->SetTranslate(
          { float(i * spriteScale / 3), float(i * spriteScale / 3) });
      sprites_.push_back(newSprite);
  }
}

void MyGame::Run() {}

void MyGame::Finalize() {
  // --- 終了処理 ---

  // 各マネージャーの終了
  particleManager_->Finalize();
  modelManager_->Finalize();
  textureManager_->Finalize();
  imGuiManager_->Finalize();

  // 個別オブジェクトの解放
  for (auto s : sprites_) {
    delete s;
  }
  for (auto o : object3ds_) {
    delete o;
  }
  delete camera_;

  // 基盤の解放 (Initializeの逆順)
  delete spriteCommon_;
  delete object3dCommon_;
  delete imGuiManager_;
  delete pipelineManager_;
  delete input_;
  delete srvManager_;
  delete dxBase_;
  delete window_;
}
