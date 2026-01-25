#include "MyGame.h"

#include <cassert>

#include <numbers> //particleに使用
#include <random>  //particleに使用

// フレームワーク基盤
#include "RAFramework.h"
// エンジン
#include "Camera.h"
#include "DX12Context.h"
#include "Model.h"
#include "Object3d.h"
#include "Sprite.h"
// 共通系
#include "Object3dCommon.h"
// 管理系
#include "ImGuiManager.h"
#include "LightManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "SrvManager.h"
#include "TextureManager.h"

// グラフィック関連の構造体
#include "LightTypes.h"
#include "ParticleTypes.h"

// 計算用関数など
#include "MathUtils.h"
#include "MatrixGenerators.h"

// ゲーム内設定用
#include "ApplicationConfig.h"

// Scene
#include "GamePlayScene.h"

// .lidはヘッダに書いてはいけない
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace MathUtils;
using namespace MathGenerators;
using namespace BlendMode;

void MyGame::Initialize() {
  // --- 基盤システムの初期化は親クラスに任せる ---
  RAFramework::Initialize();

  // --- ゲーム固有の初期化処理 ---
  gamePlayScene_ = new GamePlayScene();
  gamePlayScene_->Initialize();

  // コマンド実行と完了待機
  DX12Context::GetInstance()->ExecuteInitialCommandAndSync();
  TextureManager::GetInstance()->ReleaseIntermediateResources();
  ParticleManager::GetInstance()->ReleaseIntermediateResources();
}

void MyGame::Finalize() {
  // --- 終了処理 ---
  // ゲーム固有の終了処理
  if (gamePlayScene_) {
    gamePlayScene_->Finalize();
    delete gamePlayScene_;
    gamePlayScene_ = nullptr;
  }

  // --- 基盤システムの終了処理は親クラスに任せる ---
  RAFramework::Finalize();
}

void MyGame::Update() {
  // 1. ImGuiの受付開始
  imGuiManager_->Begin();

  // 3. ゲームプレイシーンの更新
  gamePlayScene_->Update();

  // 5. ImGuiの受付終了 (内部的な終了処理)
  imGuiManager_->End();
}

void MyGame::Draw() {
  // 1. 描画前処理
  DX12Context::GetInstance()->PreDraw();
  SrvManager::GetInstance()->PreDraw(); // SRV用のヒープをセット

  // 2. ゲームプレイシーンの描画
  gamePlayScene_->Draw();

  // 5. ImGuiの描画 (Updateで作られたUIを描画コマンドに乗せる)
  imGuiManager_->Draw();

  // 6. 描画後処理 (画面の入れ替え)
  DX12Context::GetInstance()->PostDraw();
}
