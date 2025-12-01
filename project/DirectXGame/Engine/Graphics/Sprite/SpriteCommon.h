#pragma once

#include <wrl/client.h>

#include "API/DX12Context.h"
#include "BlendMode/BlendMode.h"

// スプライト共通部
class SpriteCommon {
private: // namespace省略のためのusing宣言
#pragma region using宣言

	// Microsoft::WRL::ComPtrをComPtrで省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
	// DirectXデバイス
	DX12Context* dxBase_ = nullptr;

	// シェーダーバイナリ
	ComPtr<IDxcBlob> vsBlob_; // Vertex Shader
	ComPtr<IDxcBlob> psBlob_; // Pixel Shader

	// ルートシグネチャ
	ComPtr<ID3D12RootSignature> rootSignature_;

	// グラフィックスパイプラインステート (ブレンドモードごとに配列で保持)
	ComPtr<ID3D12PipelineState> psoArray_[BlendMode::BlendState::kCountOfBlendMode];

	// その他共通設定
	D3D12_RASTERIZER_DESC rasterizerDesc_{};


public: // メンバ関数
	// 初期化
	void Initialize(DX12Context* dxBase);

	// 共通描画設定
	void SetCommonDrawSettings(BlendMode::BlendState currentBlendMode);
	
	// DirectX基底部分を取得
	DX12Context* GetDX12Context() const { return dxBase_; }

private: // メンバ関数
	// ルートシグネチャの作成
	void CreateRootSignature();

	// グラフィックスパイプラインの生成
	void CreatePSO();

	// シェーダーの読み込み
	void LoadShader();
};

