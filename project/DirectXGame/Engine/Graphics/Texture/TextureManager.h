#pragma once

#include <d3d12.h>
#include <filesystem>
#include <unordered_map>
#include <wrl/client.h>

#include "externals/DirectXTex/DirectXTex.h"

class DX12Context;
class SrvManager;

// テクスチャマネージャー(シングルトン)
class TextureManager {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  // テクスチャ1枚分のデータ
  struct TextureData {
    DirectX::TexMetadata metadata;
    ComPtr<ID3D12Resource> resource;
    uint32_t srvIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
    ComPtr<ID3D12Resource> intermediateResource;
  };

  static TextureManager *instance_;

  // テクスチャデータ
  std::unordered_map<std::string, TextureData>
      textureDatas_; // キーの順番を保つならunordered_mapの方が高速

  DX12Context *dxBase_ = nullptr;

  SrvManager *srvManager_ = nullptr;

public: // メンバ関数
  // 初期化
  void Initialize(DX12Context *dxBase, SrvManager *srvManager);

  // シングルトンインスタンスの取得
  static TextureManager *GetInstance();
  // 終了(この後にGetInstance()するとまたnewするので注意)
  void Finalize();

  // SRVインデックスの取得
  uint32_t GetSrvIndex(const std::string &filePath);

  // テクスチャ番号からGPUハンドルを取得
  D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string &filePath);

  // メタデータを取得
  const DirectX::TexMetadata &GetMetaData(const std::string &filePath);

  /// <summary>
  /// テクスチャファイルの読み込み
  /// </summary>
  /// <param name="filePath">テクスチャファイルのパス</param>
  void LoadTexture(const std::string &filePath);

  /// <summary>
  /// 中間リソースをまとめて解放する
  /// </summary>
  void ReleaseIntermediateResources();

private: // メンバ関数
  // コンストラクタ(隠蔽)
  TextureManager() = default;
  // デストラクタ(隠蔽)
  ~TextureManager() = default;
  // コピーコンストラクタの封印
  TextureManager(TextureManager &) = delete;
  // コピー代入演算子の封印
  TextureManager &operator=(TextureManager &) = delete;
};
