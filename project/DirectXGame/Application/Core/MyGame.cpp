#include "MyGame.h"

// フレームワーク基盤
#include "RAFramework.h"
// エンジン
#include "DX12Context.h"
//#include "Input.h"
// 管理系
#include "SrvManager.h"
#include "PipelineManager.h"
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

  // 閉じているコマンドリストを再度開く（テクスチャロード等のために必要）
  DX12Context::GetInstance()->GetCommandList()->Reset(
      DX12Context::GetInstance()->GetCommandAllocator(), nullptr);

  // オフスクリーンレンダリング用のリソース初期化
  renderTexture_ = std::make_unique<RenderTexture>();
  renderTexture_->Initialize(1280, 720);

  // 後の初期化でメタデータを参照するため、先にロードしておく
  TextureManager::GetInstance()->LoadTexture("uvChecker.png");


  
  // ポストエフェクト用PSOの生成
  postProcessPSO_ = PipelineManager::GetInstance()->CreatePostProcessPSO();
  
  // ポストエフェクト用定数バッファの生成
  postProcessParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(PostProcessParams));
  postProcessParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&postProcessParamsData_));
  postProcessParamsData_->intensity = 0.5f; // デフォルト値

  // 追加した初期化コマンド（テクスチャ転送等）を実行し、GPUの完了を待つ
  DX12Context::GetInstance()->ExecuteInitialCommandAndSync();

  // ファクトリーを作成し、SceneManagerにセット
  auto sceneFactory = std::make_unique<SceneFactory>();
  SceneManager::GetInstance()->SetSceneFactory(std::move(sceneFactory));

  // 文字列で指定
  SceneManager::GetInstance()->ChangeScene("SHOOTING");
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
    ID3D12GraphicsCommandList* commandList = DX12Context::GetInstance()->GetCommandList();

    // 1.描画前処理（コマンドリストのリセット、バックバッファをセット）
    DX12Context::GetInstance()->PreDraw();
    SrvManager::GetInstance()->PreDraw(); // SRV用のヒープをセット

    // --- A. オフスクリーンレンダリングパス ---
    // シーン独自のオフスクリーン描画があれば実行
    SceneManager::GetInstance()->DrawOffscreen();

    renderTexture_->PreDraw(commandList);
    // シーンマネージャに現在のシーンを描画させる（オフスクリーンへ）
    SceneManager::GetInstance()->Draw();
	// オフスクリーン描画の終了処理
    renderTexture_->PostDraw(commandList);

    // --- 2. メインレンダリングパス（バックバッファへ戻る） ---
    // レンダーターゲットをバックバッファにセットし直す
    DX12Context::GetInstance()->SetBackBufferAsRenderTarget();
    SrvManager::GetInstance()->PreDraw(); // SRV用のヒープをセット

    // ポストエフェクト用パイプラインをセット
    commandList->SetGraphicsRootSignature(PipelineManager::GetInstance()->GetPostProcessRootSignature());
    commandList->SetPipelineState(postProcessPSO_.Get());
    
    // 定数バッファをセット
    commandList->SetGraphicsRootConstantBufferView(0, postProcessParamsResource_->GetGPUVirtualAddress());
    
    // テクスチャ（オフスクリーン描画結果）をセット
    SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(1, renderTexture_->GetSrvIndex());

    // 全画面三角形の描画 (頂点バッファは使わず、Shader内のSV_VertexIDで生成)
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    // 5. ImGuiの描画 (Updateで作られたUIを描画コマンドに乗せる)
    imGuiManager_->Draw();

    // 6. 描画後処理 (画面の入れ替え)
    DX12Context::GetInstance()->PostDraw();
}
