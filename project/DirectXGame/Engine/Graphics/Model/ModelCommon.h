#pragma once

#include <d3d12.h>
#include <wrl/client.h>

// 3Dモデル共通部(基本何もしていないので、現状では使っていない)
class ModelCommon {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

public: // シングルトン取得
    static ModelCommon* GetInstance();

public: // メンバ関数
  // 初期化
  void Initialize();
};
