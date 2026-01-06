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

void ParticleManager::SetEmitter(const std::string &name,
                                 const ParticleEmitter &emitter) {
    // 1. 該当するパーティクルグループを検索
    auto it = particleGroups_.find(name);

    if (it == particleGroups_.end()) {
        // グループが見つからない場合はエラーまたはログ出力
        Logger::Log("Error: Particle group '" + name + "' not found when setting emitter.");
        return;
    }

    // 2. グループ内の emitter メンバに、渡された emitter インスタンスをコピー
    ParticleGroup &group = it->second;
    group.emitter = emitter;

    // 3. (Optional) 頻度時刻をリセット（すぐに発生を開始させたい場合）
    group.emitter.frequencyTime = 0.0f;
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

void ParticleManager::Emit(const std::string &name, const Vector3 &translate,
                           uint32_t count) {
  // 該当するパーティクルグループを検索
  auto it = particleGroups_.find(name);
  if (it == particleGroups_.end()) {
    // グループが見つからない場合は処理を中止 (またはエラー処理)
    return;
  }

  ParticleGroup &group = it->second;

  // 指定された個数だけパーティクルを生成
  for (uint32_t i = 0; i < count; ++i) {
    // MakeNewParticle でランダムな初期値を持つパーティクルを生成
    Particle newParticle = MakeNewParticle(translate);

    // グループ内のパーティクルリストに追加
    group.particles.push_back(std::move(newParticle));
  }
}

// パーティクル生成関数
Particle ParticleManager::MakeNewParticle(const Vector3 &translate) {

  // 乱数分布の定義 (エンジンは引数の randomEngine を使用)
  std::uniform_real_distribution<float> posDist(-1.0f, 1.0f);
  std::uniform_real_distribution<float> timeDist(1.0f, 3.0f);
  std::uniform_real_distribution<float> colorDist(0.0f,
                                                  1.0f); // 0.0f to 1.0f に変更

  // パーティクルの初期化
  Particle particle;

  // -------------------
  // トランスフォーム
  // -------------------
  particle.transform.scale = {1.0f, 1.0f, 1.0f};
  particle.transform.rotate = {0.0f, 0.0f, 0.0f};

  // 基準位置からのランダムなオフセットを加える
  Vector3 randomTranslate{posDist(randomEngine_), posDist(randomEngine_),
                          posDist(randomEngine_)};
  particle.transform.translate = translate + randomTranslate;

  // -------------------
  // 寿命と時間
  // -------------------
  particle.lifeTime = timeDist(randomEngine_); // 1.0秒から 3.0秒
  particle.currentTime = 0.0f;

  // -------------------
  // 速度
  // -------------------
  // 速度をランダムに設定 (例: -1.0から1.0の範囲)
  particle.velocity = {posDist(randomEngine_), posDist(randomEngine_),
                       posDist(randomEngine_)};

  // -------------------
  // 色
  // -------------------
  // 色をランダムに設定 (0.0から1.0の範囲)
  particle.color = {colorDist(randomEngine_), colorDist(randomEngine_),
                    colorDist(randomEngine_), 1.0f}; // アルファは不透明

  return particle;
}

void ParticleManager::Update(const Camera &camera, bool &isUpdate,
                             bool &useBillboard) {
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

  // 全てのパーティクルグループについて処理
  for (auto &pair : particleGroups_) {
    ParticleGroup &group = pair.second;

    // ------------------------------------------
    // I. エミッターの更新とパーティクルの生成 (資料: 更新)
    // ------------------------------------------

    // 1. 時刻を進める
    group.emitter.frequencyTime += kDeltaTime;

    // 2. 発生頻度より大きいなら発生
    if (group.emitter.frequency > 0.0f &&
        group.emitter.frequency <= group.emitter.frequencyTime) {

      // ParticleEmitter::Emit を呼び出し、ParticleManager::Emit へ処理を委譲
      // Emitter自身の設定値 (translate, count) を使って生成を依頼する
      group.emitter.Emit(pair.first); // pair.first はグループ名

      // 余計に過ぎた時間も加味して頻度計算する (資料の指示)
      group.emitter.frequencyTime -= group.emitter.frequency;
    }

    // ------------------------------------------
    // II. 既存パーティクルの更新とインスタンスデータの書き込み
    // ------------------------------------------

    // インスタンスデータへの書き込み用ポインタ
    ParticleInstanceData *instanceData =
        (ParticleInstanceData *)group.mappedData;
    uint32_t instanceIndex = 0; // 現在書き込んでいるインスタンスのインデックス

    // グループ内の全てのパーティクルについて処理
    auto it = group.particles.begin();
    while (it != group.particles.end()) {
      Particle &particle = *it;

      // 1. フィールドの影響計算 (場の影響)
      if (isUpdate) {
        AccelerationField accelerationField;
        accelerationField.acceleration = {15.0f, 0.0f, 0.0f};
        accelerationField.area.min = {-1.0f, -1.0f, -1.0f};
        accelerationField.area.max = {1.0f, 1.0f, 1.0f};

        if (IsCollision(accelerationField.area, particle.transform.translate)) {
          particle.velocity += accelerationField.acceleration * kDeltaTime;
        }
      }

      // 2. 移動処理
      particle.transform.translate =
          particle.transform.translate + (particle.velocity * kDeltaTime);

      // 3. 経過時間とアルファ値の計算
      particle.currentTime += kDeltaTime;
      float alpha = 1.0f - (particle.currentTime / particle.lifeTime);

      // 4. 寿命チェックと削除
      if (particle.currentTime >= particle.lifeTime) {
        it = group.particles.erase(it);
        continue;
      }

      // 5. インスタンシングデータの書き込み (行列計算とGPU転送)
      if (instanceIndex < kNumMaxParticle) {

        // ... (行列計算のロジックは前回の修正案を適用) ...
        Matrix4x4 finalWorldMatrix;
        // --- 修正案 B (行列乗算) を再掲 ---
        Matrix4x4 scaleMatrix = MakeScaleMatrix(particle.transform.scale);
        Matrix4x4 billboardMatrix{};

        // useBillboard が true の場合のみビルボード行列を適用
        if (useBillboard) {
            billboardMatrix = camera.GetWorldMatrix();
            billboardMatrix.m[3][0] = 0.0f;
            billboardMatrix.m[3][1] = 0.0f;
            billboardMatrix.m[3][2] = 0.0f;
        } else {
          billboardMatrix = MakeIdentity4x4();
        }

        Matrix4x4 scaleRotateMatrix = Multiply(scaleMatrix, billboardMatrix);
        finalWorldMatrix = scaleRotateMatrix;
        finalWorldMatrix.m[3][0] = particle.transform.translate.x;
        finalWorldMatrix.m[3][1] = particle.transform.translate.y;
        finalWorldMatrix.m[3][2] = particle.transform.translate.z;
        // --- 修正案 B ここまで ---

        Matrix4x4 wvpMatrix = Multiply(finalWorldMatrix, viewProjectionMatrix);

        // GPU転送
        instanceData[instanceIndex].WVP = wvpMatrix;
        instanceData[instanceIndex].World = finalWorldMatrix;
        instanceData[instanceIndex].color = particle.color;
        instanceData[instanceIndex].color.w = alpha; // 透明度適用

        instanceIndex++;
      }

      ++it;
    }

    // グループの描画インスタンス数を更新
    group.instanceCount = instanceIndex;
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