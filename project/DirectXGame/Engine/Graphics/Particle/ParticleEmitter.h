#pragma once

#include "GraphicsTypes.h"

#include <string>

class ParticleEmitter {
public:
  // エミッターの設定値 (ご提示のコードから推測)
  Transform transform; // 位置、回転、スケール
  uint32_t count;      // 1回で発生させる個数
  float frequency;     // 発生頻度 (秒)
  float frequencyTime; // 頻度計算用の時刻 (初期値 0.0f)

  /// <summary>
  /// ParticleEmitterのコンストラクタ
  /// </summary>
  /// <param name="t">エミッターの初期トランスフォーム</param>
  /// <param name="c">1回で生成するパーティクル数</param>
  /// <param name="freq">発生頻度 (秒)</param>
  ParticleEmitter(const Transform &t, uint32_t c, float freq)
      : transform(t), count(c), frequency(freq), frequencyTime(0.0f) {}

  // デフォルトコンストラクタ
  ParticleEmitter() = default;

  void Emit(const std::string &groupName);
};
