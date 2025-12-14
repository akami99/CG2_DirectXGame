#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>
#include <string>

#include "../../Core/Utility/Math/MathTypes.h"
#include "../GraphicsTypes.h"

// 前方宣言
class SpriteCommon;

// スプライト
class Sprite {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  SpriteCommon *spriteCommon_ = nullptr;

  // 頂点データ
  // バッファリソース
  ComPtr<ID3D12Resource> vertexResource_ = nullptr;
  ComPtr<ID3D12Resource> indexResource_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  VertexData *vertexData_ = nullptr;
  uint32_t *indexData_ = nullptr;
  // バッファリソースの使い道を捕捉するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
  D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

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

  // スプライトの座標
  Vector2 translate_{0.0f, 0.0f};
  // 回転
  float rotation_ = 0.0f;
  // スケール
  Vector2 scale_{640.0f, 640.0f};

  // UV変換行列
  Transform uvTransform_{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // テクスチャファイルパス
  std::string textureFilePath_;

  // アンカーポイント
  Vector2 anchorPoint_ = {0.0f, 0.0f};
  // 左右フリップ
  bool isFlipX_ = false;
  // 上下フリップ
  bool isFlipY_ = false;
  // テクスチャ左上座標
  Vector2 textureLeftTop_ = {0.0f, 0.0f};
  // テクスチャ切り出しサイズ
  Vector2 textureSize_ = {100.0f, 100.0f};

public: // メンバ関数
  // 初期化
  void Initialize(SpriteCommon *spriteCommon, const std::string &filePath);

  // 更新処理
  void Update();

  // 描画処理
  void Draw();

  // getter

  // 座標の取得
  const Vector2 &GetTranslate() const { return translate_; }
  // 回転の取得
  const float &GetRotation() const { return rotation_; }
  // スケールの取得
  const Vector2 &GetScale() const { return scale_; }
  // 　色の取得
  const Vector4 &GetColor() const { return materialData_->color; }
  // UV座標の取得
  const Vector3 &GetUvTranslate() const { return uvTransform_.translate; }
  // UV回転の取得
  const Vector3 &GetUvRotation() const { return uvTransform_.rotate; }
  // UVスケールの取得
  const Vector3 &GetUvScale() const { return uvTransform_.scale; }
  // アンカーポイントの取得
  const Vector2 &GetAnchorPoint() const { return anchorPoint_; }
  // 左右フリップの取得
  const bool &GetFlipX() const { return isFlipX_; }
  // 上下フリップの取得
  const bool &GetFlipY() const { return isFlipY_; }
  // テクスチャ左上座標の取得
  const Vector2 &GetTextureLeftTop() const { return textureLeftTop_; }
  // テクスチャ切り出しサイズの取得
  const Vector2 &GetTextureSize() const { return textureSize_; }

  // setter

  // 座標の設定
  void SetTranslate(const Vector2 &translate) { translate_ = translate; }
  // 回転の設定
  void SetRotation(const float &rotation) { rotation_ = rotation; }
  // スケールの設定
  void SetScale(const Vector2 &scale) { scale_ = scale; }
  // 色の設定
  void SetColor(const Vector4 &color) { materialData_->color = color; }
  // UV座標の設定
  void SetUvTranslate(const Vector3 &uvTranslate) {
    uvTransform_.translate = uvTranslate;
  }
  // UV回転の設定
  void SetUvRotation(const Vector3 &uvRotation) {
    uvTransform_.rotate = uvRotation;
  }
  // UVスケールの設定
  void SetUvScale(const Vector3 &uvScale) { uvTransform_.scale = uvScale; }
  // テクスチャを設定するためのセッター
  void SetTexture(const std::string &filePath);
  // アンカーポイントの設定
  void SetAnchorPoint(const Vector2 &anchorPoint) {
    anchorPoint_ = anchorPoint;
  }
  // 左右フリップの設定
  void SetFlipX(const bool &isFlipX) { isFlipX_ = isFlipX; }
  // 上下フリップの設定
  void SetFlipY(const bool &isFlipY) { isFlipY_ = isFlipY; }
  // テクスチャ左上座標の設定
  void SetTextureLeftTop(const Vector2 &textureLeftTop) {
    textureLeftTop_ = textureLeftTop;
  }
  // テクスチャ切り出しサイズの設定
  void SetTextureSize(const Vector2 &textureSize) {
    textureSize_ = textureSize;
  }

private: // メンバ関数
  // 頂点バッファとインデックスバッファの作成
  void CreateBufferResource();

  // マテリアルバッファの作成
  void CreateMaterialResource();

  // 変換行列バッファの作成
  void CreateTransformationMatrixResource();

  // テクスチャサイズをイメージに合わせる
  void AdjustTextureSize();
};
