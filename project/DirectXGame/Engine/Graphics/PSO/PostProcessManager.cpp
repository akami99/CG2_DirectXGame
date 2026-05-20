#include "PostProcessManager.h"

#include "RenderTexture.h"
#include "DX12Context.h"
#include "SrvManager.h"
#include "PipelineManager.h"

// 静的メンバの実体化
std::unique_ptr<PostProcessManager> PostProcessManager::instance_;
int PostProcessManager::modeNext_ = 0;
float PostProcessManager::strengthNext_ = 1.0f;

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

    // 定数バッファの生成とマッピング
    paramsResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(PostProcessParams));
    paramsResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramsMapped_));

    // デフォルト値 (パススルー)
    paramsMapped_->colorScale[0] = 1.0f;
    paramsMapped_->colorScale[1] = 1.0f;
    paramsMapped_->colorScale[2] = 1.0f;
    paramsMapped_->strength = 1.0f;
}

void PostProcessManager::Draw(RenderTexture* renderTexture) {
    ID3D12GraphicsCommandList* commandList = DX12Context::GetInstance()->GetCommandList();

    // モードと強度を遅延適用
    currentMode_ = modeNext_;
    const float strength = strengthNext_;

    // 共通ルートシグネチャのセット
    commandList->SetGraphicsRootSignature(
        PipelineManager::GetInstance()->GetPostProcessRootSignature());

    // オフスクリーン描画結果を t0 にセット
    SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(1, renderTexture->GetSrvIndex());

    if (currentMode_ == kModeCopy) {
        // パススルー
        commandList->SetPipelineState(postProcessPSO_.Get());
        commandList->SetGraphicsRootConstantBufferView(0, paramsResource_->GetGPUVirtualAddress());
    } else {
        // グレースケール or セピア
        commandList->SetPipelineState(colorFilterPSO_.Get());

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
        commandList->SetGraphicsRootConstantBufferView(0, paramsResource_->GetGPUVirtualAddress());
    }

    // 全画面三角形の描画
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);
}
