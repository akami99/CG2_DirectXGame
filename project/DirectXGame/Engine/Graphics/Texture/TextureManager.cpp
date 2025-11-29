#include "TextureManager.h"
#include "../Graphics/API/DX12Context.h"
#include "../../Core/Utility/Logger/Logger.h"
#include "../../Core/Utility/String/StringUtility.h"

using namespace Logger;
using namespace StringUtility;
using namespace Microsoft::WRL;

TextureManager* TextureManager::instance_ = nullptr;

// ImGuiで0版を利用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

// 初期化
void TextureManager::Initialize(DX12Context* dxBase) {
	// SRVの数と同数
	textureDatas_.reserve(DX12Context::kMaxSRVCount);

	dxBase_ = dxBase;
}

// シングルトンインスタンスの取得
TextureManager* TextureManager::GetInstance() {
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
uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {
	std::string fullPath = filePath;
	// 既にパスに"Resources/Assets/Sounds/"が含まれている場合は追加しない
	if (filePath.find("Resources/Textures/") == std::string::npos &&
		filePath.find("resources/textures/") == std::string::npos) {
		fullPath = "Resources/Textures/" + filePath;
	}

	if (!std::filesystem::exists(fullPath)) {
		Logger::Log("ERROR: Texture file not found at path: " + fullPath);
		assert(false && "Texture file not found!");
	}

	// 読み込み済みテクスチャを検索
	auto it = std::find_if(
		textureDatas_.begin(),
		textureDatas_.end(),
		[&](TextureData& textureData) { return textureData.filePath == fullPath; }
	);
	if (it != textureDatas_.end()) {
		// 読み込み済みなら要素番号を返す
		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas_.begin(), it));
		return textureIndex + kSRVIndexTop;
	}

	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex) {
	// SRVインデックスを配列インデックスに変換
	// kSRVIndexTop (1) 未満の場合、arrayIndexは0未満になるため、
	// kSRVIndexTopでの最小値チェックを先に行う。
	assert(textureIndex >= kSRVIndexTop && "Error: texture index is the reserved index (0)!");

	uint32_t arrayIndex = textureIndex - kSRVIndexTop;

	// 配列の境界チェック（arrayIndexが配列のサイズ未満であることを確認）
	assert(arrayIndex < textureDatas_.size() && "Error: texture array index out of bounds!");

	// テクスチャデータの参照を取得
	// arrayIndex を使用してアクセスする
	const TextureData& textureData = textureDatas_[arrayIndex];

	// GPUハンドルを返却
	return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(uint32_t textureIndex) {
	// SRVインデックスを配列インデックスに変換
	// kSRVIndexTop (1) 未満の場合、arrayIndexは0未満になるため、
	// kSRVIndexTopでの最小値チェックを先に行う。
	assert(textureIndex >= kSRVIndexTop && "Error: texture index is the reserved index (0)!");

	uint32_t arrayIndex = textureIndex - kSRVIndexTop;

	// 配列の境界チェック（arrayIndexが配列のサイズ未満であることを確認）
	assert(arrayIndex < textureDatas_.size() && "Error: texture array index out of bounds!");

	// テクスチャデータの参照を取得
	// arrayIndex を使用してアクセスする
	const TextureData& textureData = textureDatas_[arrayIndex];

	// GPUハンドルを返却
	return textureData.metadata;
}

uint32_t TextureManager::LoadTexture(const std::string& filePath) {

	std::string fullPath = filePath;
	// 既にパスに"DirectXGame/Resources/Assets/Sounds/"が含まれている場合は追加しない
	if (filePath.find("Resources/Textures/") == std::string::npos &&
		filePath.find("resources/textures/") == std::string::npos) {
		fullPath = "Resources/Textures/" + filePath;
	}

	if (!std::filesystem::exists(fullPath)) {
		Logger::Log("ERROR: Texture file not found at path: " + fullPath);
		assert(false && "Texture file not found!");
	}

	// 読み込み済みテクスチャを検索
	auto it = std::find_if(
		textureDatas_.begin(),
		textureDatas_.end(),
		[&](TextureData& textureData) { return textureData.filePath == fullPath; }
	);
	if (it != textureDatas_.end()) {
		uint32_t arrayIndex = static_cast<uint32_t>(std::distance(textureDatas_.begin(), it));
		return arrayIndex + kSRVIndexTop;
	}

	// テクスチャ枚数上限チェック
	assert(textureDatas_.size() + kSRVIndexTop < DX12Context::kMaxSRVCount);

	HRESULT hr;

	// テクスチャを読み込んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(fullPath);
	hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// テクスチャデータを追加
	textureDatas_.resize(textureDatas_.size() + 1);
	// 追加したテクスチャデータの参照を取得する
	TextureData& textureData = textureDatas_.back();

	textureData.filePath = fullPath;
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxBase_->CreateTextureResource(textureData.metadata);

	// テクスチャデータの要素数番号をSRVのインデックスとする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas_.size() - 1) + kSRVIndexTop;
	// ↑DX12Contextから各種ヒープをヒープマネージャーとして別クラスに分離して、
	// ヒープから確保・解放する機能を実装したい。

	textureData.srvHandleCPU = dxBase_->GetSRVCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = dxBase_->GetSRVGPUDescriptorHandle(srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);

	// SRVの生成
	dxBase_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

	//テクスチャリソースをアップロードし、コマンドリストに積む
	textureData.intermediateResource = dxBase_->UploadTextureData(textureData.resource, mipImages);

	return srvIndex;
}

// 中間リソースをまとめて解放する
void TextureManager::ReleaseIntermediateResources() {
	for (TextureData& textureData : textureDatas_) {
		// intermediateResourceを解放
		textureData.intermediateResource.Reset();
	}

	Logger::Log("INFO: All intermediate texture resources have been released.");
}




