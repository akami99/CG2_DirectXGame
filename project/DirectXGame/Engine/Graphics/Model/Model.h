#pragma once

#include "../ModelTypes.h"

#include <d3d12.h>
#include <wrl/client.h>

// 前方宣言
class ModelCommon;

// 3Dモデル
class Model {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  // ModelCommonのポインタ
  ModelCommon *modelCommon_ = nullptr;

  // Objのファイルデータ
  ModelData modelData_{};

  // 頂点データ

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
  void Initialize(ModelCommon *modelCommon, const std::string &directoryPath,
                  const std::string &filename);

  // 描画処理
  void Draw();

  // getter

  // 色の取得
  const Vector4 &GetColor() const { return materialData_->color; }

  // ライティングが有効かどうか
  const int32_t &IsEnableLighting() const {
    return materialData_->enableLighting;
  }

  // setter

  // 色の設定
  void SetColor(const Vector4 &color) { materialData_->color = color; }

  // ライティングが有効かを設定
  void SetEnableLighting(const int32_t enableLighting) {
    materialData_->enableLighting = enableLighting;
  }

private: // メンバ関数
  // .mtlファイルの読み取り
  static MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                               const std::string &fileName);
  // .objファイルの読み取り
  void LoadObjFile(const std::string &directoryPath,
                   const std::string &fileName);

  // 頂点データの作成
  void CreateVertexResource();

  // マテリアルバッファの作成
  void CreateMaterialResource();
};
