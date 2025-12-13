#pragma once

#include "../LightTypes.h"
#include "../ModelTypes.h"

#include <d3d12.h>
#include <wrl/client.h>

// 前方宣言
class Object3dCommon;

// 3Dオブジェクト
class Object3d {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  // 共通部のポインタ
  Object3dCommon *object3dCommon_ = nullptr;

  // Objのファイルデータ
  ModelData modelData_;

  // 頂点データ
  // バッファリソース
  ComPtr<ID3D12Resource> vertexResource_ = nullptr;
  // バッファリソース内のデータを表すポインタ
  VertexData *vertexData_ = nullptr;
  // バッファリソース内の使い道を捕捉するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

  // マテリアルデータ
  // バッファリソース
  ComPtr<ID3D12Resource> materialResource_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  Material *materialData_ = nullptr;

  // 変換行列データ
  // バッファリソース
  ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData_ = nullptr;

  // バッファリソース
  ComPtr<ID3D12Resource> directionalLightResource_;
  // バッファリソース内のデータを表すポインタ
  DirectionalLight *directionalLightData_ = nullptr;

  // Transform変数を作る
  Transform transform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  Transform cameraTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};

public: // メンバ関数
  // 初期化(テクスチャロードでコマンドリスト積んでいるので注意)
  void Initialize(Object3dCommon *object3dCommon);

  // 更新
  void Update();

  // 描画処理
  void Draw();

  // getter

  // 座標の取得
  const Vector3 &GetTranslate() const { return transform_.translate; }
  // 回転の取得
  const Vector3 &GetRotation() const { return transform_.rotate; }
  // スケールの取得
  const Vector3 &GetScale() const { return transform_.scale; }
  // 色の取得
  const Vector4 &GetColor() const { return materialData_->color; }

  // カメラの座標の取得
  const Vector3 &GetCameraTranslate() const {
    return cameraTransform_.translate;
  }
  // カメラの回転の取得
  const Vector3 &GetCameraRotation() const { return cameraTransform_.rotate; }

  // ライティングが有効かどうか
  const int32_t &IsEnableLighting() const {
    return materialData_->enableLighting;
  }
  // DirectionalLightの色の取得
  const Vector4 &GetDirectionalLightColor() const {
    return directionalLightData_->color;
  }
  // DirectionalLightの方向の取得
  const Vector3 &GetDirectionalLightDirection() const {
    return directionalLightData_->direction;
  }
  // DirectionalLightの輝度の取得
  const float &GetDirectionalLightIntensity() const {
    return directionalLightData_->intensity;
  }

  // setter

  // 座標の設定
  void SetTranslate(const Vector3 &translate) {
    transform_.translate = translate;
  }
  // 回転の設定
  void SetRotation(const Vector3 &rotation) { transform_.rotate = rotation; }
  // スケールの設定
  void SetScale(const Vector3 &scale) { transform_.scale = scale; }
  // 色の設定
  void SetColor(const Vector4 &color) { materialData_->color = color; }

  // カメラの座標の設定
  void SetCameraTranslate(const Vector3 &translate) {
    cameraTransform_.translate = translate;
  }
  // カメラの回転の設定
  void SetCameraRotation(const Vector3 &rotation) {
    cameraTransform_.rotate = rotation;
  }

  // ライティングが有効かを設定
  void SetEnableLighting(const int32_t enableLighting) {
    materialData_->enableLighting = enableLighting;
  }
  // DirectionalLightの色の設定
  void SetDirectionalLightColor(const Vector4 &color) {
    directionalLightData_->color = color;
  }
  // DirectionalLightの方向の設定
  void SetDirectionalLightDirection(const Vector3 &direction) {
    directionalLightData_->direction = direction;
  }
  // DirectionalLightの輝度の設定
  void SetDirectionalLightIntensity(const float &intensity) {
    directionalLightData_->intensity = intensity;
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

  // 変換行列バッファの作成
  void CreateTransformationMatrixResource();

  // 平行光源バッファの作成
  void CreateDirectionalLightResource();
};
