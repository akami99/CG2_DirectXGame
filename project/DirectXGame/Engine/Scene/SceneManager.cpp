#include "SceneManager.h"
#include "DX12Context.h"
#include "TextureManager.h"
#include "ParticleManager.h"
#include "Logger.h" // ログ用

SceneManager *SceneManager::instance_ = nullptr;

SceneManager *SceneManager::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new SceneManager;
  }
  return instance_;
}

void SceneManager::Finalize() {
    if (currentScene_) {
        currentScene_->Finalize();
        
        currentScene_ = nullptr;
    }
    
    if (nextScene_) {
        nextScene_->Finalize();
        nextScene_ = nullptr;
    }
}

void SceneManager::Destroy() {
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

void SceneManager::Update() {
    // シーン切り替え予約があるか確認
    if (nextScene_) {
        // 1. 旧シーンの終了
        if (currentScene_) {
            currentScene_->Finalize();
            currentScene_.reset(); // unique_ptrならこれで消える
        }

        // 2. 新シーンへの入れ替え（所有権移動）
        currentScene_ = std::move(nextScene_);

        // 3. 新シーンの初期化処理
        // 重要: MyGame::Initializeと同様に、コマンドリスト管理を行う
        if (currentScene_) {
            // (A) コマンドリストをリセット（書き込み可能に）
            DX12Context::GetInstance()->GetCommandList()->Reset(DX12Context::GetInstance()->GetCommandAllocator(), nullptr);

            // (B) シーンの初期化（ロード命令の書き込み）
            currentScene_->Initialize();

            // (C) コマンド実行と待機
            DX12Context::GetInstance()->ExecuteInitialCommandAndSync();

            // (D) 中間リソースの解放
            TextureManager::GetInstance()->ReleaseIntermediateResources();
            ParticleManager::GetInstance()->ReleaseIntermediateResources();
        }
    }

    // 現在のシーンを更新
    if (currentScene_) {
        currentScene_->Update();
    }
}

void SceneManager::Draw() { currentScene_->Draw(); }

void SceneManager::ChangeScene(const std::string& sceneName) {
    if (!sceneFactory_) {
        // ファクトリーがセットされていない場合のエラー処理
        return;
    }

    // 次のシーンを生成して nextScene_ にセット
    nextScene_ = sceneFactory_->CreateScene(sceneName);
}
