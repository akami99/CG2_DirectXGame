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

// 静的メンバ変数の実体化
int MyGame::postEffectModeNext_ = 0;
float MyGame::postEffectStrengthNext_ = 1.0f;

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
  postProcessPSO_  = PipelineManager::GetInstance()->CreatePostProcessPSO();  // CopyImage (passthrough)
  colorFilterPSO_  = PipelineManager::GetInstance()->CreateColorFilterPSO();  // グレースケール/セピア
  
  // ポストエフェクト用定数バッファの生成
  postProcessParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(PostProcessParams));
  postProcessParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&postProcessParamsData_));
  // デフォルト: コピー(無効果) → white (1,1,1)
  postProcessParamsData_->colorScale[0] = 1.0f;
  postProcessParamsData_->colorScale[1] = 1.0f;
  postProcessParamsData_->colorScale[2] = 1.0f;
  postProcessParamsData_->strength = 1.0f;
  
  postEffectModeNext_ = 0;
  postEffectStrengthNext_ = 1.0f;

  // 追加した初期化コマンド（テクスチャ転送等）を実行し、GPUの完了を待つ
  DX12Context::GetInstance()->ExecuteInitialCommandAndSync();

  // ファクトリーを作成し、SceneManagerにセット
  auto sceneFactory = std::make_unique<SceneFactory>();
  SceneManager::GetInstance()->SetSceneFactory(std::move(sceneFactory));

  // 文字列で指定(TITLE/GAMEPLAY/SHOOTINGなど)してシーン切り替え予約
  SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
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

    // ライト更新
    LightManager::GetInstance()->Update();

#ifdef USE_IMGUI
    // ポストエフェクトモード選択ImGui
    ImGui::SetNextWindowPos(ImVec2(10.0f, 210.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(280.0f, 100.0f), ImGuiCond_Once);
    ImGui::Begin("PostEffect");
    ImGui::Combo("Mode", &postEffectMode_,
        "Copy (None)\0GrayScale\0Sepia\0");
    ImGui::End();
#endif

    // ImGuiの受付終了 (内部的な終了処理)
    imGuiManager_->End();
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

    // 共通のルートシグネチャをセット (CopyImage / ColorFilter 両方同じものを使用)
    commandList->SetGraphicsRootSignature(PipelineManager::GetInstance()->GetPostProcessRootSignature());

    // テクスチャ（オフスクリーン描画結果）をSRVスロット(t0)にセット
    SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(1, renderTexture_->GetSrvIndex());

    // モードと強度を更新
    postEffectMode_ = postEffectModeNext_;
    
    // モードに応じてPSOと定数バッファの値を切り替える
    if (postEffectMode_ == 0) {
        // コピー (passthrough) — CopyImage.PS.hlsl は定数バッファ不要
        commandList->SetPipelineState(postProcessPSO_.Get());
        // b0 はシェーダーが参照しないが、バリデーション用に一応セット
        commandList->SetGraphicsRootConstantBufferView(0, postProcessParamsResource_->GetGPUVirtualAddress());
    } else {
        // グレースケール or セピア — Grayscale.PS.hlsl (b0 = ColorFilter CBV 必須)
        commandList->SetPipelineState(colorFilterPSO_.Get());

        if (postEffectMode_ == 1) {
            // グレースケール: colorScale = (1, 1, 1)
            postProcessParamsData_->colorScale[0] = 1.0f;
            postProcessParamsData_->colorScale[1] = 1.0f;
            postProcessParamsData_->colorScale[2] = 1.0f;
        } else { // mode == 2
            // セピア: RGB(112, 74, 43) を最大値112で正規化
            postProcessParamsData_->colorScale[0] = 1.0f;
            postProcessParamsData_->colorScale[1] = 74.0f / 112.0f;  // ~0.661
            postProcessParamsData_->colorScale[2] = 43.0f / 112.0f;  // ~0.384
        }
        postProcessParamsData_->strength = postEffectStrengthNext_;
        commandList->SetGraphicsRootConstantBufferView(0, postProcessParamsResource_->GetGPUVirtualAddress());
    }

    // 全画面三角形の描画 (頂点バッファは使わず、Shader内のSV_VertexIDで生成)
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    // 5. ImGuiの描画 (Updateで作られたUIを描画コマンドに乗せる)
    imGuiManager_->Draw();

    // 6. 描画後処理 (画面の入れ替え)
    DX12Context::GetInstance()->PostDraw();
}
