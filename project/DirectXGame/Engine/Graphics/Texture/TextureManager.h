#pragma once

#include <d3d12.h>
#include <filesystem>
#include <wrl/client.h>

#include "externals/DirectXTex/DirectXTex.h"

class DX12Context;

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
    std::string filePath;
    DirectX::TexMetadata metadata;
    ComPtr<ID3D12Resource> resource;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
    ComPtr<ID3D12Resource> intermediateResource;
  };

  static TextureManager *instance_;

  // テクスチャデータ
  std::vector<TextureData> textureDatas_;

  DX12Context *dxBase_ = nullptr;

  // SRVインデックスの開始番号
  static uint32_t kSRVIndexTop;

public: // メンバ関数
  // 初期化
  void Initialize(DX12Context *dxBase);

  // シングルトンインスタンスの取得
  static TextureManager *GetInstance();
  // 終了(この後にGetInstance()するとまたnewするので注意)
  void Finalize();

  // SRVインデックスの開始番号
  uint32_t GetTextureIndexByFilePath(const std::string &filePath);

  // テクスチャ番号からGPUハンドルを取得
  D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

  // メタデータを取得
  const DirectX::TexMetadata &GetMetaData(uint32_t textureIndex);

  /// <summary>
  /// テクスチャファイルの読み込み
  /// </summary>
  /// <param name="filePath">テクスチャファイルのパス</param>
  /// <returns>テクスチャインデックス< /returns>
  uint32_t LoadTexture(const std::string &filePath);

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
