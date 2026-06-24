#include "PostProcessManager.h"

#include "RenderTexture.h"
#include "DX12Context.h"
#include "SrvManager.h"
#include "PipelineManager.h"
#include "TextureManager.h"
#include "Object3d/Object3dCommon.h"
#include "Camera/Camera.h"
#include "Math/Functions/MathUtils.h"
#include <cstring>

// 静的メンバの実体化
std::unique_ptr<PostProcessManager> PostProcessManager::instance_;
int PostProcessManager::modeNext_ = 0;
float PostProcessManager::strengthNext_ = 1.0f;
float PostProcessManager::vignetteScaleNext_ = 16.0f;
float PostProcessManager::vignetteExponentNext_ = 0.8f;
int PostProcessManager::smoothingKernelSizeNext_ = 3;
int PostProcessManager::gaussianBlurKernelSizeNext_ = 3;
float PostProcessManager::gaussianBlurSigmaNext_ = 2.0f;
float PostProcessManager::outlineEdgeMultiplierNext_ = 6.0f;
float PostProcessManager::radialBlurCenterXNext_ = 0.5f;
float PostProcessManager::radialBlurCenterYNext_ = 0.5f;
float PostProcessManager::radialBlurWidthNext_ = 0.01f;
int PostProcessManager::radialBlurSampleCountNext_ = 10;
float PostProcessManager::dissolveEdgeColorNext_[3] = { 1.0f, 0.4f, 0.3f };
float PostProcessManager::dissolveThresholdNext_ = 0.0f;
float PostProcessManager::dissolveEdgeRangeNext_ = 0.02f;
std::string PostProcessManager::dissolveMaskPathNext_ = "masks/noise0.png";
float PostProcessManager::randomStrengthNext_ = 0.0f;

PostProcessManager* PostProcessManager::GetInstance() {
    if (!instance_) {
        instance_ = std::make_unique<PostProcessManager>(Token{});
    }
    return instance_.get();
}

void PostProcessManager::Destroy() {
    instance_.reset();
}

void PostProcessManager::Initialize() {
    // PSO の生成
    postProcessPSO_ = PipelineManager::GetInstance()->CreatePostProcessPSO();
    colorFilterPSO_  = PipelineManager::GetInstance()->CreateColorFilterPSO();
    vignettePSO_     = PipelineManager::GetInstance()->CreateVignettePSO();
    smoothingPSO_    = PipelineManager::GetInstance()->CreateSmoothingPSO();
    gaussianBlurPSO_ = PipelineManager::GetInstance()->CreateGaussianBlurPSO();
    outlinePSO_      = PipelineManager::GetInstance()->CreateOutlinePSO();
    radialBlurPSO_   = PipelineManager::GetInstance()->CreateRadialBlurPSO();
    dissolvePSO_     = PipelineManager::GetInstance()->CreateDissolvePSO();
    randomPSO_       = PipelineManager::GetInstance()->CreateRandomPSO();

    // 定数バッファの生成とマッピング
    paramsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(PostProcessParams));
    paramsResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramsMapped_));

    // デフォルト値 (パススルー)
    paramsMapped_->colorScale[0] = 1.0f;
    paramsMapped_->colorScale[1] = 1.0f;
    paramsMapped_->colorScale[2] = 1.0f;
    paramsMapped_->strength = 1.0f;

    // ビネット用の定数バッファの生成とマッピング
    vignetteParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(VignetteParams));
    vignetteParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&vignetteParamsMapped_));
    vignetteParamsMapped_->scale = 16.0f;
    vignetteParamsMapped_->exponent = 0.8f;

    // 平滑化用の定数バッファの生成とマッピング
    smoothingParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(SmoothingParams));
    smoothingParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&smoothingParamsMapped_));
    smoothingParamsMapped_->kernelSize = 3;

    // ガウシアンフィルター用の定数バッファの生成とマッピング
    gaussianBlurParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(GaussianBlurParams));
    gaussianBlurParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&gaussianBlurParamsMapped_));
    gaussianBlurParamsMapped_->kernelSize = 3;
    gaussianBlurParamsMapped_->sigma = 2.0f;

    // アウトライン用の定数バッファの生成とマッピング
    outlineParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(OutlineParams));
    outlineParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&outlineParamsMapped_));
    outlineParamsMapped_->edgeMultiplier = 6.0f;
    std::memset(&currentProjectionInverse_, 0, sizeof(currentProjectionInverse_));

    // Radial Blur用の定数バッファの生成とマッピング
    radialBlurParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(RadialBlurParams));
    radialBlurParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&radialBlurParamsMapped_));
    radialBlurParamsMapped_->center[0] = 0.5f;
    radialBlurParamsMapped_->center[1] = 0.5f;
    radialBlurParamsMapped_->blurWidth = 0.01f;
    radialBlurParamsMapped_->sampleCount = 10;

    // Dissolve用の定数バッファの生成とマッピング
    dissolveParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(DissolveParams));
    dissolveParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&dissolveParamsMapped_));
    dissolveParamsMapped_->edgeColor[0] = 1.0f;
    dissolveParamsMapped_->edgeColor[1] = 0.4f;
    dissolveParamsMapped_->edgeColor[2] = 0.3f;
    dissolveParamsMapped_->threshold = 0.0f;
    dissolveParamsMapped_->edgeRange = 0.02f;

    // Random用の定数バッファの生成とマッピング
    randomParamsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(RandomParams));
    randomParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&randomParamsMapped_));
    randomParamsMapped_->time = 0.0f;
    randomParamsMapped_->strength = 0.0f;

    // マスクテクスチャを事前にロード
    TextureManager::GetInstance()->LoadTexture("masks/noise0.png");
    TextureManager::GetInstance()->LoadTexture("masks/noise1.png");

    // 深度バッファ用の SRV を作成
    depthSrvIndex_ = SrvManager::GetInstance()->Allocate();
    SrvManager::GetInstance()->CreateSRVForTexture(
        depthSrvIndex_,
        DX12Context::GetInstance()->GetDepthStencilResource(),
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
        1,
        false
    );
}

