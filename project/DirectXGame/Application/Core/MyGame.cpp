#include "MyGame.h"

// フレームワーク基盤
#include "RAFramework.h"
// エンジン
#include "DX12Context.h"
// 管理系
#include "SrvManager.h"
#include "PipelineManager.h"
#include "PostProcessManager.h"
#include "ImGuiManager.h"
#include "ParticleManager.h"
#include "TextureManager.h"
#include "LightManager.h"
#include "SceneManager.h"
// Scene
#include "SceneFactory.h"

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

void MyGame::Initialize() {
	RAFramework::Initialize();

	// 閉じているコマンドリストを再度開く（テクスチャロード等のために必要）
	DX12Context::GetInstance()->GetCommandList()->Reset(
		DX12Context::GetInstance()->GetCommandAllocator(), nullptr);

	// オフスクリーンレンダリング用のリソース初期化
	renderTexture_ = std::make_unique<RenderTexture>();
	renderTexture_->Initialize(1280, 720);

	// 後の初期化でメタデータを参照するため、先にロードしておく
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");

	// ポストプロセスマネージャの初期化
	PostProcessManager::GetInstance()->Initialize();

	// 追加した初期化コマンド（テクスチャ転送等）を実行し、GPUの完了を待つ
	DX12Context::GetInstance()->ExecuteInitialCommandAndSync();

	// ファクトリーを作成し、SceneManagerにセット
	auto sceneFactory = std::make_unique<SceneFactory>();
	SceneManager::GetInstance()->SetSceneFactory(std::move(sceneFactory));

	// 文字列で指定(TITLE/GAMEPLAY/SHOOTINGなど)してシーン切り替え予約
	SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
}

void MyGame::Finalize() {
	PostProcessManager::Destroy();
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
	// ポストエフェクトモード選択 ImGui
	ImGui::SetNextWindowPos(ImVec2(10.0f, 210.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(280.0f, 150.0f), ImGuiCond_Once);
	ImGui::Begin("PostEffect");
	int mode = PostProcessManager::GetInstance()->GetCurrentMode();
	if (ImGui::Combo("Mode", &mode, "Copy (None)\0GrayScale\0Sepia\0Vignette\0Smoothing\0Gaussian Blur\0")) {
		PostProcessManager::SetMode(mode);
	}
	if (mode == PostProcessManager::kModeVignette) {
		static float scale = 16.0f;
		static float exponent = 0.8f;
		bool changed = false;
		if (ImGui::SliderFloat("Vignette Scale", &scale, 0.0f, 32.0f)) {
			changed = true;
		}
		if (ImGui::SliderFloat("Vignette Exponent", &exponent, 0.0f, 5.0f)) {
			changed = true;
		}
		if (changed) {
			PostProcessManager::SetVignetteParams(scale, exponent);
		}
	} else if (mode == PostProcessManager::kModeSmoothing) {
		static int kernelSize = 3;
		if (ImGui::SliderInt("Kernel Size", &kernelSize, 3, 15)) {
			// 偶数なら奇数に補正する
			if (kernelSize % 2 == 0) {
				kernelSize += 1;
			}
			PostProcessManager::SetSmoothingParams(kernelSize);
		}
	} else if (mode == PostProcessManager::kModeGaussianBlur) {
		static int kernelSize = 3;
		static float sigma = 2.0f;
		bool changed = false;
		if (ImGui::SliderInt("Kernel Size", &kernelSize, 3, 15)) {
			if (kernelSize % 2 == 0) {
				kernelSize += 1;
			}
			changed = true;
		}
		if (ImGui::SliderFloat("Sigma", &sigma, 0.1f, 10.0f)) {
			changed = true;
		}
		if (changed) {
			PostProcessManager::SetGaussianBlurParams(kernelSize, sigma);
		}
	}
	ImGui::End();
#endif

	imGuiManager_->End();
}

void MyGame::Draw() {
	ID3D12GraphicsCommandList* commandList = DX12Context::GetInstance()->GetCommandList();

	// 描画前処理（コマンドリストのリセット、バックバッファをセット）
	DX12Context::GetInstance()->PreDraw();
	SrvManager::GetInstance()->PreDraw();

	// --- オフスクリーンレンダリングパス ---
	SceneManager::GetInstance()->DrawOffscreen();
	renderTexture_->PreDraw(commandList);
	// シーンマネージャに現在のシーンを描画させる（オフスクリーンへ）
	SceneManager::GetInstance()->Draw();
	// オフスクリーン描画の終了処理
	renderTexture_->PostDraw(commandList);

	// --- メインバックバッファへのポストエフェクト描画 ---
	DX12Context::GetInstance()->SetBackBufferAsRenderTarget();
	SrvManager::GetInstance()->PreDraw();
	PostProcessManager::GetInstance()->Draw(renderTexture_.get());

	// ImGuiの描画
	imGuiManager_->Draw();

	// 描画後処理
	DX12Context::GetInstance()->PostDraw();
}