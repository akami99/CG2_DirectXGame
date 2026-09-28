#pragma once
#include "BaseScene.h"
#include <vector>
#include <memory>
#include "MathTypes.h"

// 必要なクラスをインクルード or 前方宣言
#include "DebugCamera.h"
#include "Camera.h"
#include "Sprite.h"

class ClearScene : public BaseScene {
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

    // シューティングシーンから結果データ（スコアと残り時間）を受け取る
    static void SetResultData(int score, float remainingTime) {
        clearScore_ = score;
        clearRemainingTime_ = remainingTime;
    }

private:
    // ゲームカメラの更新
    void UpdateGameCamera();

    // ImGui操作の更新
    void UpdateImGui();

private:
    // カメラ
    std::unique_ptr<Camera> camera_;
    DebugCamera debugCamera_;
    bool useDebugCamera_ = false;
    Vector3 gameCameraRotate_{};
    Vector3 gameCameraTranslate_{};

    // UIスプライト
    std::unique_ptr<Sprite> backGroundSprite_;
    std::unique_ptr<Sprite> clearTextSprite_;
    std::unique_ptr<Sprite> timeTextSprite_;
    std::unique_ptr<Sprite> scoreTextSprite_;

    // タイム用数字スプライト（4桁 + ドット）
    std::vector<std::unique_ptr<Sprite>> timeDigits_;
    std::unique_ptr<Sprite> timeDot_;

    // スコア用数字スプライト（4桁）
    std::vector<std::unique_ptr<Sprite>> scoreDigits_;

    // 設定
    int currentBlendMode_ = 1;  // NormalBlend
    bool isShowSprite_ = true;

    // パス定数
    const std::string backGroundPath_ = "floor.png";
    const std::string whitePath_ = "white.png";
    const std::string clearTextPath_ = "ui/clearText.png";
    const std::string timeTextPath_ = "ui/timeText.png";
    const std::string scoreTextPath_ = "ui/scoreText.png";

    // 静的結果データ
    static int clearScore_;
    static float clearRemainingTime_;
};