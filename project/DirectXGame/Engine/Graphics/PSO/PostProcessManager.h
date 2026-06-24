#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include "Types/GraphicsTypes.h"

class RenderTexture;

/// @brief ポストプロセス（グレースケール・セピア等）の PSO 切り替えと
///        定数バッファ更新をカプセル化するシングルトン管理クラス
class PostProcessManager {
public:
    // --- ポストエフェクトモード定数 ---
    static constexpr int kModeCopy      = 0; // パススルー（無効果）
    static constexpr int kModeGrayscale = 1;
    static constexpr int kModeSepia     = 2;
    static constexpr int kModeVignette  = 3;
    static constexpr int kModeSmoothing = 4;
    static constexpr int kModeGaussianBlur = 5;
    static constexpr int kModeOutline = 6;
    static constexpr int kModeRadialBlur = 7;
    static constexpr int kModeDissolve = 8;
    static constexpr int kModeRandom = 9;

    // ---- Passkey Idiom ----
    struct Token {
    private:
        friend class PostProcessManager;
        Token() {}
    };

private:
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // グレースケール/セピア共通の定数バッファ構造体
    struct PostProcessParams {
        float colorScale[3]; // gColorScale に対応 (float3)
        float strength;      // 0.0f: 元の色, 1.0f: フィルター色
    };

    // ビネット用の定数バッファ構造体
    struct VignetteParams {
        float scale;
        float exponent;
    };

    // 平滑化用の定数バッファ構造体
    struct SmoothingParams {
        int kernelSize;
        float padding[3]; // 16バイトアライメント
    };

    // ガウシアンフィルター用の定数バッファ構造体
    struct GaussianBlurParams {
        int kernelSize;
        float sigma;
        float padding[2]; // 16バイトアライメント
    };

    // アウトライン用の定数バッファ構造体
    struct OutlineParams {
        Matrix4x4 projectionInverse;
        float edgeMultiplier;
        float padding[3]; // 16バイトアライメント
    };

    // Radial Blur用の定数バッファ構造体
    struct RadialBlurParams {
        float center[2];
        float blurWidth;
        int sampleCount;
    };

    // Dissolve用の定数バッファ構造体
    struct DissolveParams {
        float edgeColor[3];
        float threshold;
        float edgeRange;
        float padding[3];
    };

    // Random用の定数バッファ構造体
    struct RandomParams {
        float time;
        float strength;
        float padding[2]; // 16バイトアライメント
    };

    ComPtr<ID3D12PipelineState> postProcessPSO_; // CopyImage (passthrough)
    ComPtr<ID3D12PipelineState> colorFilterPSO_; // グレースケール/セピア
    ComPtr<ID3D12PipelineState> vignettePSO_;    // ビネット
    ComPtr<ID3D12PipelineState> smoothingPSO_;   // 平滑化
    ComPtr<ID3D12PipelineState> gaussianBlurPSO_; // ガウシアンフィルター
    ComPtr<ID3D12PipelineState> outlinePSO_;      // アウトライン
    ComPtr<ID3D12PipelineState> radialBlurPSO_;   // Radial Blur
    ComPtr<ID3D12PipelineState> dissolvePSO_;     // Dissolve
    ComPtr<ID3D12Resource> paramsResource_;
    PostProcessParams* paramsMapped_ = nullptr;

    ComPtr<ID3D12Resource> vignetteParamsResource_;
    VignetteParams* vignetteParamsMapped_ = nullptr;

    ComPtr<ID3D12Resource> smoothingParamsResource_;
    SmoothingParams* smoothingParamsMapped_ = nullptr;

    ComPtr<ID3D12Resource> gaussianBlurParamsResource_;
    GaussianBlurParams* gaussianBlurParamsMapped_ = nullptr;

    ComPtr<ID3D12Resource> outlineParamsResource_;
    OutlineParams* outlineParamsMapped_ = nullptr;

    ComPtr<ID3D12Resource> radialBlurParamsResource_;
    RadialBlurParams* radialBlurParamsMapped_ = nullptr;

    ComPtr<ID3D12Resource> dissolveParamsResource_;
    DissolveParams* dissolveParamsMapped_ = nullptr;
    
    ComPtr<ID3D12PipelineState> randomPSO_;
    ComPtr<ID3D12Resource> randomParamsResource_;
    RandomParams* randomParamsMapped_ = nullptr;

    uint32_t depthSrvIndex_ = 0;

