#include "Model.h"
#include "Base/DX12Context.h"
#include "Texture/TextureManager.h"

#include "Math/Functions/MathUtils.h"
#include "Math/Matrix/MatrixGenerators.h"
#include "Logger.h"

#include <assert.h>
#include <fstream>
#include <sstream>
#include <string>

using namespace Microsoft::WRL;
using namespace MathUtils;
using namespace MathGenerators;

void Model::Initialize(const std::string &directoryPath,
                       const std::string &filename) {
  // モデル読み込み
  LoadModelFile(directoryPath, filename);

  // 頂点データを作成する
  CreateVertexResource();

  // インデックスバッファ作成
  CreateIndexResource();

  // マテリアルバッファの作成
  CreateMaterialResource();

  // .objの参照しているテクスチャファイル読み込み、インデックスを代入
  TextureManager::GetInstance()->LoadTexture(
      modelData_.material.textureFilePath);

  modelData_.material.textureIndex = TextureManager::GetInstance()->GetSrvIndex(
      modelData_.material.textureFilePath);
}

// 描画処理
void Model::Draw() {
  // VertexBufferの設定
  DX12Context::GetInstance()->GetCommandList()->IASetVertexBuffers(
      0, 1, &vertexBufferView_);
  // IBV(インデックスバッファビュー)の設定
  DX12Context::GetInstance()->GetCommandList()->IASetIndexBuffer(
      &indexBufferView_);
  // マテリアルCBVの設定
  DX12Context::GetInstance()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          0, materialResource_->GetGPUVirtualAddress());
  // SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
  DX12Context::GetInstance()->GetCommandList()->SetGraphicsRootDescriptorTable(
      2, TextureManager::GetInstance()->GetSrvHandleGPU(
             modelData_.material.textureFilePath));
  // 描画コマンド
  /*DX12Context::GetInstance()->GetCommandList()->DrawInstanced(
      UINT(modelData_.vertices.size()), 1, 0, 0);*/
  // ★変更: インデックスを使用した描画コマンド
  // 引数: (インデックス数, インスタンス数, インデックス開始位置, 頂点オフセット, インスタンスオフセット)
  DX12Context::GetInstance()->GetCommandList()->DrawIndexedInstanced(
      UINT(modelData_.indices.size()), 1, 0, 0, 0);
}

// .objファイルの読み取り
void Model::LoadModelFile(const std::string &directoryPath,
                          const std::string &fileName) {
    Assimp::Importer importer;
    std::string fullPath = directoryPath + "/" + fileName;

    // 現在成功している設定（左手系・時計回り・UV反転）を維持
    const aiScene* scene = importer.ReadFile(fullPath.c_str(),
        aiProcess_FlipWindingOrder | // 時計回りに変換
        aiProcess_FlipUVs |          // UV反転
        //aiProcess_MakeLeftHanded |   // 左手系に変換
        aiProcess_Triangulate        // 三角形化
    );

    // シーンの読み込みチェック
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::Log("ERROR: Assimp import failed: " + std::string(importer.GetErrorString()));
        assert(false && "Assimp import failed");
        return;
    }

    // データをクリアしておく
    modelData_.vertices.clear();
    modelData_.indices.clear();

    // --- メッシュの解析（複数メッシュ対応） ---
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        assert(mesh->HasNormals());
        assert(mesh->HasTextureCoords(0));
        assert(mesh->HasFaces());

        // ★重要：現在の頂点数を保持しておく（これがインデックスのオフセットになる）
        // 1つ目のメッシュなら0、2つ目なら1つ目の頂点数がここに入ります
        uint32_t indexOffset = static_cast<uint32_t>(modelData_.vertices.size());

        // --- 1. 頂点データの追加 ---
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D& position = mesh->mVertices[i];
            aiVector3D& normal = mesh->mNormals[i];
            aiVector3D& texcoord = mesh->mTextureCoords[0][i];

            VertexData vertex;
            vertex.position = { position.x, position.y, position.z, 1.0f };
            vertex.normal = { normal.x, normal.y, normal.z };
            vertex.texcoord = { texcoord.x, texcoord.y };

            vertex.position.x *= -1.0f;
            vertex.normal.x *= -1.0f;

            modelData_.vertices.push_back(vertex);
        }

        // --- 2. インデックスデータの追加 ---
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            aiFace& face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3);

            // インデックスにオフセットを足して、全体の通し番号にする
            modelData_.indices.push_back(face.mIndices[0] + indexOffset);
            modelData_.indices.push_back(face.mIndices[1] + indexOffset);
            modelData_.indices.push_back(face.mIndices[2] + indexOffset);
        }
    }

    // --- 3. マテリアルの解析 ---
    // ※現在の構造（Modelクラスにテクスチャが1つ）だと、複数のテクスチャを持つモデルは正しく表現できません。
    // そのため、とりあえず「最初にテクスチャを持っているメッシュのマテリアル」を採用する形にします。
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial* material = scene->mMaterials[i];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
            aiString textureFilePath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
            modelData_.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
            // ひとつ見つかったら終了（現在の仕様の限界）
            break;
        }
    }

    // --- 3. ノードの解析 ---
    modelData_.rootNode = ReadNode(scene->mRootNode);

    // データが空でないか最終チェック
    assert(!modelData_.vertices.empty() && "Vertex data is empty");
    assert(!modelData_.indices.empty() && "Index data is empty");
}

