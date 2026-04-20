#include "SkyBox.h"
#include "Math/Functions/MathUtils.h"
#include "Math/Matrix/MatrixGenerators.h"
#include "Types/GraphicsTypes.h"

#include "Base/DX12Context.h"
#include "Texture/TextureManager.h"
#include "PSO/PipelineManager.h"
#include "Base/SrvManager.h"
#include "Camera/Camera.h"
using namespace MathUtils;

using namespace MathGenerators;

Skybox::~Skybox() {
	// Map済みリソースをUnmap
	if (transformResource_ && transformData_) {
		transformResource_->Unmap(0, nullptr);
		transformData_ = nullptr;
	}
	// ComPtrは自動的にReleaseされる
}

void Skybox::Initialize(const std::string& cubeMapPath) {
	CreateVertexResource();
	CreateIndexResource();
	CreateTransformResource();

	// CubeMapの読み込み(テクスチャマネージャに任せる)
	TextureManager::GetInstance()->LoadTexture(cubeMapPath);
	// CubeMapのSRVインデックスを取得して保存
	textureSrvIndex_ = TextureManager::GetInstance()->GetSrvIndex(cubeMapPath);

	// PSOとRSの作成はPipelineManagerに任せる
	pso_ = PipelineManager::GetInstance()->CreateSkyboxPSO();
}

void Skybox::Update() {
	if (!camera_) {
		return; // カメラがセットされていない場合は更新しない
	}
	Matrix4x4 view = camera_->GetViewMatrix();
	view.m[3][0] = 0.0f; // カメラの位置を反映させない
	view.m[3][1] = 0.0f;
	view.m[3][2] = 0.0f;

	Matrix4x4 wvp = Multiply(MakeScaleMatrix({ 500.0f, 500.0f, 500.0f }), view) * camera_->GetProjectionMatrix();
	transformData_->WVP = wvp;
	transformData_->World = MakeIdentity4x4();
}

void Skybox::Draw() {
	auto* cmd = DX12Context::GetInstance()->GetCommandList();
	// PSO / ルートシグネチャのセット
	cmd->SetPipelineState(pso_.Get());
	cmd->SetGraphicsRootSignature(PipelineManager::GetInstance()->GetSkyboxRootSignature());
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// VBV / IBV（Modelと同じ）
	cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);
	cmd->IASetIndexBuffer(&indexBufferView_);
	// Transform CBV (b0)
	cmd->SetGraphicsRootConstantBufferView(0, transformResource_->GetGPUVirtualAddress());
	// CubeMap SRV (t0)
	SrvManager::GetInstance()->SetGraphicRootDescriptorTable(1, textureSrvIndex_);
	// 描画（Modelと同じDrawIndexedInstanced）
	cmd->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

