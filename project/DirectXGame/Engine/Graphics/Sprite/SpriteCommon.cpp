#include "SpriteCommon.h"

#include <cassert>

#include "Base/DX12Context.h"
#include "PSO/PipelineManager.h"

#include "externals/DirectXTex/d3dx12.h"

using namespace Microsoft::WRL;
using namespace BlendMode;

HRESULT hr;

SpriteCommon *SpriteCommon::instance_ = nullptr;

// シングルトンインスタンスの実装
SpriteCommon *SpriteCommon::GetInstance() {
    if (instance_ == nullptr) {
    instance_ = new SpriteCommon;
  }
    return instance_;
}

// 初期化
void SpriteCommon::Initialize() {
  // BlendModeごとの設定は汎用関数から取得
  std::array<D3D12_BLEND_DESC, kCountOfBlendMode> blendDescs =
      CreateBlendStateDescs();

  // パイプラインマネージャを使って、ブレンドモードごとのPSOを生成
  for (size_t i = 0; i < kCountOfBlendMode; ++i) {
    psoArray_[i] = PipelineManager::GetInstance()->CreateSpritePSO(blendDescs[i]);
  }
}

void SpriteCommon::Finalize() {
    delete instance_;
    instance_ = nullptr;
}

void SpriteCommon::SetCommonDrawSettings(BlendState currentBlendMode) {
  // RootSignatureを設定
  DX12Context::GetInstance()->GetCommandList()->SetGraphicsRootSignature(
      PipelineManager::GetInstance()->GetSpriteRootSignature());
  // ブレンドモードに応じたPSOを設定
  if (currentBlendMode >= 0 && currentBlendMode < kCountOfBlendMode) {
    // PSOを設定
    DX12Context::GetInstance()->GetCommandList()->SetPipelineState(
        psoArray_[currentBlendMode].Get());
  } else {
    // 不正な値の場合
    DX12Context::GetInstance()->GetCommandList()->SetPipelineState(
        psoArray_[kBlendModeNormal].Get());
  }
  // トポロジを設定
  DX12Context::GetInstance()->GetCommandList()->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
