#pragma once
#include <cstdint>

#ifndef APPLICATION_CONFIG_H
#define APPLICATION_CONFIG_H

// クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

// その他のアプリケーション全体に関わる設定
static const float kDeltaTime = 1.0f / 60.0f;

#endif // APPLICATION_CONFIG_H