#include "SrvManager.h"

#include "API/DX12Context.h"

using namespace Microsoft::WRL;

const uint32_t SrvManager::kMaxSRVCount = 512;

// 初期化
void SrvManager::Initialize(DX12Context *dxBase) {
  // 引数で受け取ってメンバ変数に記録する
  dxBase_ = dxBase;

  // デスクリプタヒープの生成
  descriptorHeap_ = dxBase_->CreateDescriptorHeap(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
  // デスクリプタヒープ1個分のサイズを取得して記録
  descriptorSize_ = dxBase_->GetDevice()->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

// 描画開始前処理
void SrvManager::PreDraw() {
  // 描画用のDescriptorHeapの設定
  // "生のポインタ配列"を作成(ComPtrだとスマートポインタなので変えないように！)
  ID3D12DescriptorHeap *descriptorHeaps[] = {descriptorHeap_.Get()};

  // 生のポインタ配列をSetDescriptorHeapsに渡す
  dxBase_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
}

bool SrvManager::AllocatableTexture() {
  // useIndexがkMaxSRVCount未満であることを確認
  if (useIndex_ < kMaxSRVCount) {
    return true;
  }

  assert(useIndex_ < kMaxSRVCount && "Error: texture use index out of bounds!");
  return false;
}

uint32_t SrvManager::Allocate() {
  // useIndexがkMaxSRVCount未満であることを確認
  assert(useIndex_ < kMaxSRVCount && "Error: texture use index out of bounds!");

  // returnする番号を一旦記録しておく
  uint32_t index = useIndex_;
  // 次回のために番号を1進める
  useIndex_++;
  // 記録した番号をreturn
  return index;
}

// SRVをセット
void SrvManager::SetGraphicRootDescriptorTable(UINT RootParameterIndex,
                                               uint32_t srvIndex) {
  dxBase_->GetCommandList()->SetGraphicsRootDescriptorTable(
      RootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}

// SRVの指定したインデックスのCPUディスクリプタハンドルを取得
D3D12_CPU_DESCRIPTOR_HANDLE
SrvManager::GetCPUDescriptorHandle(uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
      descriptorHeap_.Get()->GetCPUDescriptorHandleForHeapStart();
  handleCPU.ptr += (descriptorSize_ * index);
  return handleCPU;
}

// SRVの指定したインデックスのGPUディスクリプタハンドルを取得
D3D12_GPU_DESCRIPTOR_HANDLE
SrvManager::GetGPUDescriptorHandle(uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
      descriptorHeap_.Get()->GetGPUDescriptorHandleForHeapStart();
  handleGPU.ptr += (descriptorSize_ * index);
  return handleGPU;
}

// テクスチャ用のSRVを作成
void SrvManager::CreateSRVForTexture(uint32_t srvIndex,
                                              ComPtr<ID3D12Resource> resource,
                                              DXGI_FORMAT Format,
                                              UINT MipLevels) {

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = Format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = MipLevels;

  // SRVを作成するディスクリプタヒープの場所を取得
  D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = GetCPUDescriptorHandle(srvIndex);

  // SRVの生成
  dxBase_->GetDevice()->CreateShaderResourceView(resource.Get(), &srvDesc,
                                                 srvHandleCPU);
}

// StructuredBuffer用のSRVを作成
void SrvManager::CreateSRVForStructuredBuffer(uint32_t srvIndex,
                                              ComPtr<ID3D12Resource> resource,
                                              uint32_t numElement,
                                              uint32_t structureByteStride) {
  // SRVの設定
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = DXGI_FORMAT_UNKNOWN; // StructuredBufferの場合はUNKNOWN
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER; // バッファとして扱う
  srvDesc.Buffer.FirstElement = 0;                    // 最初の要素位置
  srvDesc.Buffer.NumElements = numElement;            // 要素数
  srvDesc.Buffer.StructureByteStride =
      structureByteStride; // 構造体（要素）のバイトサイズ
  srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE; // 特にフラグは無し

  // SRVを作成するディスクリプタヒープの場所を取得
  D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = GetCPUDescriptorHandle(srvIndex);

  // SRVの生成
  dxBase_->GetDevice()->CreateShaderResourceView(resource.Get(), &srvDesc,
                                                 srvHandleCPU);
}
