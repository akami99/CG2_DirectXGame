#pragma once
#include "LightTypes.h" // DirectionalLight構造体があるヘッダ
#include <d3d12.h>
#include <wrl.h>

class DX12Context; // 前方宣言

class LightManager {
public:
  void Initialize(DX12Context *dxBase);
  void Update();

  // それぞれのライトのGPUアドレスを取得
  D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalLightAddress() const {
    return directionalResource_->GetGPUVirtualAddress();
  }
  D3D12_GPU_VIRTUAL_ADDRESS GetPointLightAddress() const {
    return pointResource_->GetGPUVirtualAddress();
  }
  D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightAddress() const {
    return spotResource_->GetGPUVirtualAddress();
  }

  // データ設定用セッター
  void SetDirectionalLightColor(const Vector4 &color) {
    directionalData_->color = color;
  }
  void SetDirectionalLightDirection(const Vector3 &direction) {
    directionalData_->direction = direction;
  }
  void SetDirectionalLightIntensity(float intensity) {
    directionalData_->intensity = intensity;
  }

  void SetPointLightColor(const Vector4 &color) { pointData_->color = color; }
  void SetPointLightPosition(const Vector3 &position) {
    pointData_->position = position;
  }
  void SetPointLightIntensity(float intensity) {
    pointData_->intensity = intensity;
  }

  void SetSpotLightColor(const Vector4 &color) { spotData_->color = color; }
  void SetSpotLightPosition(const Vector3 &position) {
      spotData_->position = position;
  }
  void SetSpotLightIntensity(float intensity) {
      spotData_->intensity = intensity;
  }
  void SetSpotLightDirection(const Vector3 &direction) {
      spotData_->direction = direction;
  }

  // ImGui用ゲッター
  DirectionalLight &GetDirectionalLightData() { return *directionalData_; }
  PointLight &GetPointLightData() { return *pointData_; }
  SpotLight &GetSpotLightData() { return *spotData_; }

private:
  // 平行光源バッファの作成
  void CreateDirectionalResource();

  // 点光源バッファの作成
  void CreatePointResource();

  // スポットライトバッファの作成
  void CreateSpotResource();

private:
  // DirectX基底部分のポインタ
  DX12Context *dxBase_ = nullptr;

  // 平行光源用
  Microsoft::WRL::ComPtr<ID3D12Resource> directionalResource_;
  DirectionalLight *directionalData_ = nullptr;

  // 点光源用
  Microsoft::WRL::ComPtr<ID3D12Resource> pointResource_;
  PointLight *pointData_ = nullptr;

  // スポットライト用
  Microsoft::WRL::ComPtr<ID3D12Resource> spotResource_;
  SpotLight *spotData_ = nullptr;
};