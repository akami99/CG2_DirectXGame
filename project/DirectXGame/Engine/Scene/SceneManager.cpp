#include "SceneManager.h"
#include "Logger.h"

SceneManager *SceneManager::instance_ = nullptr;

SceneManager *SceneManager::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new SceneManager;
  }
  return instance_;
}

void SceneManager::Finalize() {
      Logger::Log("=== SceneManager::Finalize Start ===\n");
    
    if (currentScene_) {
        Logger::Log("Finalizing current scene...\n");
        currentScene_->Finalize();
        
        Logger::Log("Deleting current scene...\n");
        delete currentScene_;
        currentScene_ = nullptr;
        
        Logger::Log("Current scene deleted.\n");
    }
    
    if (nextScene_) {
        nextScene_->Finalize();
        delete nextScene_;
        nextScene_ = nullptr;
    }
    
    Logger::Log("=== SceneManager::Finalize End ===\n");
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
      delete currentScene_;
    }

    // 新シーンの開始
    currentScene_ = nextScene_;
    nextScene_ = nullptr;

    currentScene_->Initialize();
  }

  // 現在のシーンを更新
  if (currentScene_) {
    currentScene_->Update();
  }
}

void SceneManager::Draw() { currentScene_->Draw(); }