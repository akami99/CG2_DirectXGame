#pragma once

#include "Types/ModelTypes.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// 3Dモデル
class Model {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  // Objのファイルデータ
  ModelData modelData_{};

  // インデックスデータ用リソース
  ComPtr<ID3D12Resource> indexResource_ = nullptr;
  D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

  // バッファリソース
  ComPtr<ID3D12Resource> vertexResource_ = nullptr;
  // バッファリソース内の使い道を捕捉するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
  // バッファリソース内のデータを表すポインタ
  VertexData *vertexData_ = nullptr;

  // マテリアルデータ

  // バッファリソース
  ComPtr<ID3D12Resource> materialResource_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  Material *materialData_ = nullptr;

public: // メンバ関数
  // 初期化(テクスチャロードでコマンドリスト積んでいるので注意)
  void Initialize(const std::string &directoryPath,
                  const std::string &filename);

  // 描画処理
  void Draw();

  // getter

  // RootNodeを取得するGetter
  const Node& GetRootNode() const { return modelData_.rootNode; }

  // 色の取得
  const Vector4 &GetColor() const { return materialData_->color; }

  // ライティングが有効かどうか
  const int32_t &IsEnableLighting() const {
    return materialData_->enableLighting;
  }

  // 環境マップ係数の取得
  const float &GetEnvironmentCoefficient() const {
    return materialData_->environmentCoefficient;
  }

#ifdef USE_IMGUI
  // 色の参照取得(ImGui用)
  Vector4 &GetColorDebug() { return materialData_->color; }
#endif // USE_IMGUI


  // setter

  // 色の設定
  void SetColor(const Vector4 &color) { materialData_->color = color; }

  // ライティングが有効かを設定
  void SetEnableLighting(const int32_t enableLighting) {
    materialData_->enableLighting = enableLighting;
  }

  // 環境マップ係数の設定
  void SetEnvironmentCoefficient(const float coefficient) {
    materialData_->environmentCoefficient = coefficient;
  }

private: // メンバ関数
  // .objファイルの読み取り
  void LoadModelFile(const std::string &directoryPath,
                   const std::string &fileName);

  // assimpのノードを変換
  Node ReadNode(aiNode* node);

  // インデックスバッファ作成用関数
  void CreateIndexResource();

  // 頂点データの作成
  void CreateVertexResource();

  // マテリアルバッファの作成
  void CreateMaterialResource();
};
