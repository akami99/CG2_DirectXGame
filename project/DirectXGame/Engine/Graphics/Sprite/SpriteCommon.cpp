#include "SpriteCommon.h"

#include <cassert>

#include "Logger/Logger.h"

#include "externals/DirectXTex/d3dx12.h"
using namespace Logger;
using namespace Microsoft::WRL;
using namespace BlendMode;

HRESULT hr;

// 初期化
void SpriteCommon::Initialize(DX12Context* dxBase) {
	// NULLポインタチェック
	assert(dxBase);
	// メンバ変数にセット
	dxBase_ = dxBase;
	// シェーダーの読み込み
	LoadShader();
	// グラフィックスパイプラインの生成
	CreatePSO();
}

void SpriteCommon::SetCommonDrawSettings(BlendState currentBlendMode) {
	// RootSignatureを設定
	dxBase_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
	// ブレンドモードに応じたPSOを設定
	if (currentBlendMode >= 0 && currentBlendMode < kCountOfBlendMode) {
		// PSOを設定
		dxBase_->GetCommandList()->SetPipelineState(psoArray_[currentBlendMode].Get());
	} else {
		// 不正な値の場合
		dxBase_->GetCommandList()->SetPipelineState(psoArray_[kBlendModeNormal].Get());
	}
	// トポロジを設定
	dxBase_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// ルートシグネチャの作成
void SpriteCommon::CreateRootSignature() {
#pragma region 共通のSampler設定
	// 共通のSamplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;   // 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;   // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;   // ありったけのMipmapを使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	staticSamplers[0].ShaderRegister = 0;   // レジスタ番号0を使う

#pragma endregion ここまで

#pragma region Sprite用のRootSignature作成
	// Sprite用のRootSignature作成

	// Texture用 SRV (Pixel Shader用)
	D3D12_DESCRIPTOR_RANGE spriteTextureSrvRange[1] = {};
	spriteTextureSrvRange[0].BaseShaderRegister = 0;// t0
	spriteTextureSrvRange[0].NumDescriptors = 1;
	spriteTextureSrvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	spriteTextureSrvRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameter作成
	D3D12_ROOT_PARAMETER spriteRootParameters[3] = {};

	// Root Parameter 0: Pixel Shader用 Material CBV (b0)
	spriteRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	spriteRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	spriteRootParameters[0].Descriptor.ShaderRegister = 0; // b0

	// Root Parameter 1: Vertex Shader用 Transform CBV (b0)
	spriteRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	spriteRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	spriteRootParameters[1].Descriptor.ShaderRegister = 1; // b1

	// Root Parameter 2: Pixel Shader用 Texture SRV (t0)
	spriteRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	spriteRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	spriteRootParameters[2].DescriptorTable.pDescriptorRanges = spriteTextureSrvRange;
	spriteRootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(spriteTextureSrvRange);

	// RootSignatureDesc
	D3D12_ROOT_SIGNATURE_DESC spriteRootSignatureDesc{};
	spriteRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	spriteRootSignatureDesc.pParameters = spriteRootParameters;
	spriteRootSignatureDesc.NumParameters = _countof(spriteRootParameters);
	spriteRootSignatureDesc.pStaticSamplers = staticSamplers; // 共通Samplerを使用
	spriteRootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズと生成
	ComPtr<ID3DBlob> spriteSignatureBlob = nullptr;
	ComPtr<ID3DBlob> spriteErrorBlob = nullptr;

	hr = D3D12SerializeRootSignature(&spriteRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &spriteSignatureBlob, &spriteErrorBlob);
	// エラーチェックと生成処理...
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(spriteErrorBlob->GetBufferPointer()));
		assert(false);
	}
	hr = dxBase_->GetDevice()->CreateRootSignature(0, spriteSignatureBlob->GetBufferPointer(), spriteSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

#pragma endregion ここまで
}

// グラフィックスパイプラインの生成
void SpriteCommon::CreatePSO() {
	// ルートシグネチャの作成
	CreateRootSignature();

#pragma region InputLayout

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].InputSlot = 0; // 明示的に指定
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
	inputElementDescs[0].InstanceDataStepRate = 0; // デフォルト値だが明示するとより分かりやすい
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].InputSlot = 0; // 明示的に指定
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
	inputElementDescs[1].InstanceDataStepRate = 0;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].InputSlot = 0; // 明示的に指定
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
	inputElementDescs[2].InstanceDataStepRate = 0;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

#pragma endregion ここまで

#pragma region PSO共通設定
	// PSO共通設定

	// RasterizerState作成
	// カリング
	rasterizerDesc_.CullMode = D3D12_CULL_MODE_NONE;
	// 三角形の中を塗りつぶす
	rasterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescBase{};

	// 共通設定をセット
	psoDescBase.pRootSignature = rootSignature_.Get(); // Sprite用Root Signature
	psoDescBase.InputLayout = inputLayoutDesc; // 共通のInputLayoutを使用 (VertexDataの構成)
	psoDescBase.VS = { vsBlob_->GetBufferPointer(), vsBlob_->GetBufferSize() };
	psoDescBase.PS = { psBlob_->GetBufferPointer(), psBlob_->GetBufferSize() };
	psoDescBase.RasterizerState = rasterizerDesc_; // 共通のRasterizerStateを使用
	psoDescBase.NumRenderTargets = 1;
	psoDescBase.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDescBase.SampleDesc.Count = 1;

	// Depth/Stencilの設定は DepthTest を無効化
	psoDescBase.DepthStencilState.DepthEnable = false; // 2DなのでDepthテストを無効化
	psoDescBase.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込みも無効
	psoDescBase.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	psoDescBase.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDescBase.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

#pragma endregion ここまで

#pragma region ブレンドモードごとのPSO生成

	// BlendModeごとの設定は外部の汎用関数から取得
	std::array<D3D12_BLEND_DESC, kCountOfBlendMode> blendDescs = CreateBlendStateDescs();

	// PSOの生成 (ループ部分)
	for (int i = 0; i < kCountOfBlendMode; ++i) {
		// i番目のブレンド設定をPSOにセット
		psoDescBase.BlendState = blendDescs[i];

		// PSOの生成

		hr = dxBase_->GetDevice()->CreateGraphicsPipelineState(&psoDescBase, IID_PPV_ARGS(&psoArray_[i]));
		assert(SUCCEEDED(hr));
	}

#pragma endregion ここまで
}

// シェーダーの読み込み
void SpriteCommon::LoadShader() {
	// VertexShaderの読み込み
	vsBlob_ = dxBase_->CompileShader(L"Sprite.VS.hlsl", L"vs_6_0");
	assert(SUCCEEDED(hr));
	// PixelShaderの読み込み
	psBlob_ = dxBase_->CompileShader(L"Sprite.PS.hlsl", L"ps_6_0");
	assert(SUCCEEDED(hr));
}