Node Model::ReadNode(aiNode* node) {
    Node result;
    aiMatrix4x4 aiLocalMatrix = node->mTransformation; // nodeのMatrixを取得
    aiLocalMatrix.Transpose(); // 列ベクトル形式を行ベクトル形式に転置（Assimpは列優先、DirectXは行優先のため）
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.localMatrix.m[i][j] = aiLocalMatrix[i][j];
        }
    }

    result.name = node->mName.C_Str(); // Node名を取得
    result.children.resize(node->mNumChildren); // 子供の数だけ確保
    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        // 再帰的に読んで階層構造を作っていく
        result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
    }
    return result;
}

void Model::CreateIndexResource() {
  // リソースのサイズ（インデックス数 * uint32_tのサイズ）
  size_t sizeInBytes = sizeof(uint32_t) * modelData_.indices.size();

  // リソース作成
  indexResource_ =
      DX12Context::GetInstance()->CreateBufferResource(sizeInBytes);

  // インデックスバッファビューの作成
  indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
  indexBufferView_.SizeInBytes = UINT(sizeInBytes);
  indexBufferView_.Format = DXGI_FORMAT_R32_UINT; // uint32_tを使用

  // データの書き込み
  uint32_t *mappedIndex = nullptr;
  indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&mappedIndex));
  std::memcpy(mappedIndex, modelData_.indices.data(), sizeInBytes);
  indexResource_->Unmap(0, nullptr);
}

// 頂点バッファの作成
void Model::CreateVertexResource() {

#pragma region リソースとバッファビューの作成
  // リソースとバッファビューの作成

  // VertexResourceを作る
  vertexResource_ = DX12Context::GetInstance()->CreateBufferResource(
      sizeof(VertexData) * modelData_.vertices.size());

  // VertexBufferViewを作成する
  // リソースの先頭アドレスから使う
  vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
  // 使用するリソースのサイズは頂点6つ分のサイズ
  vertexBufferView_.SizeInBytes =
      UINT(sizeof(VertexData) * modelData_.vertices.size());
  // １頂点当たりのサイズ
  vertexBufferView_.StrideInBytes = sizeof(VertexData);

#pragma endregion ここまで

#pragma region VertexDataの設定
  // VertexDataの設定

  // 書き込むためのアドレスを取得
  vertexResource_->Map(
      0, nullptr,
      reinterpret_cast<void **>(&vertexData_)); // 書き込むためのアドレスを取得
  std::memcpy(vertexData_, modelData_.vertices.data(),
              sizeof(VertexData) *
                  modelData_.vertices.size()); // 頂点データをリソースにコピー

#pragma endregion ここまで
}

// マテリアルバッファの作成
void Model::CreateMaterialResource() {

#pragma region マテリアルリソースの作成
  // マテリアルリソースの作成

  // マテリアルリソースを作る
  materialResource_ =
      DX12Context::GetInstance()->CreateBufferResource(sizeof(Material));

#pragma endregion ここまで

#pragma region MaterialDataの設定
  // MaterialDataの設定

  // MaterialResourceにデータを書き込むためのアドレスを取得してMaterialDataに割り当てる
  materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
  // マテリアルデータの初期値を書き込む
  materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  // ライティングを無効に設定
  materialData_->enableLighting = true;
  // UV変換行列を単位行列に設定
  materialData_->uvTransform = MakeIdentity4x4();
  // 光沢度を設定
  materialData_->shininess = 10.0f;
  // 環境マップ係数を設定
  materialData_->environmentCoefficient = 0.0f;

#pragma endregion ここまで
}
