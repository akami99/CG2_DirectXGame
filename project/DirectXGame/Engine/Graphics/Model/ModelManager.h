#pragma once

#include <map>
#include <string>
#include <memory>

// 前方宣言
class DX12Context;
class ModelCommon;
class Model;

// モデルマネージャー
class ModelManager {
private: // メンバ変数
  static ModelManager *instance_;

  // モデルデータ
  std::map<std::string, std::unique_ptr<Model>> models_;

  // モデルのポインタ
  ModelCommon *modelCommon_ = nullptr;

public: // メンバ関数
  // 初期化
  void Initialize(DX12Context *dxBase);

  // シングルトンインスタンスの取得
  static ModelManager *GetInstance();
  // 終了(この後にGetInstance()するとまたnewするので注意)
  void Finalize();

  /// <summary>
  /// モデルの検索
  /// </summary>
  /// <param name="filePath">モデルファイルのパス</param>
  /// <returns>モデル</returns>
  Model *FindModel(const std::string &filePath);

  /// <summary>
  /// モデルファイルの読み込み
  /// </summary>
  /// <param name="filePath">モデルファイルのパス</param>
  void LoadModel(const std::string &directoryPath, const std::string &filePath);

private: // メンバ関数
  // コンストラクタ(隠蔽)
  ModelManager() = default;
  // デストラクタ(隠蔽)
  ~ModelManager() = default;
  // コピーコンストラクタの封印
  ModelManager(ModelManager &) = delete;
  // コピー代入演算子の封印
  ModelManager &operator=(ModelManager &) = delete;
};
