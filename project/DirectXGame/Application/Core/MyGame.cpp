#include "MyGame.h"

// フレームワーク基盤
#include "RAFramework.h"
// エンジン
#include "DX12Context.h"
//#include "Input.h"
// 管理系
#include "SrvManager.h"
#include "ImGuiManager.h"
#include "ParticleManager.h"
#include "TextureManager.h"
#include "LightManager.h"
#include "SceneManager.h"
// Scene
#include "SceneFactory.h"

// .lidはヘッダに書いてはいけない
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace BlendMode;

void MyGame::Initialize() {
  // --- 基盤システムの初期化は親クラスに任せる ---
  RAFramework::Initialize();

  // ファクトリーを作成し、SceneManagerにセット
  auto sceneFactory = std::make_unique<SceneFactory>();
  SceneManager::GetInstance()->SetSceneFactory(std::move(sceneFactory));

  // 文字列で指定
  SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
  //SceneManager::GetInstance()->ChangeScene("TITLE");
}

void MyGame::Finalize() {
  // --- 基盤システムの終了処理は親クラスに任せる ---
  RAFramework::Finalize();
}

void MyGame::Update() {
    

    // ImGuiの受付開始
    imGuiManager_->Begin();

    // シーンマネージャに現在のシーンを更新させる
    SceneManager::GetInstance()->Update();

    // ImGuiの受付終了 (内部的な終了処理)
    imGuiManager_->End();

    // ライト更新
    LightManager::GetInstance()->Update();
}

void MyGame::Draw() {
    // 1. 描画前処理
    DX12Context::GetInstance()->PreDraw();
    SrvManager::GetInstance()->PreDraw(); // SRV用のヒープをセット

    // シーンマネージャに現在のシーンを描画させる
    SceneManager::GetInstance()->Draw();

    // 5. ImGuiの描画 (Updateで作られたUIを描画コマンドに乗せる)
    imGuiManager_->Draw();

    // 6. 描画後処理 (画面の入れ替え)
    DX12Context::GetInstance()->PostDraw();
}
