#pragma once

#include "BlendMode/BlendMode.h"
#include "Types/ModelTypes.h"
#include "Types/ParticleTypes.h"
#include "ParticleEmitter.h"

#include <d3d12.h>
#include <random>
#include <string>
#include <unordered_map>
#include <wrl/client.h>

class Camera;

class ParticleManager {
public:
  // 【Passkey Idiom】
  struct Token {
  private:
    friend class ParticleManager;
    Token() {}
  };

private: // namespace省略のためのusing宣言
#pragma region using宣言

  // Microsoft::WRL::ComPtrをComPtrで省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma endregion

private: // メンバ変数
  struct ParticleGroup {
    MaterialData materialData;               // マテリアルデータ
    ComPtr<ID3D12Resource> materialResource; // CBV用リソース
    Material *materialMappedData = nullptr;  // マッピングされたポインタ
    std::list<Particle> particles;           // パーティクルのリスト
    D3D12_GPU_DESCRIPTOR_HANDLE
    instanceSrvHandleGPU; // インスタンシングデータ用SRVインデックス
    ComPtr<ID3D12Resource>
        instanceResource;   // インスタンシングリソース (StructuredBuffer)
    uint32_t instanceCount; // インスタンス数
    ParticleInstanceData *mappedData =
        nullptr; // インスタンシングデータを書き込むためのポインタ
    ParticleEmitter emitter;
  };
  std::unordered_map<std::string, ParticleGroup> particleGroups_{};

  // シングルトンインスタンス
  static std::unique_ptr<ParticleManager> instance_;

  // パイプライン・頂点リソース関連のメンバ変数
  ComPtr<ID3D12RootSignature> particleRootSignature_{};
  ComPtr<ID3D12PipelineState>
      particlePsoArray_[BlendMode::BlendState::kCountOfBlendMode]{};
  ComPtr<ID3D12Resource> vertexResource_{};
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
  VertexData *vertexData_ = nullptr;

  ComPtr<ID3D12Resource> indexResource_{};    // インデックスバッファリソース
  D3D12_INDEX_BUFFER_VIEW indexBufferView_{}; // インデックスバッファビュー
  uint32_t numIndices_ = 6;                   // 描画インデックス数 (6固定)

  // ランダムエンジン
  std::mt19937 randomEngine_;

  // Particleの最大数
  static const uint32_t kNumMaxParticle = 100;

  // 初期化中に使用したアップロードリソースを保持するリスト
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateResources_;

public: // シングルトンインスタンス取得
    static ParticleManager* GetInstance();
    static void Destroy();

public: // メンバ関数
  // コンストラクタ(隠蔽)
  explicit ParticleManager(Token);
  // 初期化処理
  void Initialize();

  // パーティクルグループの生成
  void CreateParticleGroup(const std::string &name,
                           const std::string &textureFilePath);

  // パーティクルの発生 (Emit)
  void Emit(const std::string &name, const Vector3 &translate, uint32_t count);

  // パーティクル生成関数
  Particle MakeNewParticle(const Vector3 &translate);

  // 更新処理
  void Update(const Camera &camera, bool &isUpdate, bool &useBillboard, float deltaTime);

  // 描画処理
  void Draw(BlendMode::BlendState blendMode);

  void SetEmitter(const std::string &name, const ParticleEmitter &emitter);

  // 解放処理
  void ReleaseIntermediateResources();

  // 終了処理
  void Finalize();

private: // メンバ関数
  // デストラクタ(隠蔽)
  ~ParticleManager() = default;
  // コピーコンストラクタの封印
  ParticleManager(const ParticleManager &) = delete;
  // コピー代入演算子の封印
  ParticleManager &operator=(const ParticleManager &) = delete;

  friend std::default_delete<ParticleManager>;
};
