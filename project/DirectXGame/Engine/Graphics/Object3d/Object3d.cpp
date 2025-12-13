#include "Object3d.h"
#include "../Input/Input.h"
#include "API/DX12Context.h"
#include "Object3dCommon.h"
#include "Texture/TextureManager.h"

#include "Math/Functions/MathUtils.h"
#include "Math/Matrix/MatrixGenerators.h"

#include <assert.h>
#include <fstream>
#include <sstream>
#include <string>

using namespace Microsoft::WRL;
using namespace MathUtils;
using namespace MathGenerators;

void Object3d::Initialize(Object3dCommon *object3dCommon) {
  // 引数で受け取ってメンバ変数に記録する
  object3dCommon_ = object3dCommon;

  // モデル読み込み
  LoadObjFile("plane", "plane.obj");

  // 頂点データを作成する
  CreateVertexResource();

  // マテリアルバッファの作成
  CreateMaterialResource();

  // 変換行列バッファの作成
  CreateTransformationMatrixResource();

  // 平行光源バッファの作成
  CreateDirectionalLightResource();

  // .objの参照しているテクスチャファイル読み込み、インデックスを代入
  modelData_.material.textureIndex = TextureManager::GetInstance()->LoadTexture(
      modelData_.material.textureFilePath);
}

// 更新処理
void Object3d::Update() {

#pragma region 変換行列データの設定
  // Transform情報を作る

  // Transform情報から変換行列を作る
  Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate);
  // cameraTransformからcameraMatrixを作る
  Matrix4x4 cameraMatrix =
      MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate,
                       cameraTransform_.translate);
  // cameraMatrixからViewMatrixを作る
  Matrix4x4 viewMatrix = Inverse(cameraMatrix);
  // ProjectionMatrixを作って透視投影行列を書き込む
  Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
      0.45f,
      float(Win32Window::kClientWidth) / float(Win32Window::kClientHeight),
      0.1f, 100.0f);
  Matrix4x4 wvpMatrix = worldMatrix * viewMatrix * projectionMatrix;
  transformationMatrixData_->WVP =
      wvpMatrix; // World-View-Projection行列をWVPメンバーに入れる
  transformationMatrixData_->World =
      worldMatrix; // 純粋なワールド行列をWorldメンバーに入れる

#pragma endregion ここまで
}

// 描画処理
void Object3d::Draw() {
  // VertexBufferの設定
  object3dCommon_->GetDX12Context()->GetCommandList()->IASetVertexBuffers(
      0, 1, &vertexBufferView_);
  // マテリアルCBVの設定
  object3dCommon_->GetDX12Context()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          0, materialResource_->GetGPUVirtualAddress());
  // 変換行列CBVの設定
  object3dCommon_->GetDX12Context()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          1, transformationMatrixResource_->GetGPUVirtualAddress());
  // SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
  object3dCommon_->GetDX12Context()
      ->GetCommandList()
      ->SetGraphicsRootDescriptorTable(
          2, TextureManager::GetInstance()->GetSrvHandleGPU(
                 modelData_.material.textureIndex));
  // DirectionalLightのCBufferの場所を設定 (PS b1, rootParameter[3]に対応)
  object3dCommon_->GetDX12Context()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          3,
          directionalLightResource_ // directionalLightResourceはライトのCBV
              ->GetGPUVirtualAddress());
  // 描画コマンド
  object3dCommon_->GetDX12Context()->GetCommandList()->DrawInstanced(
      UINT(modelData_.vertices.size()), 1, 0, 0);
}

// .mtlファイルの読み取り
MaterialData
Object3d::LoadMaterialTemplateFile(const std::string &directoryPath,
                                   const std::string &fileName) {
  // 中で必要となる変数の宣言
  MaterialData materialData; // 構築するMaterialData
  std::string line;          // ファイルから読んだ1行を格納するもの
  // ファイルを開く
  std::ifstream file(directoryPath + "/" + fileName); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める
  // 実際にファイルを読み、MaterialDataを構築していく
  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier; // 先頭の識別子を読む

    // identifierに応じた処理
    if (identifier == "map_Kd") { // テクスチャファイルパス
      std::string textureFilename;
      s >> textureFilename;
      // 連結してテクスチャファイルパスにする
      materialData.textureFilePath = textureFilename;
    }
  }

  // MaterialDataを返す
  return materialData;
}

