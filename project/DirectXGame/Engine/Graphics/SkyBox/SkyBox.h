#pragma once
#include "Types/GraphicsTypes.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
class Camera;
class Skybox {
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
private:
    // 頂点・インデックスバッファ
    ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    // 定数バッファ (WVP行列)
    ComPtr<ID3D12Resource> transformResource_;
    TransformationMatrix* transformData_ = nullptr;
    // PSOとRS
    ComPtr<ID3D12PipelineState> pso_;
    // CubeMapのSRVインデックス
    uint32_t textureSrvIndex_ = 0;
    // カメラ参照
    Camera* camera_ = nullptr;
public:
	~Skybox();
    void Initialize(const std::string& cubeMapPath);
    void Update();
    void Draw();
    void SetCamera(Camera* camera) { camera_ = camera; }
private:
    void CreateVertexResource();   // 箱の頂点データ生成
    void CreateIndexResource();    // 箱のインデックスデータ生成
    void CreateTransformResource();
};