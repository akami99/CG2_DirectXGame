#include "SpriteCommon.h"

#include <cassert>

#include "API/DX12Context.h"
#include "PSO/PipelineManager.h"

#include "externals/DirectXTex/d3dx12.h"

using namespace Microsoft::WRL;
using namespace BlendMode;

HRESULT hr;

// 初期化
void SpriteCommon::Initialize(DX12Context* dxBase, PipelineManager* pipelineManager) {
	// NULLポインタチェック
	assert(dxBase);
	assert(pipelineManager);
	// メンバ変数にセット
	dxBase_ = dxBase;

	// BlendModeごとの設定は汎用関数から取得
	std::array<D3D12_BLEND_DESC, kCountOfBlendMode> blendDescs = CreateBlendStateDescs();

	// パイプラインマネージャを使って、ブレンドモードごとのPSOを生成
	for (size_t i = 0; i < kCountOfBlendMode; ++i) {
		psoArray_[i] = pipelineManager->CreateSpritePSO(blendDescs[i]);
	}
}

void SpriteCommon::SetCommonDrawSettings(BlendState currentBlendMode, PipelineManager* pipelineManager) {
	// RootSignatureを設定
	dxBase_->GetCommandList()->SetGraphicsRootSignature(pipelineManager->GetSpriteRootSignature());
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