// .objファイルの読み取り
void Object3d::LoadObjFile(const std::string &directoryPath,
                           const std::string &fileName) {
  // 中で必要となる変数の宣言
  std::vector<Vector4> positions; // 位置
  std::vector<Vector3> normals;   // 法線
  std::vector<Vector2> texcoords; // テクスチャ座標
  std::string line;               // ファイルから読んだ1行を格納するもの
  // ファイルを開く
  std::string fullPath = directoryPath;
  // 既にパスに"DirectXGame/Resources/Assets/Sounds/"が含まれている場合は追加しない
  if (directoryPath.find("Resources/Assets/Models/") == std::string::npos &&
      directoryPath.find("resources/assets/models/") == std::string::npos) {
    fullPath = "Resources/Assets/Models/" + directoryPath;
  }

  std::ifstream file(fullPath + "/" + fileName); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める
  // 実際にファイルを読み、ModelDataを構築していく
  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier; // 先頭の識別子を読む

    // identifierに応じた処理
    if (identifier == "v") { // 頂点位置
      Vector4 position;
      s >> position.x >> position.y >> position.z; // x, y, zを読む
      position.w = 1.0f;                           // wは1.0fに設定
      positions.push_back(position);               // positionsに追加
    } else if (identifier == "vt") {               // テクスチャ座標
      Vector2 texcoord;
      s >> texcoord.x >> texcoord.y;  // x, yを読む
      texcoord.y = 1.0f - texcoord.y; // Y軸を反転させる
      texcoords.push_back(texcoord);  // texcoordsに追加
    } else if (identifier == "vn") {  // 法線ベクトル
      Vector3 normal;
      s >> normal.x >> normal.y >> normal.z; // x, y, zを読む
      normals.push_back(normal);             // normalsに追加
    } else if (identifier == "f") {          // 面情報
      VertexData triangle[3];
      // 面は三角形限定。その他は未対応
      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
        std::string vertexDefinition;
        s >> vertexDefinition;
        // 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
        std::istringstream v(vertexDefinition);
        uint32_t elementIndices[3];
        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
          elementIndices[element] = std::stoi(index);
        }
        // 要素へのIndexから、実際の要素の値を取得して、頂点を読んでいく
        Vector4 position =
            positions[elementIndices[0] - 1]; // OBJは1-indexedなので-1する
        // position.x *= -1.0f; // X軸を反転する
        Vector2 texcoord =
            texcoords[elementIndices[1] - 1]; // OBJは1-indexedなので-1する
        Vector3 normal =
            normals[elementIndices[2] - 1]; // OBJは1-indexedなので-1する
        // normal.x *= -1.0f; // X軸を反転する
        VertexData vertex = {position, texcoord, normal}; // 頂点データを構築
        modelData_.vertices.push_back(vertex);            // ModelDataに追加
        triangle[faceVertex] = {position, texcoord, normal};
      }
      // 頂点を逆順で登録することで、周り順を逆にする
      modelData_.vertices.push_back(triangle[2]);
      modelData_.vertices.push_back(triangle[1]);
      modelData_.vertices.push_back(triangle[0]);
    } else if (identifier == "mtllib") {
      // materialTemplateLibraryファイルの名前を取得する
      std::string materialFileName;
      s >> materialFileName;
      // 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
      modelData_.material =
          LoadMaterialTemplateFile(fullPath, materialFileName);
    }
  }
}

// 頂点バッファの作成
void Object3d::CreateVertexResource() {

#pragma region リソースとバッファビューの作成
  // リソースとバッファビューの作成

  // VertexResourceを作る
  vertexResource_ = object3dCommon_->GetDX12Context()->CreateBufferResource(
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
void Object3d::CreateMaterialResource() {

#pragma region マテリアルリソースの作成
  // マテリアルリソースの作成

  // マテリアルリソースを作る
  materialResource_ =
      object3dCommon_->GetDX12Context()->CreateBufferResource(sizeof(Material));

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

#pragma endregion ここまで
}

// 変換行列バッファの作成
void Object3d::CreateTransformationMatrixResource() {
#pragma region 変換行列リソースの作成
  // 変換行列リソースの作成

  // 変換行列リソースを作る
  transformationMatrixResource_ =
      object3dCommon_->GetDX12Context()->CreateBufferResource(
          sizeof(TransformationMatrix));

#pragma endregion ここまで

#pragma region TransformationMatrixDataの設定
  // TransformationMatrixDataの設定

  // TransformationMatrixResourceにデータを書き込むためのアドレスを取得してTransformationMatrixDataに割り当てる
  // 書き込むためのアドレスを取得
  transformationMatrixResource_->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));
  // 単位行列を書き込んでおく
  transformationMatrixData_->WVP = MakeIdentity4x4();
  transformationMatrixData_->World = MakeIdentity4x4();

#pragma endregion ここまで
}

// 平行光源バッファの作成
void Object3d::CreateDirectionalLightResource() {
#pragma region 平行光源リソースの作成
  // 平行光源リソースの作成

  // 平行光源リソースを作る
  directionalLightResource_ =
      object3dCommon_->GetDX12Context()->CreateBufferResource(
          sizeof(DirectionalLight));

#pragma endregion ここまで

#pragma region DirectionalLightDataの設定
  // DirectionalLightDataの設定

  // DirectionalLightlResourceにデータを書き込むためのアドレスを取得してDirectionalLightDataに割り当てる
  directionalLightResource_->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData_));
  // デフォルト値を書き込んでおく
  directionalLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLightData_->direction = {0.0f, -1.0f, 0.0f};
  directionalLightData_->intensity = 1.0f;

#pragma endregion ここまで
}
