#pragma once

#include <cstdint>

#include "../Math/MathTypes.h"

#ifndef GRAPHICS_TYPES_H
#define GRAPHICS_TYPES_H

// 頂点データの構造体
struct VertexData {
  Vector4 position; // ワールド座標系での位置
  Vector2 texcoord; // UV座標
  Vector3 normal;   // 法線ベクトル
}; // 16+8+12=36バイト。float*4+float*2+float*3

// 変換行列を構成するための構造体(s,r,t)
struct Transform {
  Vector3 scale;     // スケーリング
  Vector3 rotate;    // 回転
  Vector3 translate; // 平行移動
}; // Vector3*3=36バイト

// マテリアルの構造体
struct Material {
  Vector4 color;          // マテリアルの色
  int32_t enableLighting; // ライティングの有効・無効フラグ
  float shininess;        // 光沢度
  float padding[2];       // 8バイトのアライメントを確保するためのパディング
  Matrix4x4 uvTransform;  // UV変換行列
};

// 変換行列をまとめた構造体
struct TransformationMatrix {
  Matrix4x4 WVP;   // ワールドビュー射影行列
  Matrix4x4 World; // ワールド行列
  Matrix4x4 WorldInverseTranspose; // ワールド行列の逆行列の転置行列
}; // Matrix4x4*2=128バイト

// Camera用の構造体(16バイト)
struct CameraForGPU {
  Vector3 worldPosition; // カメラのワールド座標
  float padding;          // 4バイトのアライメントを確保するためのパディング
};

#endif // GRAPHICS_TYPES_H