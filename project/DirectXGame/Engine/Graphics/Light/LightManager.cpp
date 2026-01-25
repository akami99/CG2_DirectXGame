#include "LightManager.h"
#include "DX12Context.h" // CreateBufferResourceを使うため

#include <numbers>

LightManager *LightManager::instance_ = nullptr;

LightManager* LightManager::GetInstance() {
    if (instance_ == nullptr) {
    instance_ = new LightManager;
  }
    return instance_;
}

void LightManager::Initialize() {
  // 平行光源バッファの作成
  CreateDirectionalResource();
  // 点光源バッファの作成
  CreatePointResource();
  // スポットライトバッファの作成
  CreateSpotResource();
}

void LightManager::Finalize() {
  // インスタンスの解放
  delete instance_;
  instance_ = nullptr;
}

void LightManager::Update() {
    // スポットライトの0除算防止調整
      // cosAngle と cosFalloffStart が近すぎるとシェーダーで 1.0f / 0.0f が起きてしまう
    const float epsilon = 0.0001f;

    if (spotData_->cosFalloffStart <= spotData_->cosAngle + epsilon) {
        // FalloffStartは常にAngleより少しだけ大きく（内側に）設定されるように強制する
        // ※ cosなので、角度が小さいほど値が大きいことに注意
        spotData_->cosFalloffStart = spotData_->cosAngle + epsilon;
    }

    // 必要であれば、cosの範囲(0~1)にクランプする処理もここで行う
    // spotData_->cosAngle = std::clamp(spotData_->cosAngle, 0.0f, 1.0f);
}

void LightManager::Draw() {
    // 平行光源 (RootParameter Index 3)
    DX12Context::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(
        3, directionalResource_->GetGPUVirtualAddress());

    // 点光源 (RootParameter Index 5)
    DX12Context::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(
        5, pointResource_->GetGPUVirtualAddress());

    // スポットライト (RootParameter Index 6)
    DX12Context::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(
        6, spotResource_->GetGPUVirtualAddress());
}

// 平行光源バッファの作成
void LightManager::CreateDirectionalResource() {
  // 定数バッファの生成 (サイズは構造体に合わせる)
  directionalResource_ =
      DX12Context::GetInstance()->CreateBufferResource(sizeof(DirectionalLight));

  // データの書き込み先を取得
  directionalResource_->Map(0, nullptr,
                            reinterpret_cast<void **>(&directionalData_));

  // 初期値設定
  directionalData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalData_->direction = {0.0f, -1.0f, 0.0f};
  directionalData_->intensity = 1.0f;
}

void LightManager::CreatePointResource() {
  // リソース（バッファ）を作成
  // サイズが PointLight 構造体の大きさになる点に注意
  pointResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(PointLight));

  // データを書き込むためのアドレスを取得 (Map)
  pointResource_->Map(0, nullptr, reinterpret_cast<void **>(&pointData_));

  // デフォルト値を設定しておく (真っ暗にならないように白などを入れておく)
  pointData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  pointData_->position = {0.0f, 2.0f, 0.0f}; // 適当な位置
  pointData_->intensity = 1.0f;
  pointData_->radius = 10.0f;
  pointData_->decay = 1.0f;
}

void LightManager::CreateSpotResource() {
  // 定数バッファの生成 (サイズは構造体に合わせる)
  spotResource_ = DX12Context::GetInstance()->CreateBufferResource(sizeof(SpotLight));

  // データの書き込み先を取得
  spotResource_->Map(0, nullptr, reinterpret_cast<void **>(&spotData_));

  // 初期値設定
  spotData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  spotData_->position = {2.0f, 1.25f, 0.0f};
  spotData_->distance = 7.0f;
  spotData_->direction = {-1.0f, -1.0f, 0.0f};
  spotData_->intensity = 4.0f;
  spotData_->decay = 2.0f;
  spotData_->cosAngle = std::cosf(std::numbers::pi_v<float> / 3.0f);
  spotData_->cosFalloffStart = std::cosf(std::numbers::pi_v<float> / 4.0f);
}
