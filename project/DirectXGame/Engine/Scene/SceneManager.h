#pragma once
#include "BaseScene.h"
#include "AbstractSceneFactory.h"
#include <memory>
#include <string>

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

    // 文字列で次のシーンを予約する
    void ChangeScene(const std::string& sceneName);

    // ファクトリーをセットする（MyGame初期化時などに呼ぶ）
    void SetSceneFactory(std::unique_ptr<AbstractSceneFactory> sceneFactory) {
        sceneFactory_ = std::move(sceneFactory);
    }

private:
    SceneManager() = default;
    ~SceneManager() = default;
    static SceneManager* instance_;

    // シーン工場
    std::unique_ptr<AbstractSceneFactory> sceneFactory_;

    // 現在のシーン
    std::unique_ptr<BaseScene> currentScene_ = nullptr;
    // 次のシーン（予約用）
    std::unique_ptr<BaseScene> nextScene_ = nullptr;
};