#pragma once

#include "GraphicsTypes.h"

#ifndef PARTICLE_TYPES_H
#define PARTICLE_TYPES_H

// パーティクルの構造体
struct Particle {
  Transform transform; // 変換行列
  Vector3 velocity;    // 速度
  Vector4 color;       // 色
  float lifeTime;      // 生存時間
  float currentTime;   // 経過時間
}; // Transform(36)+Vector3(12)+Vector4(16)=64バイト

// パーティクルインスタンスデータの構造体
struct ParticleInstanceData {
  Matrix4x4 WVP;   // ワールドビュー射影行列
  Matrix4x4 World; // ワールド行列
  Vector4 color;   // 色
}; // Matrix4x4(64)+Vector4(16)=80バイト

// パーティクルのエミッタの構造体
struct Emitter {
  Transform transform; // エミッタのTransform
  uint32_t count;      // 発生数
  float frequency;     // 発生頻度
  float frequencyTime; // 頻度用時刻
                       // 初期色、初速度、生存期間などをいじれたり、
  // emitterの生存期間、emitterの形状の設定、rotate/scaleの利用、範囲の描画もできると使いやすい。
};

// パーティクルの加速フィールドの構造体
struct AccelerationField {
  Vector3 acceleration; // 加速度
  AABB area;            // 範囲
};

#endif // PARTICLE_TYPES_H