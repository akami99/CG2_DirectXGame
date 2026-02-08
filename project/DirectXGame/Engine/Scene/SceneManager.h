#pragma once
#include "BaseScene.h"
#include <memory>

// シーン管理
class SceneManager {
public:
    static SceneManager* GetInstance();

    // 終了処理
    void Finalize();

    // 破棄用の関数
    static void Destroy();

    // 更新処理（シーンの切り替えチェックもここで行う）
    void Update();

    // 描画処理
    void Draw();

    // 次のシーンを予約する
    void SetNextScene(std::unique_ptr<BaseScene> nextScene) { nextScene_ = std::move(nextScene); }

private:
    SceneManager() = default;
    ~SceneManager() = default;
    static SceneManager* instance_;

    // 現在のシーン
    std::unique_ptr<BaseScene> currentScene_ = nullptr;
    // 次のシーン（予約用）
    std::unique_ptr<BaseScene> nextScene_ = nullptr;
};