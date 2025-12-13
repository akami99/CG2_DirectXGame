#pragma once

#include <d3d12.h>
#include <wrl/client.h>

// 前方宣言
class DX12Context;

// 3Dモデル共通部
class ModelCommon {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  // DirectXデバイス
  DX12Context *dxBase_ = nullptr;

public: // メンバ関数
  // 初期化
  void Initialize(DX12Context *dxBase);

  // DirectX基底部分を取得
  DX12Context *GetDX12Context() const { return dxBase_; }
};
