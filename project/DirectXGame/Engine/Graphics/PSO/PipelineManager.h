#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "API/DX12Context.h"

class PipelineManager {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  // DirectXデバイス
  DX12Context *dxBase_ = nullptr;

  // --- Sprite描画用アセット ---
  ComPtr<IDxcBlob> vsBlobSprite_;                   // Vertex Shader
  ComPtr<IDxcBlob> psBlobSprite_;                   // Pixel Shader
  ComPtr<ID3D12RootSignature> rootSignatureSprite_; // ルートシグネチャ

  // --- 3D描画用アセット ---
  ComPtr<IDxcBlob> vsBlob3D_;                   // Vertex Shader
  ComPtr<IDxcBlob> psBlob3D_;                   // Pixel Shader
  ComPtr<ID3D12RootSignature> rootSignature3D_; // ルートシグネチャ

  ComPtr<IDxcBlob> vsBlobParticle_;                   // Vertex Shader
  ComPtr<IDxcBlob> psBlobParticle_;                   // Pixel Shader
  ComPtr<ID3D12RootSignature> rootSignatureParticle_; // ルートシグネチャ

public: // メンバ関数
  // 初期化(シェーダーとルートシグネチャのロード/生成を実行)
  void Initialize(DX12Context *dxBase);

  // Sprite用のグラフィックスパイプラインを生成して返す関数
  ComPtr<ID3D12PipelineState>
  CreateSpritePSO(const D3D12_BLEND_DESC &blendDesc);

  ComPtr<ID3D12PipelineState>
      CreateParticlePSO(const D3D12_BLEND_DESC &blendDesc);

  // 3D用のグラフィックスパイプラインを生成して返す関数
  ComPtr<ID3D12PipelineState> CreateObject3dPSO(
      const D3D12_INPUT_ELEMENT_DESC *inputLayout, // Object3dCommonが決定
      uint32_t numElements,
      const D3D12_RASTERIZER_DESC &rasterizerDesc, // Object3dCommonが決定
      const D3D12_DEPTH_STENCIL_DESC &depthStencilDesc,
      const D3D12_BLEND_DESC &blendDesc);

  // ゲッター

  // SpriteCommon用
  ID3D12RootSignature *GetSpriteRootSignature() const {
    return rootSignatureSprite_.Get();
  }

  // Object3dCommon用
  ID3D12RootSignature *Get3DRootSignature() const {
    return rootSignature3D_.Get();
  }

  ID3D12RootSignature *GetParticleRootSignature() const {
      return rootSignatureParticle_.Get();
  }

private: // メンバ関数
  // シェーダーの読み込み
  void LoadShader();
  // ルートシグネチャの作成
  void CreateRootSignature();
};
