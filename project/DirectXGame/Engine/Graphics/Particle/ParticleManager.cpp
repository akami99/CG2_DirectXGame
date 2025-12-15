#include "ParticleManager.h"
#include "../Graphics/API/DX12Context.h"
#include "Application/Core/ApplicationConfig.h"
#include "BlendMode/BlendMode.h"
#include "Camera/Camera.h"
#include "Logger/Logger.h"
#include "MathUtils.h"
#include "MatrixGenerators.h"
#include "PSO/PipelineManager.h"
#include "SRV/SrvManager.h"
#include "Texture/TextureManager.h"

#include <algorithm>
#include <assert.h>
#include <numbers>

#include "externals/DirectXTex/d3dx12.h"

using namespace MathUtils;
using namespace MathGenerators;
using namespace Logger;
using namespace BlendMode;

ParticleManager *ParticleManager::instance_ = nullptr;

// シングルトン実装
ParticleManager *ParticleManager::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new ParticleManager;
  }
  return instance_;
}

void ParticleManager::ReleaseIntermediateResources() {
  // GPUがコマンド実行を完了した後、保持していたアップロードリソースをクリアする
  intermediateResources_.clear();
}

// 終了
void ParticleManager::Finalize() {
  // リソースの解放
  // (ComPtrが解放されるため、明示的な解放は不要だが、mappedDataのUnmapは必要)
  for (auto &pair : particleGroups_) {
    ParticleGroup &group = pair.second;
    // MapしたリソースをUnmapする
    if (group.mappedData) {
      group.instanceResource->Unmap(0, nullptr);
      group.mappedData = nullptr;
    }
    /*if (group.materialMappedData) {
      group.materialResource->Unmap(0, nullptr);
      group.materialMappedData = nullptr;
    }*/
  }
  particleGroups_.clear(); // グループのクリア

  // 頂点リソースの解放
  vertexResource_.Reset();

  // シングルトンインスタンスの解放
  delete GetInstance(); // シングルトンインスタンス自身を解放
}

