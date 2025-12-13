#pragma once
#include <array>
#include <d3d12.h>

namespace BlendMode {

// BlendState
enum BlendState {
  kBlendModeNone,     // ブレンド無し
  kBlendModeNormal,   // 通常aブレンド
  kBlendModeAdd,      // 加算
  kBlendModeSubtract, // 減算
  kBlendModeMultiply, // 乗算
  kBlendModeScreen,   // スクリーン
  kCountOfBlendMode,  // 利用してはいけない (要素数)
};

// ブレンド設定デスクリプタの配列を生成する関数を宣言
// 汎用的なユーティリティ関数
std::array<D3D12_BLEND_DESC, kCountOfBlendMode> CreateBlendStateDescs();

} // namespace BlendMode