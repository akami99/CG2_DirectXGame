#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "BlendMode/BlendMode.h"

// 前方宣言
class DX12Context;
class PipelineManager;
class Camera;

// 3Dオブジェクト共通部
class Object3dCommon {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  // DirectXデバイス
  DX12Context *dxBase_ = nullptr;

  // グラフィックスパイプラインステート (ブレンドモードごとに配列で保持)
  ComPtr<ID3D12PipelineState>
      psoArray_[BlendMode::BlendState::kCountOfBlendMode];

  // デフォルトカメラ
  Camera *defaultCamera_ = nullptr;

public: // メンバ関数
  // 初期化
  void Initialize(DX12Context *dxBase, PipelineManager *pipelineManager);

  // 共通描画設定
  void SetCommonDrawSettings(BlendMode::BlendState currentBlendMode,
                             PipelineManager *pipelineManager);
  // getter

  // デフォルトカメラの取得
  Camera *GetDefaultCamera() const { return defaultCamera_; };

  // DirectX基底部分を取得
  DX12Context *GetDX12Context() const { return dxBase_; }

  // setter

  // デフォルトカメラの設定
  void SetDefaultCamera(Camera *camera) { defaultCamera_ = camera; };
};
