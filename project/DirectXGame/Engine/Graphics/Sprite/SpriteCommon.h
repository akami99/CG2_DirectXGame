#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <array> // std::array用に必要

#include "BlendMode/BlendMode.h"

// スプライト共通部
class SpriteCommon {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数

    static SpriteCommon *instance_;

  // グラフィックスパイプラインステート (ブレンドモードごとに配列で保持)
  ComPtr<ID3D12PipelineState>
      psoArray_[BlendMode::BlendState::kCountOfBlendMode];

  // その他共通設定
  D3D12_RASTERIZER_DESC rasterizerDesc_{};

public: // シングルトンインスタンス取得
    static SpriteCommon* GetInstance();

public: // メンバ関数
  // 初期化
  void Initialize();

  // 終了
  void Finalize();

  // 共通描画設定
  void SetCommonDrawSettings(BlendMode::BlendState currentBlendMode);

private: // コンストラクタ周り
    SpriteCommon() = default;
    ~SpriteCommon() = default;
    SpriteCommon(const SpriteCommon&) = delete;
    const SpriteCommon& operator=(const SpriteCommon&) = delete;
};
