#include "Object3d.h"
#include "API/DX12Context.h"
#include "Model/Model.h"
#include "Object3dCommon.h"
#include "Texture/TextureManager.h"
#include "Model/ModelManager.h"

#include "Math/Functions/MathUtils.h"
#include "Math/Matrix/MatrixGenerators.h"

#include <assert.h>

using namespace Microsoft::WRL;
using namespace MathUtils;
using namespace MathGenerators;

void Object3d::Initialize(Object3dCommon *object3dCommon) {
  // 引数で受け取ってメンバ変数に記録する
  object3dCommon_ = object3dCommon;

  // 変換行列バッファの作成
  CreateTransformationMatrixResource();

  // 平行光源バッファの作成
  CreateDirectionalLightResource();
}

// 更新処理
void Object3d::Update() {

#pragma region 変換行列データの設定
  // Transform情報を作る

  // Transform情報から変換行列を作る
  Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate);
  // cameraTransformからcameraMatrixを作る
  Matrix4x4 cameraMatrix =
      MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate,
                       cameraTransform_.translate);
  // cameraMatrixからViewMatrixを作る
  Matrix4x4 viewMatrix = Inverse(cameraMatrix);
  // ProjectionMatrixを作って透視投影行列を書き込む
  Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
      0.45f,
      float(Win32Window::kClientWidth) / float(Win32Window::kClientHeight),
      0.1f, 100.0f);
  Matrix4x4 wvpMatrix = worldMatrix * viewMatrix * projectionMatrix;
  transformationMatrixData_->WVP =
      wvpMatrix; // World-View-Projection行列をWVPメンバーに入れる
  transformationMatrixData_->World =
      worldMatrix; // 純粋なワールド行列をWorldメンバーに入れる

#pragma endregion ここまで
}

// 描画処理
void Object3d::Draw() {
  // 変換行列CBVの設定
  object3dCommon_->GetDX12Context()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          1, transformationMatrixResource_->GetGPUVirtualAddress());
  // DirectionalLightのCBufferの場所を設定 (PS b1, rootParameter[3]に対応)
  object3dCommon_->GetDX12Context()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          3,
          directionalLightResource_ // directionalLightResourceはライトのCBV
              ->GetGPUVirtualAddress());
  // 描画コマンド
  if (model_) {
    model_->Draw();
  }
}

void Object3d::SetModel(const std::string &filepath) {
    // モデルを検索してセットする
    model_ = ModelManager::GetInstance()->FindModel(filepath);
}

// 変換行列バッファの作成
void Object3d::CreateTransformationMatrixResource() {
#pragma region 変換行列リソースの作成
  // 変換行列リソースの作成

  // 変換行列リソースを作る
  transformationMatrixResource_ =
      object3dCommon_->GetDX12Context()->CreateBufferResource(
          sizeof(TransformationMatrix));

#pragma endregion ここまで

#pragma region TransformationMatrixDataの設定
  // TransformationMatrixDataの設定

  // TransformationMatrixResourceにデータを書き込むためのアドレスを取得してTransformationMatrixDataに割り当てる
  // 書き込むためのアドレスを取得
  transformationMatrixResource_->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));
  // 単位行列を書き込んでおく
  transformationMatrixData_->WVP = MakeIdentity4x4();
  transformationMatrixData_->World = MakeIdentity4x4();

#pragma endregion ここまで
}

// 平行光源バッファの作成
void Object3d::CreateDirectionalLightResource() {
#pragma region 平行光源リソースの作成
  // 平行光源リソースの作成

  // 平行光源リソースを作る
  directionalLightResource_ =
      object3dCommon_->GetDX12Context()->CreateBufferResource(
          sizeof(DirectionalLight));

#pragma endregion ここまで

#pragma region DirectionalLightDataの設定
  // DirectionalLightDataの設定

  // DirectionalLightlResourceにデータを書き込むためのアドレスを取得してDirectionalLightDataに割り当てる
  directionalLightResource_->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData_));
  // デフォルト値を書き込んでおく
  directionalLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLightData_->direction = {0.0f, -1.0f, 0.0f};
  directionalLightData_->intensity = 1.0f;

#pragma endregion ここまで
}
