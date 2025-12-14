#include "ModelManager.h"
#include "Model.h"
#include "ModelCommon.h"

#include "../../Core/Utility/Logger/Logger.h"
#include "../../Core/Utility/String/StringUtility.h"

#include <cassert>
#include <filesystem>
#include <format>
#include <utility>

using namespace Logger;
using namespace StringUtility;

ModelManager *ModelManager::instance_ = nullptr;

// 初期化
void ModelManager::Initialize(DX12Context *dxBase) {

  // モデルの初期化
  modelCommon_ = new ModelCommon();
  modelCommon_->Initialize(dxBase);
}

// シングルトンインスタンスの取得
ModelManager *ModelManager::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new ModelManager;
  }
  return instance_;
}

// 終了
void ModelManager::Finalize() {
  delete instance_;
  instance_ = nullptr;
}

Model *ModelManager::FindModel(const std::string &filePath) {
  // --- 読み込み済みモデルを検索し、存在すればreturn ---
  if (models_.contains(filePath)) {
    // 読み込み済みなのでreturn
    return models_.at(filePath).get();
  }
  // ファイル名一致無し
  return nullptr;
}

void ModelManager::LoadModel(const std::string &directoryPath,
                             const std::string &filePath) {
    std::string fulldirectoryPath = directoryPath;
    // 既にパスに"Resources/Assets/Models/"が含まれている場合は追加しない
    if (filePath.find("Resources/Assets/Models/") == std::string::npos &&
        filePath.find("resources/assets/models/") == std::string::npos) {
        fulldirectoryPath = "Resources/Assets/Models/" + directoryPath;
    }

    if (!std::filesystem::exists(fulldirectoryPath + "/" + filePath)) {
        Logger::Log("ERROR: Texture file not found at path: " + fulldirectoryPath + filePath);
        assert(false && "Texture file not found!");
    }

  // --- 読み込み済みモデルを検索し、存在すれば早期return ---
  if (models_.contains(filePath)) {
    // 読み込み済みなので何もしない
    return;
  }

  // --- モデルの生成とファイル読み込み、初期化 ---
  // unique_ptrでModelのインスタンスを生成
  std::unique_ptr<Model> model = std::make_unique<Model>();

  model->Initialize(modelCommon_, fulldirectoryPath, filePath);
  Log("INFO: Loaded model at path: " + fulldirectoryPath + "/" + filePath + "\n");

  // --- 4. モデルをマップコンテナに格納 ---
  // std::unique_ptr はコピー禁止なので、std::move() によって所有権を移譲する
  models_.insert(std::make_pair(filePath, std::move(model)));
}
