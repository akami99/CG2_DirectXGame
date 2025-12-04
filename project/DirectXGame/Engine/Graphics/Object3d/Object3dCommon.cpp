#include "Object3dCommon.h"

#include <cassert>

#include "API/DX12Context.h"
#include "PSO/PipelineManager.h"

#include "externals/DirectXTex/d3dx12.h"

using namespace Microsoft::WRL;
using namespace BlendMode;

void Object3dCommon::Initialize(DX12Context* dxBase, PipelineManager* pipelineManager) {
	// NULLポインタチェック
	assert(dxBase);
	assert(pipelineManager);
	// メンバ変数にセット
	dxBase_ = dxBase;

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	const UINT kNumElements = _countof(inputLayout);

	// ラスタライザステート
	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; // 背面カリング

	// デプスステンシルステート
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	// BlendModeごとの設定は汎用関数から取得
	std::array<D3D12_BLEND_DESC, kCountOfBlendMode> blendDescs = CreateBlendStateDescs();

	// パイプラインマネージャを使って、ブレンドモードごとのPSOを生成
	for (size_t i = 0; i < kCountOfBlendMode; ++i) {
		psoArray_[i] = pipelineManager->CreateObject3dPSO(
			inputLayout, _countof(inputLayout),
			rasterizerDesc, depthStencilDesc, blendDescs[i]
		);
	}
}

void Object3dCommon::SetCommonDrawSettings(BlendState currentBlendMode, PipelineManager* pipelineManager) {
	// RootSignatureを設定
	dxBase_->GetCommandList()->SetGraphicsRootSignature(pipelineManager->Get3DRootSignature());
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
