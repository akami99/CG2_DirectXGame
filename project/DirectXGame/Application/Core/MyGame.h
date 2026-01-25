#pragma once
#include "RAFramework.h" // RAFrameworkをインクルード

// 必要な前方宣言やインクルード
#include "AudioManager.h"
#include "DebugCamera.h"
#include "MathTypes.h"
#include <vector>

class GamePlayScene;

// Frameworkを継承する
class MyGame : public RAFramework {
public: // メンバ関数
    // オーバーライドすることを明示
    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;


private: // メンバ変数
    // ゲームプレイシーン
  GamePlayScene *gamePlayScene_ = nullptr;

};
