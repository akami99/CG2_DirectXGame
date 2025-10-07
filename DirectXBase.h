#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>
#include <array>

#include "WindowWrapper.h"

// DirectX基盤
class DirectXBase {
public: // namespace省略のためのusing宣言
#pragma region using宣言

	// Microsoft::WRL::ComPtrをComPtrで省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
#pragma region privateメンバ変数

	// WindowsAPI
	WindowWrapper* window_ = nullptr;

	// DirectX12デバイス
	ComPtr<ID3D12Device> device_ = nullptr;
	// DXGIファクトリ
	ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;

	// コマンドアロケータ
	ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	// コマンドリスト
	ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	// コマンドキュー
	ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

	// スワップチェーン
	ComPtr<IDXGISwapChain4> swapChain_ = nullptr;

	// 深度バッファ
	ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;// = CreateDepthStencilTextureResource(device, WindowWrapper::kClientWidth, WindowWrapper::kClientHeight);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_ = {};

	// 各種デスクリプタヒープサイズ
	uint32_t descriptorSizeRTV_ = 0; // RTV用
	uint32_t descriptorSizeSRV_ = 0; // SRV/UAV/CBV用
	uint32_t descriptorSizeDSV_ = 0; // DSV用
	// 各種デスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr; // RTV用
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_ = nullptr; // SRV/UAV/CBV用
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr; // DSV用
	
	// レンダーターゲットビュー
	static const uint32_t kSwapChainResourcesCount_ = 2; // スワップチェーンの要素数(ダブルバッファ)
	std::array<ComPtr<ID3D12Resource>, kSwapChainResourcesCount_> swapChainResources_ = {}; // レンダーターゲットビューの配列
	std::array <D3D12_CPU_DESCRIPTOR_HANDLE, kSwapChainResourcesCount_> rtvHandles_ = {}; // RTVのハンドル配列

	// フェンス
	ComPtr<ID3D12Fence> fence_ = nullptr; // GPU-CPU同期用
	uint64_t fenceValue_ = 0; // GPUが次に書き込むフェンス値(CPUで保持)
	HANDLE fenceEvent_ = nullptr; // フェンスの待機イベント

	// ビューポート矩形
	D3D12_VIEWPORT viewport_ = {};

	// シザー矩形
	D3D12_RECT scissorRect_ = {};

	// DXCコンパイラ
	ComPtr<IDxcUtils> dxcUtils_ = nullptr; // ユーティリティ
	ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr; // コンパイラ
	ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr; // インクルードハンドラ

#pragma endregion

public: // メンバ関数
#pragma region publicメンバ関数

	// 初期化
	void Initialize(WindowWrapper* window);
	// 描画前処理
	void PreDraw();
	// 描画後処理
	void PostDraw();

#pragma region ゲッター

    /// <summary>
    /// RTVの指定したインデックスのCPUディスクリプタハンドルを取得
    /// </summary>
    /// <param name="index">取得するRTVのインデックス</param>
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index);
	
    /// <summary>
    /// SRVの指定したインデックスのCPUディスクリプタハンドルを取得
    /// </summary>
	/// <param name="index">取得するSRVのインデックス</param>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// SRVの指定したインデックスのGPUディスクリプタハンドルを取得
	/// </summary>
	/// <param name="index">取得するSRVのインデックス</param>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// DSVの指定したインデックスのCPUディスクリプタハンドルを取得
	/// </summary>
	/// <param name="index">取得するDSVのインデックス</param>
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle(uint32_t index);

#pragma endregion

#pragma endregion

private: // メンバ関数
#pragma region privateメンバ関数

	// デバイスの初期化
	void InitializeDevice();
	// コマンド関連の初期化
	void InitializeCommand();
	// スワップチェーンの生成
	void CreateSwapChain();
	// 深度バッファの生成
	void CreateDepthBuffer();
	// 各種デスクリプタヒープの生成
	void CreateDescriptorHeaps();
	// レンダーターゲットビューの初期化
	void InitializeRenderTargetView();
	// 深度ステンシルビューの初期化
	void InitializeDepthStencilView();
	// フェンスの生成
	void CreateFence();
	// ビューポート矩形の初期化
	void InitializeViewportRect();
	// シザリング矩形の初期化
	void InitializeScissorRect();
	// DXCコンパイラの生成
	void CreateDXCCompiler();
	// ImGuiの初期化
	void InitializeImGui();

#pragma endregion

private: // ヘルパー関数
#pragma region privateヘルパー関数

	/// <summary>
	/// デスクリプタヒープを生成する
	/// </summary>
	/// <param name="heapType">デスクリプタヒープのタイプ</param>
	/// <param name="numDescriptors">デスクリプタの数</param>
	/// <param name="shaderVisible">シェーダから見えるかどうか</param>
	/// <returns>生成したデスクリプタヒープへのポインタ</returns>
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	/// <summary>
	/// 指定したインデックスのCPUディスクリプタハンドルを取得
	/// </summary>
	/// <param name="descriptorHeap">ディスクリプタヒープへのポインタ</param>
	/// <param name="descriptorSize">各ディスクリプタのサイズ</param>
	/// <param name="index">取得するディスクリプタのインデックス</param>
	/// <returns>指定したインデックスに対応する D3D12_CPU_DESCRIPTOR_HANDLE</returns>
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);
	
	/// <summary>
	/// 指定したインデックスのGPUディスクリプタハンドルを取得
	/// </summary>
	/// <param name="descriptorHeap">ディスクリプタヒープへのポインタ</param>
	/// <param name="descriptorSize">各ディスクリプタのサイズ</param>
	/// <param name="index">取得するディスクリプタのインデックス</param>
	/// <returns>指定したインデックスに対応する D3D12_GPU_DESCRIPTOR_HANDLE</returns>
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

#pragma endregion

};

