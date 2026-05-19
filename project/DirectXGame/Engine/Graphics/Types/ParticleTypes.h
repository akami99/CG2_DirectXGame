#pragma once

#include "Types/GraphicsTypes.h"

#ifndef PARTICLE_TYPES_H
#define PARTICLE_TYPES_H

// パーティクルの構造体
struct Particle {
  Transform transform; // 変換行列
  Vector3 velocity;    // 速度
  Vector4 color;       // 色
  float lifeTime;      // 生存時間
  float currentTime;   // 経過時間

  // 個別UVアニメーション用パラメータ
  Vector2 uvTranslate = { 0.0f, 0.0f };
  float uvRotate = 0.0f;
  Vector2 uvScale = { 1.0f, 1.0f };
};

// パーティクルインスタンスデータの構造体
struct ParticleInstanceData {
  Matrix4x4 WVP;   // ワールドビュー射影行列
  Matrix4x4 World; // ワールド行列
  Vector4 color;   // 色
  Matrix4x4 uvTransform; // 個別のUV変換行列
};

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

// リングプリミティブ用の詳細設定構造体
struct RingSettings {
    bool isRing = false;
    float innerRadius = 0.5f;
    float startOuterRadius = 1.0f;
    float midOuterRadius = 1.0f;
    float endOuterRadius = 1.0f;
    float startAngle = 0.0f;     // 度数法 (0.0f - 360.0f)
    float endAngle = 360.0f;    // 度数法 (0.0f - 360.0f)
    uint32_t division = 32;

    // UVとカラーのアニメーション/ブレンド設定
    bool isUvSwap = false;
    Vector4 innerColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector4 outerColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fadeStartAlpha = 1.0f;
    float fadeEndAlpha = 1.0f;
    float fadeRange = 0.0f;     // 開始/終了地点からのフェード幅 (0.0f - 0.5f)
};

#endif // PARTICLE_TYPES_H