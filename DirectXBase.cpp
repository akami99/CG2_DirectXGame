#include <cassert>
#include <format>

#include "DirectXBase.h"
#include "Logger.h"
#include "StringUtility.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"

// .lidはヘッダに書いてはいけない
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;
using namespace Logger;
using namespace StringUtility;

#pragma region publicメンバ関数

// 初期化
void DirectXBase::Initialize(WindowWrapper* window) {
	// NULLポインタチェック
	assert(window);

	// メンバ変数にウィンドウをセット
	window_ = window;

	// デバイスの生成
	InitializeDevice();

	// コマンド関連の生成
	InitializeCommand();

	// スワップチェーンの生成
	CreateSwapChain();

	// 深度バッファの生成
	CreateDepthBuffer();

	// 各種デスクリプタヒープの生成
	CreateDescriptorHeaps();

	// レンダーターゲットビューの初期化
	InitializeRenderTargetView();

	// 深度ステンシルビューの初期化
	InitializeDepthStencilView();

	// フェンスの生成
	CreateFence();

	// ビューポート矩形の初期化
	InitializeViewportRect();

	// シザー矩形の初期化
	InitializeScissorRect();

	// DCXコンパイラの生成
	CreateDXCCompiler();

	// ImGuiの初期化
	InitializeImGui();
}

#pragma region 描画処理

// 描画開始前処理
void DirectXBase::PreDraw() {

	// バックバッファのインデックスを取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	// バリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	// 遷移前（現在）のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	// 遷移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// TransitionBarrierを張る
	commandList_->ResourceBarrier(1, &barrier);

	// 描画先のRTSとDSVを設定する
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle_);
	
	// 指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f }; // 青っぽい色。RGBAの順
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);
	
	// 指定した深度で画面全体をクリアする
	commandList_->ClearDepthStencilView(dsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 描画用のDescriptorHeapの設定
	// "生のポインタ配列"を作成(ComPtrだとスマートポインタなので変えないように！)
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
	
	// 生のポインタ配列をSetDescriptorHeapsに渡す
	commandList_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// ビューポート領域を設定
	commandList_->RSSetViewports(1, &viewport_);
	
	// シザー矩形を設定
	commandList_->RSSetScissorRects(1, &scissorRect_);
}

