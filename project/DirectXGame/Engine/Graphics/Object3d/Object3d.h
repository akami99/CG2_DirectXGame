#pragma once

#include "Types/GraphicsTypes.h"
#include "Types/LightTypes.h"

#include <d3d12.h>
#include <string>
#include <wrl/client.h>

// 前方宣言
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
  // モデルのポインタ
  Model *model_ = nullptr;
  // カメラのポインタ
  Camera *camera_ = nullptr;

  // 変換行列データ
  // バッファリソース
  ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData_ = nullptr;

  // Transform変数を作る
  Transform transform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

public: // メンバ関数
    ~Object3d();
  // 初期化
  void Initialize();

  // 更新
  void Update();

  // 描画処理
  void Draw();

private: // 作成関数
  // 変換行列バッファの作成
  void CreateTransformationMatrixResource();

public: // getter
  
  // 変換行列の取得
  const Transform &GetTransform() const { return transform_; }
  // 座標の取得
  const Vector3 &GetTranslate() const { return transform_.translate; }
  // 回転の取得
  const Vector3 &GetRotation() const { return transform_.rotate; }
  // スケールの取得
  const Vector3 &GetScale() const { return transform_.scale; }

#ifdef USE_IMGUI
  // モデルの取得
  Model &GetModelDebug() const { return *model_; }
  // ImGui用の参照ゲッター
  Transform &GetTransformDebug() { return transform_; }
#endif

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
};
