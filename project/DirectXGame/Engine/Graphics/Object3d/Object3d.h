#pragma once

#include "Types/GraphicsTypes.h"
#include "Types/LightTypes.h"

#include <d3d12.h>
#include <string>
#include <wrl/client.h>

// 前方宣言
class Object3dCommon;
class Model;
class Camera;

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
  // モデルのポインタ
  Model *model_ = nullptr;
  // カメラのポインタ
  Camera *camera_ = nullptr;

  // 変換行列データ
  // バッファリソース
  ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData_ = nullptr;

  // 平行光源データ
  // バッファリソース
  ComPtr<ID3D12Resource> directionalLightResource_;
  // バッファリソース内のデータを表すポインタ
  DirectionalLight *directionalLightData_ = nullptr;

  // 点光源データ
  // バッファリソース
  ComPtr<ID3D12Resource> pointLightResource_;
  // バッファリソース内のデータを表すポインタ
  PointLight *pointLightData_ = nullptr;

  // Transform変数を作る
  Transform transform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

public: // メンバ関数
  // 初期化
  void Initialize(Object3dCommon *object3dCommon);

  // 更新
  void Update();

  // 描画処理
  void Draw();

private: // 作成関数
  // 変換行列バッファの作成
  void CreateTransformationMatrixResource();

  // 平行光源バッファの作成
  void CreateDirectionalLightResource();

  // 点光源バッファの作成
  void CreatePointLightResource();

public: // getter
  // 座標の取得
  const Vector3 &GetTranslate() const { return transform_.translate; }
  // 回転の取得
  const Vector3 &GetRotation() const { return transform_.rotate; }
  // スケールの取得
  const Vector3 &GetScale() const { return transform_.scale; }

  // DirectionalLight 
  // 色の取得
  const Vector4 &GetDirectionalLightColor() const {
    return directionalLightData_->color;
  }
  // 方向の取得
  const Vector3 &GetDirectionalLightDirection() const {
    return directionalLightData_->direction;
  }
  // 輝度の取得
  const float &GetDirectionalLightIntensity() const {
    return directionalLightData_->intensity;
  }

  // PointLight
  // 色の設定
  const Vector4 &GetPointLightColor() const {
      return pointLightData_->color;
  }
  // 位置の設定
  const Vector3 &GetPointLightPosition() const {
      return pointLightData_->position;
  }
  // 輝度の設定
  const float &GetPointLightIntensity() const {
      return pointLightData_->intensity;
  }
  /* const float &GetPointLightRadius() const { return pointLightData_->radius; }
   const float &GetPointLightDecay() const { return pointLightData_->decay; }*/

public: // setter
  // モデルの設定
  void SetModel(Model *model) { model_ = model; };
  void SetModel(const std::string &filepath);

  // カメラの設定
  void SetCamera(Camera *camera) { camera_ = camera; };

  // 座標の設定
  void SetTranslate(const Vector3 &translate) {
    transform_.translate = translate;
  }
  // 回転の設定
  void SetRotation(const Vector3 &rotation) { transform_.rotate = rotation; }
  // スケールの設定
  void SetScale(const Vector3 &scale) { transform_.scale = scale; }

  // DirectionalLight
  // 色の設定
  void SetDirectionalLightColor(const Vector4 &color) {
    directionalLightData_->color = color;
  }
  // 方向の設定
  void SetDirectionalLightDirection(const Vector3 &direction) {
    directionalLightData_->direction = direction;
  }
  // 輝度の設定
  void SetDirectionalLightIntensity(const float &intensity) {
    directionalLightData_->intensity = intensity;
  }

  // PointLight
  // 色の設定
  void SetPointLightColor(const Vector4 &color) {
    pointLightData_->color = color;
  }
  // 位置の設定
  void SetPointLightPosition(const Vector3 &pos) {
    pointLightData_->position = pos;
  }
  // 輝度の設定
  void SetPointLightIntensity(float intensity) {
    pointLightData_->intensity = intensity;
  }
  /*void SetPointLightRadius(float radius) { pointLightData_->radius = radius; }
  void SetPointLightDecay(float decay) { pointLightData_->decay = decay; }*/
};
