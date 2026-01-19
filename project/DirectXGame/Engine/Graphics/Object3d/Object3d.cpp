#include "Object3d.h"
#include "Base/DX12Context.h"
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

  // 点光源バッファの作成
  CreatePointLightResource();
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
  // RootParameter Index 4: Camera (b2)
  if (camera_) {
    object3dCommon_->GetDX12Context()
        ->GetCommandList()
        ->SetGraphicsRootConstantBufferView(
            4, camera_->GetConstantBufferGPUVirtualAddress());
  }
  // PointLightのCBufferの場所を設定 (PS b3, rootParameter[5]に対応)
  object3dCommon_->GetDX12Context()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          5,
          pointLightResource_ // pointLightResourceはライトのCBV
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
  // 変換行列リソースの作成
  transformationMatrixResource_ =
      object3dCommon_->GetDX12Context()->CreateBufferResource(
          sizeof(TransformationMatrix));

  // TransformationMatrixDataの設定
  // TransformationMatrixResourceにデータを書き込むためのアドレスを取得してTransformationMatrixDataに割り当てる
  // 書き込むためのアドレスを取得
  transformationMatrixResource_->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));
  // 単位行列を書き込んでおく
  transformationMatrixData_->WVP = MakeIdentity4x4();
  transformationMatrixData_->World = MakeIdentity4x4();
}

// 平行光源バッファの作成
void Object3d::CreateDirectionalLightResource() {
  // 平行光源リソースの作成
  directionalLightResource_ =
      object3dCommon_->GetDX12Context()->CreateBufferResource(
          sizeof(DirectionalLight));

  // DirectionalLightDataの設定
  // DirectionalLightResourceにデータを書き込むためのアドレスを取得してDirectionalLightDataに割り当てる
  directionalLightResource_->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData_));
  // デフォルト値を書き込んでおく
  directionalLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLightData_->direction = {0.0f, -1.0f, 0.0f};
  directionalLightData_->intensity = 1.0f;
}

void Object3d::CreatePointLightResource() {
  // リソース（バッファ）を作成
  // サイズが PointLight 構造体の大きさになる点に注意
  pointLightResource_ = object3dCommon_->GetDX12Context()->CreateBufferResource(
      sizeof(PointLight));

  // データを書き込むためのアドレスを取得 (Map)
  pointLightResource_->Map(0, nullptr,
                           reinterpret_cast<void **>(&pointLightData_));

  // デフォルト値を設定しておく (真っ暗にならないように白などを入れておく)
  pointLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  pointLightData_->position = {0.0f, 2.0f, 0.0f}; // 適当な位置
  pointLightData_->intensity = 1.0f;
  /*pointLightData_->radius = 10.0f;
  pointLightData_->decay = 1.0f;*/
}