void ParticleManager::Initialize(DX12Context *dxBase, SrvManager *srvManager,
                                 PipelineManager *pipelineManager) {
  // 引数で受け取ったポインタをメンバ変数に記録する
  dxBase_ = dxBase;
  srvManager_ = srvManager;
  pipelineManager_ = pipelineManager;

  // BlendModeごとの設定は汎用関数から取得
  std::array<D3D12_BLEND_DESC, kCountOfBlendMode> blendDescs =
      CreateBlendStateDescs();

  // パイプラインマネージャを使って、ブレンドモードごとのPSOを生成
  for (size_t i = 0; i < kCountOfBlendMode; ++i) {
    particlePsoArray_[i] = pipelineManager_->CreateParticlePSO(blendDescs[i]);
  }

  // ランダムエンジンの初期化
  std::random_device seedGenerator;
  randomEngine_ = std::mt19937(seedGenerator());

  // 頂点データの初期化 (四角形ポリゴン)
  // 単位四角形の頂点データ

  VertexData vertices[4] = {
      {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // 左下
      {{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 左上
      {{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},  // 右下
      {{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}    // 右上
  };

  // インデックスデータで2つの三角形を構成
  const uint16_t indices[6] = {0, 1, 2, 1, 3, 2}; // 6つのインデックス
  const uint32_t kIndexSize = sizeof(indices);
  const UINT kVertexCount = _countof(vertices); // 4
  const UINT sizeVB = sizeof(VertexData) * kVertexCount;

  // 頂点リソースの生成

  // 1. Default Heap にリソースを生成 (テクスチャと同じ構造)
  D3D12_RESOURCE_DESC vertexResourceDesc{};
  vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  vertexResourceDesc.Width = sizeVB;
  vertexResourceDesc.Height = 1;
  vertexResourceDesc.DepthOrArraySize = 1;
  vertexResourceDesc.MipLevels = 1;
  // ... (その他、バッファに必要な設定)
  vertexResourceDesc.SampleDesc.Count = 1;
  vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  const D3D12_HEAP_PROPERTIES defaultHeapProps =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  HRESULT hr = dxBase_->GetDevice()->CreateCommittedResource(
      &defaultHeapProps, // Default Heap!
      D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&vertexResource_));
  assert(SUCCEEDED(hr));

  // 2. 中間アップロードリソースを生成
  ComPtr<ID3D12Resource> vertexUploadResource = dxBase_->CreateBufferResource(
      sizeVB); // UploadHeapリソース生成はここで利用

  // 3. 中間リソースに頂点データを Map & Copy
  VertexData *mappedVertexData = nullptr;
  vertexUploadResource->Map(0, nullptr,
                            reinterpret_cast<void **>(&mappedVertexData));
  memcpy(mappedVertexData, vertices, sizeVB);
  vertexUploadResource->Unmap(0, nullptr); // アンマップ

  // 4. コマンドリストにコピーを積む
  dxBase_->GetCommandList()->CopyResource(vertexResource_.Get(),
                                          vertexUploadResource.Get());

  // 5. 状態遷移コマンドを積む
  D3D12_RESOURCE_BARRIER vertexBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      vertexResource_.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

  dxBase_->GetCommandList()->ResourceBarrier(1, &vertexBarrier);

  // 最後に、このリソースをリストに移動して保持する
  intermediateResources_.push_back(vertexUploadResource);

  // 頂点バッファビュー (VBV) 作成
  vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
  vertexBufferView_.StrideInBytes = sizeof(VertexData);
  vertexBufferView_.SizeInBytes = sizeVB;

  // リソースの生成

  // 1. Default Heap にリソースを生成 (テクスチャと同じ構造)
  D3D12_RESOURCE_DESC indexResourceDesc{};
  indexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  indexResourceDesc.Width = kIndexSize;
  indexResourceDesc.Height = 1;
  indexResourceDesc.DepthOrArraySize = 1;
  indexResourceDesc.MipLevels = 1;
  // ... (その他、バッファに必要な設定)
  indexResourceDesc.SampleDesc.Count = 1;
  indexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  const D3D12_HEAP_PROPERTIES defaultIndexHeapProps =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  hr = dxBase_->GetDevice()->CreateCommittedResource(
      &defaultIndexHeapProps, // Default Heap!
      D3D12_HEAP_FLAG_NONE, &indexResourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&indexResource_));
  assert(SUCCEEDED(hr));

  // 2. 中間アップロードリソースを生成
  ComPtr<ID3D12Resource> indexUploadResource = dxBase_->CreateBufferResource(
      kIndexSize); // UploadHeapリソース生成はここで利用

  // 3. 中間リソースに頂点データを Map & Copy
  uint16_t *mappedIndexData = nullptr;
  HRESULT hrMap = indexUploadResource->Map(
      0, nullptr, reinterpret_cast<void **>(&mappedIndexData));
  assert(SUCCEEDED(hrMap));

  memcpy(mappedIndexData, indices, kIndexSize);
  indexUploadResource->Unmap(0, nullptr); // アンマップ

  // 4. コマンドリストにコピーを積む
  dxBase_->GetCommandList()->CopyResource(indexResource_.Get(),
                                          indexUploadResource.Get());

  // 5. 状態遷移コマンドを積む (COPY_DEST → VERTEX_AND_CONSTANT_BUFFER)
  D3D12_RESOURCE_BARRIER indexBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      indexResource_.Get(), //
      D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

  dxBase_->GetCommandList()->ResourceBarrier(1, &indexBarrier);

  // 最後に、このリソースをリストに移動して保持する
  intermediateResources_.push_back(indexUploadResource);

  // --- IBV (Index Buffer View) の設定 ---
  indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
  indexBufferView_.SizeInBytes = kIndexSize;
  indexBufferView_.Format =
      DXGI_FORMAT_R16_UINT;        // uint16_t を使用するため R16_UINT
  numIndices_ = _countof(indices); // 6を記録

  vertexBufferView_.SizeInBytes = sizeVB;
}

void ParticleManager::CreateParticleGroup(const std::string &name,
                                          const std::string &textureFilePath) {
  // 登録済みの名前かチェックしてassert
  assert(particleGroups_.find(name) == particleGroups_.end());

  // 新たな空のパーティクルグループを作成し、コンテナに登録
  ParticleGroup newGroup;
  newGroup.materialData.textureFilePath = textureFilePath;

  // マテリアルデータにテクスチャのSRVインデックスを記録
  newGroup.materialData.textureFilePath = textureFilePath;
  TextureManager::GetInstance()->LoadTexture(
      newGroup.materialData.textureFilePath);

  // 新しいパーティクルグループのインスタンシング用リソースの生成とSRVの確保/生成

  // インスタンシング用にSRVを確保してSRVインデックスとGPUハンドルを記録
  uint32_t instanceDataIndex = srvManager_->GetNewIndex();

  // インスタンシング用リソース（StructuredBuffer）を生成
  newGroup.instanceResource = dxBase_->CreateBufferResource(
      sizeof(ParticleInstanceData) * kNumMaxParticle);

  // SRV生成 (StructuredBuffer用設定)
  // StructuredBuffer用のSRVを作成します
  srvManager_->CreateSRVForStructuredBuffer(
      instanceDataIndex, newGroup.instanceResource, kNumMaxParticle,
      sizeof(ParticleInstanceData));

  // 描画時に使用するStructuredBufferのGPUハンドルをメンバに記録
  newGroup.instanceSrvHandleGPU =
      srvManager_->GetGPUDescriptorHandle(instanceDataIndex);

  // 書き込み用ポインタを取得し、単位行列で初期化
  newGroup.instanceResource->Map(
      0, nullptr, reinterpret_cast<void **>(&newGroup.mappedData));

  // 初期値を書き込んでおく
  ParticleInstanceData *particleData =
      (ParticleInstanceData *)newGroup.mappedData;
  for (uint32_t index = 0; index < kNumMaxParticle; ++index) {
    particleData[index].WVP = MakeIdentity4x4();
    particleData[index].World = MakeIdentity4x4();
    particleData[index].color = {1.0f, 1.0f, 1.0f, 1.0f};
  }

  // 1. マテリアルデータ用のCBVリソースを生成
  const UINT kCbvAlignedSize = 256;
  newGroup.materialResource = dxBase_->CreateBufferResource(
      (sizeof(MaterialData) + kCbvAlignedSize - 1) & ~(kCbvAlignedSize - 1));

  // 2. リソースをMapし、ポインタを取得
  HRESULT hr = newGroup.materialResource->Map(
      0, nullptr, reinterpret_cast<void **>(&newGroup.materialMappedData));
  assert(SUCCEEDED(hr));

  // 3. マテリアルにデータを書き込む
  newGroup.materialMappedData->color = {1.0f, 1.0f, 1.0f, 1.0f};
  newGroup.materialMappedData->enableLighting =
      0; // パーティクルではライティングを無効化 (0)
  newGroup.materialMappedData->uvTransform = MakeIdentity4x4();

  // グループをマップに追加
  particleGroups_.emplace(name, std::move(newGroup));
}

void ParticleManager::Emit(const std::string &name, const Emitter &emitter) {
  // 登録済みのパーティクルグループ名かチェックしてassert
  assert(particleGroups_.count(name) > 0);

  ParticleGroup &group = particleGroups_.at(name);

  // 速度と寿命の分布を定義 (メンバ変数でないならここで定義)
  std::uniform_real_distribution<float> distVelocity(-1.0f, 1.0f);
  std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
  std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
  //
  // 新たなパーティクルを生成し、指定されたパーティクルグループに登録
  for (uint32_t count = 0; count < emitter.count; ++count) {
    if (group.particles.size() >= kNumMaxParticle) {
      // 最大数を超えたら生成しない
      break;
    }

    Particle particle;

    // -------------------
    // トランスフォーム
    // -------------------
    particle.transform.scale = {1.0f, 1.0f, 1.0f};
    particle.transform.rotate = {0.0f, 0.0f, 0.0f};

    // 位置設定 (emitter.transform.translate
    // を基点にランダムなオフセットを加える)
    Vector3 randomTranslate{distVelocity(randomEngine_),
                            distVelocity(randomEngine_),
                            distVelocity(randomEngine_)};
    particle.transform.translate =
        emitter.transform.translate + randomTranslate;

    // -------------------
    // 寿命と時間
    // -------------------
    particle.lifeTime = distTime(randomEngine_);
    particle.currentTime = 0.0f;

    // -------------------
    // 速度
    // -------------------
    particle.velocity = {distVelocity(randomEngine_),
                         distVelocity(randomEngine_),
                         distVelocity(randomEngine_)};

    // -------------------
    // 色
    // -------------------
    // 色をランダムに設定 (0.0から1.0の範囲)
    particle.color = {distColor(randomEngine_), distColor(randomEngine_),
                      distColor(randomEngine_), 1.0f}; // アルファは不透明

    // 4. グループに追加
    group.particles.push_back(particle);
  }
}

void ParticleManager::Update(const Camera &camera) {
  // 1. ビルボード行列の計算
  // カメラの回転情報からビルボード行列を計算します
  Matrix4x4 billboardMatrix = camera.GetWorldMatrix();
  billboardMatrix.m[3][0] = 0.0f; // 平行移動成分を削除
  billboardMatrix.m[3][1] = 0.0f;
  billboardMatrix.m[3][2] = 0.0f;
  // ※カリングがoffになっていないのなら表裏を反転 (必要に応じてコメント解除)
  // billboardMatrix = Multiply(billboardMatrix,
  // MakeRotateYMatrix(std::numbers::pi_v<float>));

  // 2. ビュー行列とプロジェクション行列をカメラから取得
  const Matrix4x4 viewProjectionMatrix =
      Multiply(camera.GetViewMatrix(), camera.GetProjectionMatrix());

  // ユーザー設定のフラグ (ParticleManagerのメンバか外部定数)
  bool isUpdate = true;
  bool useBillboard = true; // 仮にビルボードを常に使用

  // 全てのパーティクルグループについて処理
  for (auto &pair : particleGroups_) {
      ParticleGroup &group = pair.second;

      //// 【統合 1-1】エミッターの更新と生成
      //group.emitter_.frequencyTime += kDeltaTime; // 時刻を進める
      //if (group.emitter_.frequency <= group.emitter_.frequencyTime) { // 頻度より大きいなら発生

      //    // パーティクルを発生させる
      //    // Emit関数は外部で定義されているとして、リストにパーティクルを追加
      //    // group.particles.splice(group.particles.end(), Emit(group.emitter_, randomEngine_)); 

      //    // Emitがない場合は、直接ここで生成
      //    for (int i = 0; i < group.emitter_.count; ++i) {
      //        Particle newParticle = MakeNewParticle(randomEngine_, group.emitter_.transform.translate);
      //        group.particles.push_back(std::move(newParticle));
      //    }

      //    group.emitter_.frequencyTime -= group.emitter_.frequency; // 余計に過ぎた時間も加味して頻度計算する
      //}


      // インスタンスデータへの書き込み用ポインタ (StructuredBufferのmappedData)
      ParticleInstanceData *instanceData =
          (ParticleInstanceData *)group.mappedData;
      uint32_t instanceIndex = 0; // 現在書き込んでいるインスタンスのインデックス

      // グループ内の全てのパーティクルについて処理
      auto it = group.particles.begin();
      while (it != group.particles.end()) {
          Particle &particle = *it;

          // ------------------------------------------
          // パーティクル実体の更新
          // ------------------------------------------

          // 【統合 2-1】フィールドの影響計算 
          if (isUpdate) {
              AccelerationField accelerationField;
              accelerationField.acceleration = { 15.0f, 0.0f, 0.0f };
              accelerationField.area.min = { -1.0f, -1.0f, -1.0f };
              accelerationField.area.max = { 1.0f, 1.0f, 1.0f };

              if (IsCollision(accelerationField.area, particle.transform.translate)) {
                  particle.velocity += accelerationField.acceleration * kDeltaTime;
              }
          }

          // 移動処理 (速度を座標に加算)
          particle.transform.translate =
              particle.transform.translate + (particle.velocity * kDeltaTime);

          // 経過時間を加算
          particle.currentTime += kDeltaTime;

          // 【統合 2-2】アルファ値を計算 (透明化の処理)
          float alpha = 1.0f - (particle.currentTime / particle.lifeTime);


          // 寿命チェック: 寿命に達していたらグループから外す
          if (particle.currentTime >= particle.lifeTime) {
              // 寿命が尽きた場合、リストから削除する
              it = group.particles.erase(it);
              continue; // 削除されたので、GPU転送はスキップし、次の要素へ
          }

          // ------------------------------------------
          // インスタンシングデータの書き込み (GPU転送処理)
          // ------------------------------------------

          // 【統合 3-1】GPU最大数チェック (オーバーフロー防止)
          if (instanceIndex < kNumMaxParticle) {

              // ワールド行列を計算
              Matrix4x4 worldMatrix =
                  MakeAffineMatrix(particle.transform.scale, particle.transform.rotate,
                      particle.transform.translate);

              Matrix4x4 finalWorldMatrix;

              // 【統合 3-2】ビルボード処理の適用/不適用
              if (useBillboard) {

                  Matrix4x4 scaleMatrix = MakeScaleMatrix(particle.transform.scale);
                  Matrix4x4 scaleRotateMatrix = Multiply(scaleMatrix, billboardMatrix);
                  finalWorldMatrix = scaleRotateMatrix;
                  finalWorldMatrix.m[3][0] = particle.transform.translate.x;
                  finalWorldMatrix.m[3][1] = particle.transform.translate.y;
                  finalWorldMatrix.m[3][2] = particle.transform.translate.z;

              } else {
                  // ビルボードを使用しない場合は、通常のアフィン変換
                  finalWorldMatrix = worldMatrix;
              }

              // ワールドビュープロジェクション行列を合成
              Matrix4x4 wvpMatrix = Multiply(finalWorldMatrix, viewProjectionMatrix);

              // インスタンシング用データ1個分の書き込み
              instanceData[instanceIndex].WVP = wvpMatrix;
              instanceData[instanceIndex].World = finalWorldMatrix;

              // 転送
              instanceData[instanceIndex].color = particle.color;
              instanceData[instanceIndex].color.w = alpha; // 徐々に透明にする

              instanceIndex++; // 有効なパーティクル数をカウント
          }

          // 4. 次の要素へ進める（削除されなかった場合のみ）
          ++it;
      }

      // グループの描画インスタンス数を更新
      group.instanceCount = instanceIndex; // 描画数を更新
  }
}

void ParticleManager::Draw(BlendMode::BlendState blendMode) {
  // 1. コマンド: ルートシグネチャを設定
  dxBase_->GetCommandList()->SetGraphicsRootSignature(
      pipelineManager_->GetParticleRootSignature());

  // 2. コマンド: PSO(Pipeline State Object)を設定
  dxBase_->GetCommandList()->SetPipelineState(
      particlePsoArray_[blendMode].Get());
  // PSOのインデックスは、ParticleManager::Initializeで生成したブレンドモードの配列順に合わせる

  // 3. コマンド: プリミティブトポロジー (描画形状)を設定
  dxBase_->GetCommandList()->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // 4. コマンド: VBV(Vertex Buffer View)を設定
  dxBase_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
  // インデックスバッファビューを設定
  dxBase_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

  // 5. 全てのパーティクルグループについて処理
  // 1グループ分で1DrawCallなので、こちらは二重for文にはならない。
  for (auto &pair : particleGroups_) {
    ParticleGroup &group = pair.second;

    // 描画インスタンス数が0の場合はスキップ
    if (group.instanceCount == 0) {
      continue;
    }

    // コマンド：マテリアルデータのCBVを設定 (RootParameter[0])
    if (group.materialResource) {
      dxBase_->GetCommandList()->SetGraphicsRootConstantBufferView(
          0, group.materialResource->GetGPUVirtualAddress());

    } else {
      // 警告またはアサート: マテリアルリソースがNULLです
      assert(false && "Material Resource is NULL in Draw");
    }

    // コマンド：インスタンシングデータのSRVのDescriptorTableを設定(RootParameter[1])
    dxBase_->GetCommandList()->SetGraphicsRootDescriptorTable(
        1, group.instanceSrvHandleGPU);

    // コマンド：テクスチャのSRVのDescriptorTableを設定(RootParameter[2])
    dxBase_->GetCommandList()->SetGraphicsRootDescriptorTable(
        2, TextureManager::GetInstance()->GetSrvHandleGPU(
               group.materialData.textureFilePath));

    // コマンド：DrawCall (インスタンシング描画)
    // インスタンス数: group.instanceCount
    if (group.instanceCount > 0) {
      dxBase_->GetCommandList()->DrawIndexedInstanced(
          numIndices_, // ParticleManager::numIndices_
          group.instanceCount, 0, 0, 0);
    }

    // dxBase_->GetCommandList()->DrawInstanced(6, group.instanceCount, 0, 0);
  }
}