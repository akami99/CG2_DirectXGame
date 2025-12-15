#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

// 前方宣言
class DX12Context;

// SRV管理
class SrvManager {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

public: // メンバ変数
  // 最大SRV数(最大テクスチャ枚数)
  static const uint32_t kMaxSRVCount;

private: // メンバ変数
  DX12Context *dxBase_ = nullptr;

  // デスクリプタヒープサイズ
  uint32_t descriptorSize_ = 0; // SRV/UAV/CBV用
  // デスクリプタヒープ
  ComPtr<ID3D12DescriptorHeap> descriptorHeap_ = nullptr; // SRV/UAV/CBV用

  // 次に使用するSRVインデックス
  uint32_t useIndex_ = 0;

public: // メンバ関数
  // 初期化
  void Initialize(DX12Context *dxBase);

  // 描画前処理
  void PreDraw();

  // テクスチャ枚数上限チェック
  bool AllocatableTexture();

  uint32_t Allocate();

  // SRVをセット
  void SetGraphicRootDescriptorTable(UINT RootParameterIndex,
                                     uint32_t srvIndex);

  // SrvManager.h
  /// <summary>
  /// 次に使用するSRVインデックスを取得
  /// </summary>
  uint32_t GetNewIndex();

  /// <summary>
  /// SRVの指定したインデックスのCPUディスクリプタハンドルを取得
  /// </summary>
  /// <param name="index">取得するSRVのインデックス</param>
  D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);

  /// <summary>
  /// SRVの指定したインデックスのGPUディスクリプタハンドルを取得
  /// </summary>
  /// <param name="index">取得するSRVのインデックス</param>
  D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

  /// <summary>
  /// テクスチャ用のSRVを作成
  /// </summary>
  /// <param name="srvIndex">デスクリプタヒープ上のインデックス</param>
  /// <param name="resource">StructuredBufferのID3D12Resource</param>
  /// <param name="numElement">格納されている要素（インスタンス）の数</param>
  /// <param name="structureByteStride">構造体（要素）のバイトサイズ</param>
  void CreateSRVForTexture(uint32_t srvIndex,
                                    ComPtr<ID3D12Resource> resource,
                                    DXGI_FORMAT Format, UINT MipLevels);

  /// <summary>
  /// StructuredBuffer用のSRVを作成
  /// </summary>
  /// <param name="srvIndex">デスクリプタヒープ上のインデックス</param>
  /// <param name="resource">StructuredBufferのID3D12Resource</param>
  /// <param name="numElement">格納されている要素（インスタンス）の数</param>
  /// <param name="structureByteStride">構造体（要素）のバイトサイズ</param>
  void CreateSRVForStructuredBuffer(uint32_t srvIndex,
                                    ComPtr<ID3D12Resource> resource,
                                    uint32_t numElement,
                                    uint32_t structureByteStride);
};
