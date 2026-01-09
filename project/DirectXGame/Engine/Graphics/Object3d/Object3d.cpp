#include "Object3d.h"
#include "API/DX12Context.h"
#include "Camera/Camera.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3dCommon.h"
#include "Texture/TextureManager.h"

#include "Math/Functions/MathUtils.h"
#include "Math/Matrix/MatrixGenerators.h"

#include <assert.h>

using namespace Microsoft::WRL;
using namespace MathUtils;
using namespace MathGenerators;

void Object3d::Initialize(Object3dCommon *object3dCommon) {
  // 引数で受け取ってメンバ変数に記録する
  object3dCommon_ = object3dCommon;

  camera_ = object3dCommon_->GetDefaultCamera();

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

  // ワールドビュー射影行列の計算
  Matrix4x4 wvpMatrix;
  if (camera_) {
    const Matrix4x4 &viewProjectionMatrix = camera_->GetViewProjectionMatrix();
    wvpMatrix = worldMatrix * viewProjectionMatrix;
  } else {
    wvpMatrix = worldMatrix;
  }

  // 非均一スケール対応：逆転置行列の計算
    // 法線の変形には「ワールド行列の逆行列の転置行列」が必要
  Matrix4x4 worldInverseTranspose = Transpose(Inverse(worldMatrix));

  // 変換行列データを更新する
  transformationMatrixData_->WVP =
      wvpMatrix; // World-View-Projection行列をWVPメンバーに入れる
  transformationMatrixData_->World =
      worldMatrix; // 純粋なワールド行列をWorldメンバーに入れる
  transformationMatrixData_->WorldInverseTranspose =
      worldInverseTranspose; // ワールド行列の逆行列の転置行列をWorldInverseTransposeメンバーに入れる

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
  // 【追加】RootParameter Index 4: Camera (b2)
  if (camera_) {
      object3dCommon_->GetDX12Context()
          ->GetCommandList()->SetGraphicsRootConstantBufferView(4, camera_->GetConstantBufferGPUVirtualAddress());
  }
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
