#pragma once

#include "../GraphicsTypes.h"

#include <d3d12.h>
#include <wrl/client.h>

// カメラ
class Camera {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  Transform transform_{};
  // ワールド行列
  Matrix4x4 worldMatrix_{};
  // ビュー行列
  Matrix4x4 viewMatrix_{};
  // 透視投影行列
  Matrix4x4 projectionMatrix_{};
  // 水平方向視野角
  float fovY_{};
  // アスペクト比
  float aspectRatio_{};
  // ニアクリップ距離
  float nearClip_{};
  // ファークリップ距離
  float farClip_{};
  // ビュープロジェクション行列
  Matrix4x4 viewProjectionMatrix_{};

public: // メンバ関数
  // コンストラクタ
  Camera();

  // 更新
  void Update();

  // getter

  // ワールド行列の取得
  const Matrix4x4 &GetWorldMatrix() const { return worldMatrix_; }
  // ビュー行列の取得
  const Matrix4x4 &GetViewMatrix() const { return viewMatrix_; }
  // 透視投影行列の取得
  const Matrix4x4 &GetProjectionMatrix() const { return projectionMatrix_; }
  // ビュープロジェクション行列の取得
  const Matrix4x4 &GetViewProjectionMatrix() const {
    return viewProjectionMatrix_;
  }
  // 回転の取得
  const Vector3 &GetRotate() const { return transform_.rotate; }
  // 座標の取得
  const Vector3 &GetTranslate() const { return transform_.translate; }

  // setter

  // 回転の設定
  void SetRotate(const Vector3 &rotate) { transform_.rotate = rotate; }
  // 座標の設定
  void SetTranslate(const Vector3 &translate) {
    transform_.translate = translate;
  }
  // 水平方向視野角の設定
  void SetFovY(const float &fovY) { fovY_ = fovY; }
  // アスペクト比の設定
  void SetAspectRatio(const float &aspectRatio) { aspectRatio_ = aspectRatio; }
  // ニアクリップ距離の設定
  void SetNearClip_(const float &nearClip) { nearClip_ = nearClip; }
  // ファークリップ距離の設定
  void SetFarClip(const float &farClip) { farClip_ = farClip; }
};