// 描画終了後処理
void DirectXBase::PostDraw() {
	HRESULT hr;

	// バックバッファのインデックスを取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	// バリアの共通設定(preDrawと同じ)
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();

	// 画面に描く処理がすべて終わり、画面に移すので、状態を遷移
	// RenderTargetからPresentにする
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// TransitionBarrierを張る
	commandList_->ResourceBarrier(1, &barrier);

	// コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	// 画面入れ替え

	// GPUにコマンドリストの実行を行わせる
	ComPtr<ID3D12CommandList> commandLists[] = { commandList_ };
	commandQueue_->ExecuteCommandLists(1, commandLists->GetAddressOf());
	// GPUとOSに画面の交換を行うよう通知する
	swapChain_->Present(1, 0);

	// Fenceの値を更新
	fenceValue_++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue_->Signal(fence_.Get(), fenceValue_);

	// Fenceの値が指定したSignal値にたどり着いているか確認する
	// GetCompletedValueの初期値はFence作成時に渡した初期値
	if (fence_->GetCompletedValue() < fenceValue_) {
		// 指定したSignalにたどりついていないので、たどり着くまで待つようにイベントを設定する
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		// イベントを待つ
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

#pragma endregion

#pragma region ゲッター

D3D12_CPU_DESCRIPTOR_HANDLE DirectXBase::GetRTVCPUDescriptorHandle(uint32_t index) {
	return GetCPUDescriptorHandle(rtvDescriptorHeap_, descriptorSizeRTV_, index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXBase::GetSRVCPUDescriptorHandle(uint32_t index) {
	return GetCPUDescriptorHandle(srvDescriptorHeap_, descriptorSizeSRV_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXBase::GetSRVGPUDescriptorHandle(uint32_t index) {
	return GetGPUDescriptorHandle(srvDescriptorHeap_, descriptorSizeSRV_, index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXBase::GetDSVCPUDescriptorHandle(uint32_t index) {
	return GetCPUDescriptorHandle(dsvDescriptorHeap_, descriptorSizeDSV_, index);
}

#pragma endregion

#pragma endregion

#pragma region privateメンバ関数

// デバイスの初期化
void DirectXBase::InitializeDevice() {

	HRESULT hr;

#ifdef _DEBUG
	ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		// デバッグレイヤーを有効化する
		debugController->EnableDebugLayer();
		// さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// DXGIファクトリーの生成

	// HRESULTはWindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	//　初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	// どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	// 使用するアダプタ用の変数。最初にnullptrを入れておく
	ComPtr<IDXGIAdapter4> useAdapter = nullptr;
	// 良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; i++) {
		// アダプタの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr)); // 取得できないのは一大事
		// ソフトウェアアダプタでなければ採用!
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
			// 採用したアダプタの情報をログに出力。wstringの方なので注意
			Log(ConvertString(std::format(L"Use Adapter: {}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr; // ソフトウェアアダプタ場合は見なかったことにする
	}
	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);


	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		// 採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		// 指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr)) {
			// 生成できたのでログ出力を行ってループを抜ける
			Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	// デバイスが生成が上手くいかなかったので起動できない
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");// 初期化完了のログをだす

#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時に止まる。※解放を忘れた際は絶対に戻すように気を付けつつコメントアウトしてログを見る
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			// Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			// https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
	}
#endif
}

// コマンド関連の初期化
void DirectXBase::InitializeCommand() {
	HRESULT hr;

	// コマンドアロケータを生成する
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	// コマンドアロケータの生成が上手くいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	// コマンドリストの生成が上手くいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	// コマンドキューの生成が上手くいかなかったので起動できない
	assert(SUCCEEDED(hr));
}

// スワップチェーンの初期化
void DirectXBase::CreateSwapChain() {
	HRESULT hr;

	// スワップチェーン生成の設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = WindowWrapper::kClientWidth;     // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = WindowWrapper::kClientHeight;   // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 色の形式
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2; // ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニタに移したら、中身を破棄

	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), window_->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

// 深度バッファの生成
void DirectXBase::CreateDepthBuffer() {
	// ウィンドウサイズを取得
	uint32_t width = WindowWrapper::kClientWidth;
	uint32_t height = WindowWrapper::kClientHeight;

	HRESULT hr;

	// Resourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width; // Textureの幅
	resourceDesc.Height = height; // Textureの高さ
	resourceDesc.MipLevels = 1; // mipmapの数
	resourceDesc.DepthOrArraySize = 1; // 奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定。
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 標準的なレイアウト
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f（最大値）でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// Resourceの生成
	hr = device_->CreateCommittedResource(
		&heapProps, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし。
		&resourceDesc,  // Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,  // 深度値を書き込む状態にしておく
		&depthClearValue, // Clear最適値。
		IID_PPV_ARGS(&depthStencilResource_)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
}

// 各種デスクリプタヒープの生成
void DirectXBase::CreateDescriptorHeaps() {
	// DiscriptorSizeを取得しておく
	descriptorSizeRTV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeDSV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisubleはfalse
	rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	// SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisubleはtrue
	srvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	// DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisubleはfalse
	dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

// レンダーターゲットビューの初期化
void DirectXBase::InitializeRenderTargetView() {
	HRESULT hr;

	// SwapChainからResourceを引っ張ってくる
	for (uint32_t i = 0; i < kSwapChainResourcesCount_; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(swapChainResources_[i].ReleaseAndGetAddressOf()));
		// うまく取得できなければ起動できない
		assert(SUCCEEDED(hr));
	}

	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2dテクスチャとして書き込む

	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	// ディスクリプタのサイズを取得する
	uint32_t rtvDescriptorSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 裏表の2つ分
	for (uint32_t i = 0; i < kSwapChainResourcesCount_; ++i) {
		// i番目のレンダーターゲットリソースを取得
		device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandle);

		// 作成したRTVに対応するCPUハンドルをメンバ変数に保存
		rtvHandles_[i] = rtvHandle;

		// 次のディスクリプタの場所に移動
		rtvHandle.ptr += rtvDescriptorSize;
	}

}

// 深度ステンシルビューの初期化
void DirectXBase::InitializeDepthStencilView() {

	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Format。基本的にはResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2dTexture
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // 特殊な設定はなし

	// DSVハンドルを取得
	dsvHandle_ = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	// DSVの生成
	device_->CreateDepthStencilView(
		depthStencilResource_.Get(),
		&dsvDesc,
		dsvHandle_
	);
}

// フェンスの生成
void DirectXBase::CreateFence() {
	HRESULT hr;

	// 初期値０でFenceを作る
	hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	// FenceのSignalを持つためのイベントを作成する
	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

// ビューポート矩形の初期化
void DirectXBase::InitializeViewportRect() {

	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport_.Width = WindowWrapper::kClientWidth;
	viewport_.Height = WindowWrapper::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

// シザー矩形の初期化
void DirectXBase::InitializeScissorRect() {

	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect_.left = 0;
	scissorRect_.right = WindowWrapper::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WindowWrapper::kClientHeight;
}

// DCXコンパイラの生成
void DirectXBase::CreateDXCCompiler() {
	HRESULT hr;

	// dxcCompilerを初期化
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));

	// includeに対応するための設定
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}

// ImGuiの初期化
void DirectXBase::InitializeImGui() {
	// ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window_->GetHwnd());
	
	// ImGuiのDX12初期化
	ImGui_ImplDX12_Init(device_.Get(),
		kSwapChainResourcesCount_,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // RTVのFormat
		srvDescriptorHeap_.Get(),
		srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());

	// フォントを設定する場合はここに追加
}

#pragma endregion

#pragma region privateヘルパー関数

// ディスクリプタヒープの生成
ComPtr<ID3D12DescriptorHeap> DirectXBase::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	HRESULT hr;

	// ディスクリプタヒープの生成
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType; // レンダーターゲットビュー用
	descriptorHeapDesc.NumDescriptors = numDescriptors; // 多くても別に構わない
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	// ディスクリプタヒープの生成が作れなかったので起動できない
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

// CPUHandleの取得
D3D12_CPU_DESCRIPTOR_HANDLE DirectXBase::GetCPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

// GPUHandleの取得
D3D12_GPU_DESCRIPTOR_HANDLE DirectXBase::GetGPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

#pragma endregion
