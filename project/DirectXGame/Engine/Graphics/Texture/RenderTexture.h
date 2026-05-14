#pragma once

#pragma warning(push)
#pragma warning(disable: 5050)
#pragma warning(disable: 4005)
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#pragma warning(pop)
#include <cstdint>

class RenderTexture {
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    RenderTexture() = default;
    ~RenderTexture() = default;

    void Initialize(uint32_t width, uint32_t height);

    // 描画前処理
    void PreDraw(ID3D12GraphicsCommandList* commandList);
    // 描画後処理
    void PostDraw(ID3D12GraphicsCommandList* commandList);

    // ゲッター
    uint32_t GetSrvIndex() const { return srvIndex_; }
    ID3D12Resource* GetResource() const { return resource_.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return rtvHandle_; }

private:
    void CreateResource(uint32_t width, uint32_t height);
    void CreateRtv();
    void CreateSrv();

private:
    ComPtr<ID3D12Resource> resource_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    
    uint32_t srvIndex_ = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};

    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};

    // クリアカラー
    const float clearColor_[4] = { 1.0f, 0.0f, 0.0f, 1.0f };//0.1f, 0.25f, 0.5f, 1.0f
};
