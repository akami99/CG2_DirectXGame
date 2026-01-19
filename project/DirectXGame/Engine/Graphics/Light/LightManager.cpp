#include "LightManager.h"
#include "DX12Context.h" // CreateBufferResourceを使うため

void LightManager::Initialize(DX12Context *dxBase) {
  // DirectX基底部分のポインタを保存
  dxBase_ = dxBase;

  // 平行光源バッファの作成
  CreateDirectionalResource();
  // 点光源バッファの作成
  CreatePointResource();
}

void LightManager::Update() {
  // 必要なら正規化処理などをここで行う
  // data_->direction = Normalize(data_->direction); など
}

// 平行光源バッファの作成
void LightManager::CreateDirectionalResource() {
  // 定数バッファの生成 (サイズは構造体に合わせる)
  directionalResource_ =
      dxBase_->CreateBufferResource(sizeof(DirectionalLight));

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
  pointResource_ = dxBase_->CreateBufferResource(sizeof(PointLight));

  // データを書き込むためのアドレスを取得 (Map)
  pointResource_->Map(0, nullptr, reinterpret_cast<void **>(&pointData_));

  // デフォルト値を設定しておく (真っ暗にならないように白などを入れておく)
  pointData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  pointData_->position = {0.0f, 2.0f, 0.0f}; // 適当な位置
  pointData_->intensity = 1.0f;
  pointData_->radius = 10.0f;
  pointData_->decay = 1.0f;
}