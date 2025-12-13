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

#endif // LIGHT_TYPES_H