#include "SceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName) {
    // 文字列に応じて、適切なシーンを make_unique して返す
    if (sceneName == "TITLE") {
        return std::make_unique<TitleScene>();
    } else if (sceneName == "GAMEPLAY") {
        return std::make_unique<GamePlayScene>();
    }

    // 未知のシーン名なら nullptr を返す（あるいはアサートで止める）
    return nullptr;
}