void PostProcessManager::Draw(RenderTexture* renderTexture) {
    ID3D12GraphicsCommandList* commandList = DX12Context::GetInstance()->GetCommandList();

    /// モードとパラメータを遅延適用
    currentMode_ = modeNext_;
    // パラメータの遅延適用
    const float strength = strengthNext_;
    // ビネット用のパラメータ
    const float vignetteScale = vignetteScaleNext_;
    const float vignetteExponent = vignetteExponentNext_;
    // 平滑化用のパラメータ
    const int smoothingKernelSize = smoothingKernelSizeNext_;
    // Gaussian Blur用のパラメータ
    const int gaussianBlurKernelSize = gaussianBlurKernelSizeNext_;
    const float gaussianBlurSigma = gaussianBlurSigmaNext_;
    const float outlineEdgeMultiplier = outlineEdgeMultiplierNext_;
    // Radial Blur用のパラメータ
    const float radialBlurCenterX = radialBlurCenterXNext_;
    const float radialBlurCenterY = radialBlurCenterYNext_;
    const float radialBlurWidth = radialBlurWidthNext_;
    const int radialBlurSampleCount = radialBlurSampleCountNext_;
    // Dissolve用のパラメータ
    const float dissolveThreshold = dissolveThresholdNext_;
    const float dissolveEdgeRange = dissolveEdgeRangeNext_;
    const float dissolveEdgeColor[3] = { dissolveEdgeColorNext_[0], dissolveEdgeColorNext_[1], dissolveEdgeColorNext_[2] };
    const std::string dissolveMaskPath = dissolveMaskPathNext_;
    // Random用のパラメータ
    const float randomStrength = randomStrengthNext_;

    // 深度バッファのリソースバリア（DEPTH_WRITE -> PIXEL_SHADER_RESOURCE）
    bool transitionDepth = (currentMode_ == kModeOutline);
    if (transitionDepth) {
        ID3D12Resource* depthResource = DX12Context::GetInstance()->GetDepthStencilResource();
        D3D12_RESOURCE_BARRIER barrierDepth{};
        barrierDepth.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDepth.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDepth.Transition.pResource = depthResource;
        barrierDepth.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrierDepth.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrierDepth);
    }

    // 共通ルートシグネチャのセット
    commandList->SetGraphicsRootSignature(
        PipelineManager::GetInstance()->GetPostProcessRootSignature());

    // オフスクリーン描画結果を t0 にセット
    SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(1, renderTexture->GetSrvIndex());
    // 深度描画結果を t1 にセット
    SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, depthSrvIndex_);

    if (currentMode_ == kModeCopy) {
        // パススルー
        commandList->SetPipelineState(postProcessPSO_.Get());
        if (currentColorFilterMode_ != currentMode_) {
            currentColorFilterMode_ = currentMode_;
            paramsMapped_->colorScale[0] = 1.0f;
            paramsMapped_->colorScale[1] = 1.0f;
            paramsMapped_->colorScale[2] = 1.0f;
            paramsMapped_->strength = 1.0f;
        }
        commandList->SetGraphicsRootConstantBufferView(0, paramsResource_->GetGPUVirtualAddress());
    } else if (currentMode_ == kModeVignette) {
        // ビネット
        commandList->SetPipelineState(vignettePSO_.Get());
        if (currentVignetteScale_ != vignetteScale || currentVignetteExponent_ != vignetteExponent) {
            currentVignetteScale_ = vignetteScale;
            currentVignetteExponent_ = vignetteExponent;
            vignetteParamsMapped_->scale = vignetteScale;
            vignetteParamsMapped_->exponent = vignetteExponent;
        }
        commandList->SetGraphicsRootConstantBufferView(0, vignetteParamsResource_->GetGPUVirtualAddress());
    } else if (currentMode_ == kModeSmoothing) {
        // 平滑化
        commandList->SetPipelineState(smoothingPSO_.Get());
        if (currentSmoothingKernelSize_ != smoothingKernelSize) {
            currentSmoothingKernelSize_ = smoothingKernelSize;
            smoothingParamsMapped_->kernelSize = smoothingKernelSize;
        }
        commandList->SetGraphicsRootConstantBufferView(0, smoothingParamsResource_->GetGPUVirtualAddress());
    } else if (currentMode_ == kModeGaussianBlur) {
        // ガウシアンフィルター
        commandList->SetPipelineState(gaussianBlurPSO_.Get());
        if (currentGaussianBlurKernelSize_ != gaussianBlurKernelSize || currentGaussianBlurSigma_ != gaussianBlurSigma) {
            currentGaussianBlurKernelSize_ = gaussianBlurKernelSize;
            currentGaussianBlurSigma_ = gaussianBlurSigma;
            gaussianBlurParamsMapped_->kernelSize = gaussianBlurKernelSize;
            gaussianBlurParamsMapped_->sigma = gaussianBlurSigma;
        }
        commandList->SetGraphicsRootConstantBufferView(0, gaussianBlurParamsResource_->GetGPUVirtualAddress());
    } else if (currentMode_ == kModeOutline) {
        // アウトライン
        commandList->SetPipelineState(outlinePSO_.Get());
        Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
        if (camera) {
            Matrix4x4 proj = camera->GetProjectionMatrix();
            Matrix4x4 projInv = MathUtils::Inverse(proj);
            if (std::memcmp(&currentProjectionInverse_, &projInv, sizeof(Matrix4x4)) != 0 || currentOutlineEdgeMultiplier_ != outlineEdgeMultiplier) {
                currentProjectionInverse_ = projInv;
                currentOutlineEdgeMultiplier_ = outlineEdgeMultiplier;
                outlineParamsMapped_->projectionInverse = projInv;
                outlineParamsMapped_->edgeMultiplier = outlineEdgeMultiplier;
            }
        }
        commandList->SetGraphicsRootConstantBufferView(0, outlineParamsResource_->GetGPUVirtualAddress());
    } else if (currentMode_ == kModeRadialBlur) {
        // Radial Blur
        commandList->SetPipelineState(radialBlurPSO_.Get());
        if (currentRadialBlurCenterX_ != radialBlurCenterX ||
            currentRadialBlurCenterY_ != radialBlurCenterY ||
            currentRadialBlurWidth_ != radialBlurWidth ||
            currentRadialBlurSampleCount_ != radialBlurSampleCount) {
            
            currentRadialBlurCenterX_ = radialBlurCenterX;
            currentRadialBlurCenterY_ = radialBlurCenterY;
            currentRadialBlurWidth_ = radialBlurWidth;
            currentRadialBlurSampleCount_ = radialBlurSampleCount;
            
            radialBlurParamsMapped_->center[0] = radialBlurCenterX;
            radialBlurParamsMapped_->center[1] = radialBlurCenterY;
            radialBlurParamsMapped_->blurWidth = radialBlurWidth;
            radialBlurParamsMapped_->sampleCount = radialBlurSampleCount;
        }
        commandList->SetGraphicsRootConstantBufferView(0, radialBlurParamsResource_->GetGPUVirtualAddress());
    } else if (currentMode_ == kModeDissolve) {
        // Dissolve
        commandList->SetPipelineState(dissolvePSO_.Get());
        
        // t1 (Root Parameter 2) にマスクテクスチャの SRV をバインド
        uint32_t maskSrvIndex = TextureManager::GetInstance()->GetSrvIndex(dissolveMaskPath);
        SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, maskSrvIndex);
        
        if (currentDissolveEdgeColor_[0] != dissolveEdgeColor[0] ||
            currentDissolveEdgeColor_[1] != dissolveEdgeColor[1] ||
            currentDissolveEdgeColor_[2] != dissolveEdgeColor[2] ||
            currentDissolveThreshold_ != dissolveThreshold ||
            currentDissolveEdgeRange_ != dissolveEdgeRange ||
            currentDissolveMaskPath_ != dissolveMaskPath) {
            
            currentDissolveEdgeColor_[0] = dissolveEdgeColor[0];
            currentDissolveEdgeColor_[1] = dissolveEdgeColor[1];
            currentDissolveEdgeColor_[2] = dissolveEdgeColor[2];
            currentDissolveThreshold_ = dissolveThreshold;
            currentDissolveEdgeRange_ = dissolveEdgeRange;
            currentDissolveMaskPath_ = dissolveMaskPath;
            
            dissolveParamsMapped_->edgeColor[0] = dissolveEdgeColor[0];
            dissolveParamsMapped_->edgeColor[1] = dissolveEdgeColor[1];
            dissolveParamsMapped_->edgeColor[2] = dissolveEdgeColor[2];
            dissolveParamsMapped_->threshold = dissolveThreshold;
            dissolveParamsMapped_->edgeRange = dissolveEdgeRange;
        }
        commandList->SetGraphicsRootConstantBufferView(0, dissolveParamsResource_->GetGPUVirtualAddress());
    } else if (currentMode_ == kModeRandom) {
        // Random (砂嵐ノイズ)
        commandList->SetPipelineState(randomPSO_.Get());

        // 毎フレーム時間を進める
        currentRandomTime_ += 1.0f / 60.0f;

        // 時間経過を伴うため毎フレーム更新する
        randomParamsMapped_->time = currentRandomTime_;
        randomParamsMapped_->strength = randomStrength;

        commandList->SetGraphicsRootConstantBufferView(0, randomParamsResource_->GetGPUVirtualAddress());
    } else {
        // グレースケール or セピア
        commandList->SetPipelineState(colorFilterPSO_.Get());

        if (currentColorFilterMode_ != currentMode_ || currentStrength_ != strength) {
            currentColorFilterMode_ = currentMode_;
            currentStrength_ = strength;

            if (currentMode_ == kModeGrayscale) {
                paramsMapped_->colorScale[0] = 1.0f;
                paramsMapped_->colorScale[1] = 1.0f;
                paramsMapped_->colorScale[2] = 1.0f;
            } else { // kModeSepia
                // RGB (112, 74, 43) を 112 で正規化
                paramsMapped_->colorScale[0] = 1.0f;
                paramsMapped_->colorScale[1] = 74.0f / 112.0f;
                paramsMapped_->colorScale[2] = 43.0f / 112.0f;
            }
            paramsMapped_->strength = strength;
        }
        commandList->SetGraphicsRootConstantBufferView(0, paramsResource_->GetGPUVirtualAddress());
    }

    // 全画面三角形の描画
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    // 深度バッファのリソースバリア（PIXEL_SHADER_RESOURCE -> DEPTH_WRITE）
    if (transitionDepth) {
        ID3D12Resource* depthResource = DX12Context::GetInstance()->GetDepthStencilResource();
        D3D12_RESOURCE_BARRIER barrierDepth{};
        barrierDepth.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDepth.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDepth.Transition.pResource = depthResource;
        barrierDepth.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrierDepth.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        commandList->ResourceBarrier(1, &barrierDepth);
    }
}
