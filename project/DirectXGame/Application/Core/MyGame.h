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

private:
    struct PostProcessParams {
        float intensity;
    };

    std::unique_ptr<RenderTexture> renderTexture_;
    
    Microsoft::WRL::ComPtr<ID3D12PipelineState> postProcessPSO_;
    Microsoft::WRL::ComPtr<ID3D12Resource> postProcessParamsResource_;
    PostProcessParams* postProcessParamsData_ = nullptr;
};