void Skybox::CreateVertexResource() {
	// 8頂点の箱を作る
	SkyboxVertex vertices[24] = {};
	// 右面 [0,1,2][2,1,3] ← 内側を向く
	vertices[0].position = { 1.0f,  1.0f,  1.0f, 1.0f }; // 右上奥
	vertices[1].position = { 1.0f,  1.0f, -1.0f, 1.0f }; // 右上前
	vertices[2].position = { 1.0f, -1.0f,  1.0f, 1.0f }; // 右下奥
	vertices[3].position = { 1.0f, -1.0f, -1.0f, 1.0f }; // 右下前

	// 左面 [4,5,6][6,5,7]
	vertices[4].position = { -1.0f,  1.0f, -1.0f, 1.0f }; // 左上前
	vertices[5].position = { -1.0f,  1.0f,  1.0f, 1.0f }; // 左上奥
	vertices[6].position = { -1.0f, -1.0f, -1.0f, 1.0f }; // 左下前
	vertices[7].position = { -1.0f, -1.0f,  1.0f, 1.0f }; // 左下奥

	// 前面 [8,9,10][10,9,11]
	vertices[8].position = { -1.0f,  1.0f,  1.0f, 1.0f }; // 左上
	vertices[9].position = { 1.0f,  1.0f,  1.0f, 1.0f }; // 右上
	vertices[10].position = { -1.0f, -1.0f,  1.0f, 1.0f }; // 左下
	vertices[11].position = { 1.0f, -1.0f,  1.0f, 1.0f }; // 右下

	// 後面 [12,13,14][14,13,15]
	vertices[12].position = { 1.0f,  1.0f, -1.0f, 1.0f }; // 右上
	vertices[13].position = { -1.0f,  1.0f, -1.0f, 1.0f }; // 左上
	vertices[14].position = { 1.0f, -1.0f, -1.0f, 1.0f }; // 右下
	vertices[15].position = { -1.0f, -1.0f, -1.0f, 1.0f }; // 左下

	// 上面 [16,17,18][18,17,19]
	vertices[16].position = { -1.0f,  1.0f, -1.0f, 1.0f }; // 左前
	vertices[17].position = { 1.0f,  1.0f, -1.0f, 1.0f }; // 右前
	vertices[18].position = { -1.0f,  1.0f,  1.0f, 1.0f }; // 左奥
	vertices[19].position = { 1.0f,  1.0f,  1.0f, 1.0f }; // 右奥

	// 下面 [20,21,22][22,21,23]
	vertices[20].position = { -1.0f, -1.0f,  1.0f, 1.0f }; // 左奥
	vertices[21].position = { 1.0f, -1.0f,  1.0f, 1.0f }; // 右奥
	vertices[22].position = { -1.0f, -1.0f, -1.0f, 1.0f }; // 左前
	vertices[23].position = { 1.0f, -1.0f, -1.0f, 1.0f }; // 右前

#pragma region リソースとバッファビューの作成
	// リソースとバッファビューの作成

	// VertexResourceを作る
	vertexResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(vertices));
	// VertexBufferViewを作成する
	// リソースの先頭アドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点24個分のサイズ
	vertexBufferView_.SizeInBytes =	UINT(sizeof(vertices));
	// １頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(SkyboxVertex);
#pragma endregion ここまで

#pragma region VertexDataの設定
	// VertexDataの設定

	// 書き込むためのアドレスを取得
	SkyboxVertex* vertexData = nullptr;
	vertexResource_->Map(
		0, nullptr,
		reinterpret_cast<void**>(&vertexData)); // 書き込むためのアドレスを取得
	std::memcpy(vertexData, vertices, sizeof(vertices)); // 頂点データをリソースにコピー
	vertexResource_->Unmap(0, nullptr); // 毎フレーム書き換えないためUnmap

#pragma endregion ここまで
}

void Skybox::CreateIndexResource() {
	// 36個のインデックスを作る（12三角形）
	uint32_t indices[36] = {
	 0,  1,  2,  2,  1,  3,  // 右面
	 4,  5,  6,  6,  5,  7,  // 左面
	 8,  9, 10, 10,  9, 11,  // 前面
	12, 13, 14, 14, 13, 15,  // 後面
	16, 17, 18, 18, 17, 19,  // 上面
	20, 21, 22, 22, 21, 23,  // 下面
	};

	// リソースのサイズ（インデックス数 * uint32_tのサイズ）
	size_t sizeInBytes = sizeof(indices);

	// リソース作成
	indexResource_ =
		DX12Context::GetInstance()->CreateBufferResource(sizeInBytes);

	// インデックスバッファビューの作成
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = UINT(sizeInBytes);
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT; // uint32_tを使用

	// データの書き込み
	uint32_t* mappedIndex = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndex));
	std::memcpy(mappedIndex, indices, sizeInBytes);
	indexResource_->Unmap(0, nullptr);
}

void Skybox::CreateTransformResource() {
	// 変換行列リソースの作成
	transformResource_ =
		DX12Context::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
	// TransformationMatrixDataの設定
	// TransformationMatrixResourceにデータを書き込むためのアドレスを取得してTransformationMatrixDataに割り当てる
	// 書き込むためのアドレスを取得
	transformResource_->Map(
		0, nullptr,
		reinterpret_cast<void**>(&transformData_)); // 書き込むためのアドレスを取得
	transformData_->WVP = MakeIdentity4x4(); // ワールドビュー射影行列を初期化
	transformData_->World = MakeIdentity4x4(); // ワールド行列を初期化
}