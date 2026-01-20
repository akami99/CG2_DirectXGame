#pragma once

#include "../Math/MathTypes.h"

#ifndef LIGHT_TYPES_H
#define LIGHT_TYPES_H

// DirectionalLightの構造体
struct DirectionalLight {
  Vector4 color;     // ライトの色
  Vector3 direction; // ライトの向き(正規化する)
  float _padding;    // 16バイトのアライメントを確保するためのパディング
  float intensity;   // 輝度
}; // Vector4(16)+Vector3(12)+float(4)=32バイト + float(4)=36バイト

// PointLightの構造体
struct PointLight {
  Vector4 color;     // ライトの色
  Vector3 position;  // ライトの位置
  float intensity;   // 輝度
  float radius;      // 届く最大距離
  float decay;       // 減衰率
  float _padding[2]; // 16バイトのアライメントを確保するためのパディング
}; // Vector4(16)+Vector3(12)+float(4)+float(4)+float(4)+float[2](8)=48バイト

// SpotLightの構造体
struct SpotLight {
  Vector4 color;         // ライトの色
  Vector3 position;      // ライトの位置
  float intensity;       // 輝度
  Vector3 direction;     // ライトの向き
  float distance;        // ライトの届く距離
  float decay;           // 減衰率
  float cosAngle;        // スポットライトのコサイン角度
  float cosFalloffStart; // スポットライトの減衰開始コサイン角度
  float _padding;        // 16バイトのアライメントを確保するためのパディング
}; // Vector4(16)+Vector3(12)+float(4)+Vector3(12)+float(4)+float(4)+float(4)+float[2](8)=64バイト

#endif // LIGHT_TYPES_H