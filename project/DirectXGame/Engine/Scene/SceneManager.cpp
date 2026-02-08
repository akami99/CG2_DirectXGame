#include "SceneManager.h"
#include "DX12Context.h"
#include "TextureManager.h"
#include "ParticleManager.h"

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
    // 旧シーンの終了
    if (currentScene_) {
      currentScene_->Finalize();
    }

    // 新シーンの切り替え処理
    currentScene_ = std::move(nextScene_);
    nextScene_ = nullptr;

    // 1. コマンドリストをリセットして「書き込み可能状態」にする
    //    (コマンドアロケータは再利用する)
    DX12Context::GetInstance()->GetCommandList()->Reset(DX12Context::GetInstance()->GetCommandAllocator(), nullptr);

    // 2. シーンの初期化（ここでコマンドリストにロード命令などが積まれる）
    currentScene_->Initialize();

    // 3. コマンドを実行して、完了まで待機する
    //    (リストをCloseしてExecuteし、Fenceで待つ処理がここに含まれている想定)
    DX12Context::GetInstance()->ExecuteInitialCommandAndSync();
    TextureManager::GetInstance()->ReleaseIntermediateResources();
    ParticleManager::GetInstance()->ReleaseIntermediateResources();
  }

  // 現在のシーンを更新
  if (currentScene_) {
    currentScene_->Update();
  }
}

void SceneManager::Draw() { currentScene_->Draw(); }