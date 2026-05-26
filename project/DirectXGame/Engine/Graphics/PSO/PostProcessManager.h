#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

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

    ComPtr<ID3D12PipelineState> postProcessPSO_; // CopyImage (passthrough)
    ComPtr<ID3D12PipelineState> colorFilterPSO_; // グレースケール/セピア
    ComPtr<ID3D12PipelineState> vignettePSO_;    // ビネット
    ComPtr<ID3D12PipelineState> smoothingPSO_;   // 平滑化
    ComPtr<ID3D12Resource> paramsResource_;
    PostProcessParams* paramsMapped_ = nullptr;

    ComPtr<ID3D12Resource> vignetteParamsResource_;
    VignetteParams* vignetteParamsMapped_ = nullptr;

    ComPtr<ID3D12Resource> smoothingParamsResource_;
    SmoothingParams* smoothingParamsMapped_ = nullptr;

    // 遅延適用用 (Update から Draw への橋渡し)
    static int modeNext_;
    static float strengthNext_;
    static float vignetteScaleNext_;
    static float vignetteExponentNext_;
    static int smoothingKernelSizeNext_;
    int currentMode_ = kModeCopy;

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
    int GetCurrentMode() const            { return currentMode_; }

private:
    ~PostProcessManager() = default;
    PostProcessManager(const PostProcessManager&) = delete;
    PostProcessManager& operator=(const PostProcessManager&) = delete;
    friend std::default_delete<PostProcessManager>;
};
