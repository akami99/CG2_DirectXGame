#pragma once
#include "Types/GraphicsTypes.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <string>

class Camera;

class Skybox {
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
    static const uint32_t kMaxViews = 3; // 0: Main, 1: Left, 2: Right

private:
    // 頂点・インデックスバッファ
    ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    // 定数バッファ (WVP行列)
    ComPtr<ID3D12Resource> transformResources_[kMaxViews];
    TransformationMatrix* transformData_[kMaxViews] = {nullptr};
    // PSOとRS
    ComPtr<ID3D12PipelineState> pso_;
    // CubeMapのSRVインデックス
    uint32_t textureSrvIndex_ = 0;
    // カメラ参照
    Camera* camera_ = nullptr;
    // トランスフォーム (Debug/ImGui用)
    Vector3 scale_ = { 500.0f, 500.0f, 500.0f };
    Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translate_ = { 0.0f, 0.0f, 0.0f };
public:
	~Skybox();
    void Initialize(const std::string& cubeMapPath);
	// 全ビュー更新
    void Update();
	// 指定したビュー用の更新
    void Update(uint32_t viewIndex, Camera* camera);
    void Draw(uint32_t viewIndex = 0);
    void SetCamera(Camera* camera) { camera_ = camera; }
    // ゲッター・セッター
    Vector3& GetScale() { return scale_; }
    void SetScale(const Vector3& scale) { scale_ = scale; }
    Vector3& GetRotate() { return rotate_; }
    void SetRotate(const Vector3& rotate) { rotate_ = rotate; }
    Vector3& GetTranslate() { return translate_; }
    void SetTranslate(const Vector3& translate) { translate_ = translate; }
private:
    void CreateVertexResource();   // 箱の頂点データ生成
    void CreateIndexResource();    // 箱のインデックスデータ生成
	void CreateTransformResource();// 定数バッファ生成
};