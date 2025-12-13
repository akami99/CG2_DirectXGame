#include "ModelCommon.h"

#include <cassert>

#include "API/DX12Context.h"

#include "externals/DirectXTex/d3dx12.h"

using namespace Microsoft::WRL;

void ModelCommon::Initialize(DX12Context *dxBase) {
  // メンバ変数にセット
  dxBase_ = dxBase;

}