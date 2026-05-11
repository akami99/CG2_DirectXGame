#include "RenderTexture.h"
#pragma warning(push)
#pragma warning(disable: 5050)
#include "Base/DX12Context.h"
#include "Base/SrvManager.h"
#pragma warning(pop)
#include <cassert>

void RenderTexture::Initialize(uint32_t width, uint32_t height) {
    CreateResource(width, height);
    CreateRtv();
    CreateSrv();

    // ビューポートの設定
    viewport_.Width = static_cast<float>(width);
    viewport_.Height = static_cast<float>(height);
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    // シザー矩形の設定
    scissorRect_.left = 0;
    scissorRect_.top = 0;
    scissorRect_.right = width;
    scissorRect_.bottom = height;
}

void RenderTexture::PreDraw(ID3D12GraphicsCommandList* commandList) {
    // 状態を RenderTarget に遷移
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource_.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // レンダーターゲットをセット
    // 深度バッファは DX12Context のものを借りる（サイズが同じ前提）
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DX12Context::GetInstance()->GetDSVCPUDescriptorHandle(0);
    commandList->OMSetRenderTargets(1, &rtvHandle_, false, &dsvHandle);

    // クリア
    commandList->ClearRenderTargetView(rtvHandle_, clearColor_, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // ビューポートとシザーをセット
    commandList->RSSetViewports(1, &viewport_);
    commandList->RSSetScissorRects(1, &scissorRect_);
}

void RenderTexture::PostDraw(ID3D12GraphicsCommandList* commandList) {
    // 状態を PixelShaderResource に戻す
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource_.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
}

void RenderTexture::CreateResource(uint32_t width, uint32_t height) {
    auto device = DX12Context::GetInstance()->GetDevice();

    // リソース設定
    D3D12_RESOURCE_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.DepthOrArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.SampleDesc.Count = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // クリア値の設定(一旦赤にしておく)
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    clearValue.Color[0] = clearColor_[0];
    clearValue.Color[1] = clearColor_[1];
    clearValue.Color[2] = clearColor_[2];
    clearValue.Color[3] = clearColor_[3];

    // リソース生成
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(&resource_));
    assert(SUCCEEDED(hr));
}

void RenderTexture::CreateRtv() {
    auto device = DX12Context::GetInstance()->GetDevice();

    // RTV用ヒープの生成
    rtvHeap_ = DX12Context::GetInstance()->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false);

    // RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // RTVの生成
    rtvHandle_ = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(resource_.Get(), &rtvDesc, rtvHandle_);
}

void RenderTexture::CreateSrv() {
    srvIndex_ = SrvManager::GetInstance()->Allocate();
    SrvManager::GetInstance()->CreateSRVForTexture(srvIndex_, resource_, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, false);
}