    // 遅延適用用 (Update から Draw への橋渡し)
    static int modeNext_;
    static float strengthNext_;
    static float vignetteScaleNext_;
    static float vignetteExponentNext_;
    static int smoothingKernelSizeNext_;
    static int gaussianBlurKernelSizeNext_;
    static float gaussianBlurSigmaNext_;
    static float outlineEdgeMultiplierNext_;
    static float radialBlurCenterXNext_;
    static float radialBlurCenterYNext_;
    static float radialBlurWidthNext_;
    static int radialBlurSampleCountNext_;
    static float dissolveEdgeColorNext_[3];
    static float dissolveThresholdNext_;
    static float dissolveEdgeRangeNext_;
    static std::string dissolveMaskPathNext_;
    static float randomStrengthNext_;
    int currentMode_ = kModeCopy;

    // パラメータ適用最適化用（変更時のみ定数バッファを更新するため）
    float currentStrength_ = -1.0f;
    int currentColorFilterMode_ = -1;
    float currentVignetteScale_ = -1.0f;
    float currentVignetteExponent_ = -1.0f;
    int currentSmoothingKernelSize_ = -1;
    int currentGaussianBlurKernelSize_ = -1;
    float currentGaussianBlurSigma_ = -1.0f;
    Matrix4x4 currentProjectionInverse_{};
    float currentOutlineEdgeMultiplier_ = -1.0f;
    float currentRadialBlurCenterX_ = -1.0f;
    float currentRadialBlurCenterY_ = -1.0f;
    float currentRadialBlurWidth_ = -1.0f;
    int currentRadialBlurSampleCount_ = -1;
    float currentDissolveEdgeColor_[3];
    float currentDissolveThreshold_ = -1.0f;
    float currentDissolveEdgeRange_ = -1.0f;
    std::string currentDissolveMaskPath_;
    float currentRandomStrength_ = -1.0f;
    float currentRandomTime_ = 0.0f;

    static std::unique_ptr<PostProcessManager> instance_;

public:
    static PostProcessManager* GetInstance();
    static void Destroy();

    explicit PostProcessManager(Token) {}

    /// @brief PSO・定数バッファを初期化する
    void Initialize();

    /// @brief 画面全体にポストエフェクトを描画する
    /// @param renderTexture オフスクリーンレンダリング結果
    void Draw(RenderTexture* renderTexture);

    // --- 静的セッター (外部から呼び出す) ---
    static void SetMode(int mode)         { modeNext_ = mode; }
    static void SetStrength(float s)      { strengthNext_ = s; }
    static void SetVignetteParams(float scale, float exponent) {
        vignetteScaleNext_ = scale;
        vignetteExponentNext_ = exponent;
    }
    static void SetSmoothingParams(int kernelSize) {
        smoothingKernelSizeNext_ = kernelSize;
    }
    static void SetGaussianBlurParams(int kernelSize, float sigma) {
        gaussianBlurKernelSizeNext_ = kernelSize;
        gaussianBlurSigmaNext_ = sigma;
    }
    static void SetOutlineParams(float edgeMultiplier) {
        outlineEdgeMultiplierNext_ = edgeMultiplier;
    }
    static void SetRadialBlurParams(float centerX, float centerY, float width, int sampleCount) {
        radialBlurCenterXNext_ = centerX;
        radialBlurCenterYNext_ = centerY;
        radialBlurWidthNext_ = width;
        radialBlurSampleCountNext_ = sampleCount;
    }
    static void SetDissolveParams(const float edgeColor[3], float threshold, float edgeRange) {
        dissolveEdgeColorNext_[0] = edgeColor[0];
        dissolveEdgeColorNext_[1] = edgeColor[1];
        dissolveEdgeColorNext_[2] = edgeColor[2];
        dissolveThresholdNext_ = threshold;
        dissolveEdgeRangeNext_ = edgeRange;
    }
    static void SetDissolveMaskTexture(const std::string& maskPath) {
        dissolveMaskPathNext_ = maskPath;
    }
    static void SetRandomParams(float strength) {
        randomStrengthNext_ = strength;
    }
    int GetCurrentMode() const            { return currentMode_; }

private:
    ~PostProcessManager() = default;
    PostProcessManager(const PostProcessManager&) = delete;
    PostProcessManager& operator=(const PostProcessManager&) = delete;
    friend std::default_delete<PostProcessManager>;
};
