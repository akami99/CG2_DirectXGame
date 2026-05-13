#pragma once
#include "RAFramework.h" // RAFrameworkをインクルード

// 必要な前方宣言やインクルード
#include "DebugCamera.h"
#include "MathTypes.h"
#include <vector>

#include "RenderTexture.h"
#include "Sprite.h"
#include <memory>

// Frameworkを継承する
class MyGame : public RAFramework {
public: // メンバ関数
    // オーバーライドすることを明示
    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;
    
    // ポストエフェクト制御用
    static void SetPostEffectMode(int mode) { postEffectModeNext_ = mode; }
    static void SetPostEffectStrength(float strength) { postEffectStrengthNext_ = strength; }

private:
    // グレースケール・セピア共通の定数バッファ構造体
    struct PostProcessParams {
        float colorScale[3]; // シェーダーの gColorScale に対応 (float3)
        float strength;      // 0.0f: 元の色, 1.0f: フィルター色
    };

    std::unique_ptr<RenderTexture> renderTexture_;
    
    Microsoft::WRL::ComPtr<ID3D12PipelineState> postProcessPSO_;   // CopyImage (無効果)
    Microsoft::WRL::ComPtr<ID3D12PipelineState> colorFilterPSO_;   // グレースケール/セピア
    Microsoft::WRL::ComPtr<ID3D12Resource> postProcessParamsResource_;
    PostProcessParams* postProcessParamsData_ = nullptr;
    // 0: コピー(無効果), 1: グレースケール, 2: セピア
    int postEffectMode_ = 0;

    static int postEffectModeNext_;
    static float postEffectStrengthNext_;
};
