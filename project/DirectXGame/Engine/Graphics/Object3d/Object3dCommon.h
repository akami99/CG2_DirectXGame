#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <array> // std::array用に必要

#include "BlendMode/BlendMode.h"

// 前方宣言
class Camera;

// 3Dオブジェクト共通部
class Object3dCommon {
private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  static Object3dCommon * instance_;

  // デフォルトカメラ
  Camera* defaultCamera_ = nullptr;

  // グラフィックスパイプラインステート (ブレンドモードごとに配列で保持)
  ComPtr<ID3D12PipelineState>
      psoArray_[BlendMode::BlendState::kCountOfBlendMode];

public: // シングルトンインスタンス取得
    static Object3dCommon* GetInstance();

public: // メンバ関数
  // 初期化
  void Initialize();

  // 終了
  void Finalize();

  // 共通描画設定
  void SetCommonDrawSettings(BlendMode::BlendState currentBlendMode);
  // getter

  // デフォルトカメラの取得
  Camera* GetDefaultCamera() const { return defaultCamera_; };

  // setter

  // デフォルトカメラの設定(最初に設定しておく用)
  void SetDefaultCamera(Camera *camera) { defaultCamera_ = camera; };

private: // コンストラクタ周り
    Object3dCommon() = default;
    ~Object3dCommon() = default;
    Object3dCommon(const Object3dCommon&) = delete;
    const Object3dCommon& operator=(const Object3dCommon&) = delete;
};
