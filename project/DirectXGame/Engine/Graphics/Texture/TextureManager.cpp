#include "TextureManager.h"
#include "../../Core/Utility/Logger/Logger.h"
#include "../../Core/Utility/String/StringUtility.h"
#include "../Graphics/API/DX12Context.h"
#include "../SRV/SrvManager.h"

using namespace Logger;
using namespace StringUtility;
using namespace Microsoft::WRL;

TextureManager *TextureManager::instance_ = nullptr;

// 初期化
void TextureManager::Initialize(DX12Context *dxBase, SrvManager *srvManager) {
  dxBase_ = dxBase;

  srvManager_ = srvManager;

  // SRVの数と同数
  textureDatas_.reserve(SrvManager::kMaxSRVCount);
}

// シングルトンインスタンスの取得
TextureManager *TextureManager::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new TextureManager;
  }
  return instance_;
}

// 終了
void TextureManager::Finalize() {
  delete instance_;
  instance_ = nullptr;
}

// SRVインデックスの開始番号
uint32_t TextureManager::GetSrvIndex(const std::string &filePath) {
  std::string fullPath = filePath;
  // 既にパスに"Resources/Textures/"が含まれている場合は追加しない
  if (filePath.find("Resources/Textures/") == std::string::npos &&
      filePath.find("resources/textures/") == std::string::npos) {
    fullPath = "Resources/Textures/" + filePath;
  }

  // 読み込み済みテクスチャを検索
  auto it = textureDatas_.find(fullPath);

  // ロード済みならインデックスを返す
  if (it != textureDatas_.end()) {
    // 読み込み済みなら要素番号を返す
    return it->second.srvIndex;
  }

  // ロードされていなかったらエラーとして処理する
  Logger::Log("ERROR: Texture not loaded for path: " + fullPath);
  assert(false && "Texture not loaded!");
  return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE
TextureManager::GetSrvHandleGPU(const std::string &filePath) {

  // 配列の境界チェック（arrayIndexが配列のサイズ未満であることを確認）
  assert(srvManager_->AllocatableTexture() &&
         "Error: texture array index out of bounds!");

  std::string fullPath = filePath;
  // 既にパスに"Resources/Textures/"が含まれている場合は追加しない
  if (filePath.find("Resources/Textures/") == std::string::npos &&
      filePath.find("resources/textures/") == std::string::npos) {
      fullPath = "Resources/Textures/" + filePath;
  }

  // 読み込み済みテクスチャを検索
  auto it = textureDatas_.find(fullPath);

  // データが見つかったか確認
  if (it == textureDatas_.end()) {
    Logger::Log("ERROR: Requested texture not found in map: " + fullPath);
    assert(false && "Texture data not found!");
    // エラー処理として、適切なデフォルト値やエラーテクスチャのハンドルを返す
    return D3D12_GPU_DESCRIPTOR_HANDLE{};
  }

  // 見つかった要素 (it->second) からハンドルを返す
  return it->second.srvHandleGPU;
}

const DirectX::TexMetadata &
TextureManager::GetMetaData(const std::string &filePath) {
  // 配列の境界チェック（arrayIndexが配列のサイズ未満であることを確認）
  assert(srvManager_->AllocatableTexture() &&
         "Error: texture array index out of bounds!");

  std::string fullPath = filePath;
  // 既にパスに"Resources/Textures/"が含まれている場合は追加しない
  if (filePath.find("Resources/Textures/") == std::string::npos &&
      filePath.find("resources/textures/") == std::string::npos) {
      fullPath = "Resources/Textures/" + filePath;
  }

  // 読み込み済みテクスチャを検索
  auto it = textureDatas_.find(fullPath);

  // データが見つかったか確認
  if (it == textureDatas_.end()) {
    Logger::Log("ERROR: Requested texture not found in map: " + fullPath);
    assert(false && "Texture data not found!");
    // エラー処理として、適切なエラーメタデータを返す（ここでは簡略化）
    // 実際にはデフォルトの静的なエラー用メタデータを返すのが望ましい
    static DirectX::TexMetadata errorMetadata{};
    return errorMetadata;
  }

  // 見つかった要素 (it->second) からメタデータを返す
  return it->second.metadata;
}

// テクスチャロード
void TextureManager::LoadTexture(const std::string &filePath) {

  std::string fullPath = filePath;
  // 既にパスに"Resources/Textures/"が含まれている場合は追加しない
  if (filePath.find("Resources/Textures/") == std::string::npos &&
      filePath.find("resources/textures/") == std::string::npos) {
    fullPath = "Resources/Textures/" + filePath;
  }

  if (!std::filesystem::exists(fullPath)) {
    Logger::Log("ERROR: Texture file not found at path: " + fullPath);
    assert(false && "Texture file not found!");
    return;
  }

  // 読み込み済みテクスチャを検索
  auto it = textureDatas_.find(fullPath);

  if (it != textureDatas_.end()) {
    return;
  }

  // テクスチャ枚数上限チェック
  assert(srvManager_->AllocatableTexture());

  HRESULT hr;

  // テクスチャを読み込んでプログラムで扱えるようにする
  DirectX::ScratchImage image{};
  std::wstring filePathW = ConvertString(fullPath);
  hr = DirectX::LoadFromWICFile(filePathW.c_str(),
                                DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミップマップの作成
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  Log("INFO: Loaded texture at path: " + fullPath + "\n");

  // テクスチャデータを追加
  TextureData &textureData = textureDatas_[fullPath];

  textureData.metadata = mipImages.GetMetadata();
  textureData.resource = dxBase_->CreateTextureResource(textureData.metadata);

  // SRV確保
  textureData.srvIndex = srvManager_->Allocate();
  textureData.srvHandleCPU =
      srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
  textureData.srvHandleGPU =
      srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

  srvManager_->CreateSRVForTexture(
      textureData.srvIndex, textureData.resource, textureData.metadata.format,
      UINT(textureData.metadata.mipLevels));

  // テクスチャリソースをアップロードし、コマンドリストに積む
  textureData.intermediateResource =
      dxBase_->UploadTextureData(textureData.resource, mipImages);
}

// 中間リソースをまとめて解放する
void TextureManager::ReleaseIntermediateResources() {
  for (auto &pair : textureDatas_) {
    // intermediateResourceを解放
    pair.second.intermediateResource.Reset();
  }

  Logger::Log("INFO: All intermediate texture resources have been released.\n");
}
