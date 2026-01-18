#pragma once
#include <Windows.h>

// 前方宣言
class Win32Window;
class DX12Context;
class Input;
class PipelineManager;
class SrvManager;
class ImGuiManager;
class SpriteCommon;
class Object3dCommon;

// ゲーム全体を管理するフレームワーククラス
class RAFramework {
public:
    virtual ~RAFramework() = default;

    // 実行関数
    void Run();

    // 継承先で実装する関数
    virtual void Initialize();
    virtual void Finalize();
    virtual void Update();
    virtual void Draw() = 0; // 描画だけは絶対に個別の実装が必要なので純粋仮想関数

    // 終了フラグ
    virtual bool IsEndRequest() {
        return endRequest_;
    }

protected: // 継承先の MyGame でも使えるように protected にする
    bool endRequest_ = false;

    // --- 基盤システム (これらはFrameworkが持つ) ---
    Win32Window *window_ = nullptr;
    DX12Context *dxBase_ = nullptr;
    Input *input_ = nullptr;
    SrvManager *srvManager_ = nullptr;
    PipelineManager *pipelineManager_ = nullptr;
    ImGuiManager *imGuiManager_ = nullptr;

    // --- 描画共通システム ---
    SpriteCommon *spriteCommon_ = nullptr;
    Object3dCommon *object3dCommon_ = nullptr;
};
