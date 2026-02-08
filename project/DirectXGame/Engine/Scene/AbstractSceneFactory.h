#pragma once
#include "BaseScene.h"
#include <memory>
#include <string>

// シーン生成工場のインターフェース
class AbstractSceneFactory {
public:
    virtual ~AbstractSceneFactory() = default;

    // 純粋仮想関数：シーン名を指定して、そのユニークポインタを返す
    virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
};