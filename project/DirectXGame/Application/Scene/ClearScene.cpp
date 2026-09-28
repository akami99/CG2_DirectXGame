#include "ClearScene.h"
#include "DX12Context.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "ParticleManager.h"
#include "Input.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "BlendMode.h"
#include "SceneManager.h"
#include "ApplicationConfig.h" // kDeltaTimeなど
// 計算用関数など
#include "MathUtils.h"
#include "MatrixGenerators.h"

// using
using namespace MathUtils;
using namespace MathGenerators;
using namespace BlendMode;

// 静的メンバ変数の実体
int ClearScene::clearScore_ = 0;
float ClearScene::clearRemainingTime_ = 0.0f;

void ClearScene::Initialize()
{
    // カメラ生成
    camera_ = std::make_unique<Camera>();
    camera_->Initialize();
    camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
    camera_->SetTranslate({ 0.0f, 0.0f, -10.0f });

    gameCameraRotate_ = camera_->GetRotate();
    gameCameraTranslate_ = camera_->GetTranslate();

    debugCamera_.Initialize();

    // テクスチャ読み込み
    TextureManager::GetInstance()->LoadTexture(backGroundPath_);
    TextureManager::GetInstance()->LoadTexture(whitePath_);
    TextureManager::GetInstance()->LoadTexture(clearTextPath_);
    TextureManager::GetInstance()->LoadTexture(timeTextPath_);
    TextureManager::GetInstance()->LoadTexture(scoreTextPath_);
    for (int i = 0; i < 10; ++i) {
        TextureManager::GetInstance()->LoadTexture("numbers/" + std::to_string(i) + ".png");
    }

    // 0. 背景スプライト
    backGroundSprite_ = std::make_unique<Sprite>();
    backGroundSprite_->Initialize(backGroundPath_);
    backGroundSprite_->SetAnchorPoint({ 0.0f, 0.0f });
    backGroundSprite_->SetTranslate({ 0.0f, 0.0f });
    backGroundSprite_->SetScale({ 1280.0f, 720.0f });

    // 1. クリアテキスト（画面上部中央）
    clearTextSprite_ = std::make_unique<Sprite>();
    clearTextSprite_->Initialize(clearTextPath_);
    clearTextSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    clearTextSprite_->SetTranslate({ 640.0f, 150.0f });
    clearTextSprite_->SetScale({ 420.0f, 105.0f });

    // 2. タイムテキスト & 経過時間数値スプライト（クリアテキスト中央の少し左下に配置）
    float elapsedTime = 60.0f - clearRemainingTime_;
    if (elapsedTime < 0.0f) elapsedTime = 0.0f;
    if (elapsedTime > 99.99f) elapsedTime = 99.99f;

    timeTextSprite_ = std::make_unique<Sprite>();
    timeTextSprite_->Initialize(timeTextPath_);
    timeTextSprite_->SetAnchorPoint({ 0.0f, 0.5f });
    timeTextSprite_->SetTranslate({ 360.0f, 330.0f });
    timeTextSprite_->SetScale({ 220.0f, 55.0f });

    int tempTime = static_cast<int>(std::round(elapsedTime * 100.0f));
    if (tempTime > 9999) tempTime = 9999;
    int tDigits[4] = { tempTime / 1000, (tempTime % 1000) / 100, (tempTime % 100) / 10, tempTime % 10 };

    timeDigits_.clear();
    for (int i = 0; i < 4; ++i) {
        auto unit = std::make_unique<Sprite>();
        unit->Initialize("numbers/" + std::to_string(tDigits[i]) + ".png");
        unit->SetAnchorPoint({ 0.0f, 0.5f });
        float x = 700.0f + i * 36.0f;
        if (i >= 2) {
            x += 12.0f; // ドット用の隙間
        }
        unit->SetTranslate({ x, 325.0f });
        unit->SetScale({ 28.0f, 44.0f });
        timeDigits_.push_back(std::move(unit));
    }

    timeDot_ = std::make_unique<Sprite>();
    timeDot_->Initialize(whitePath_);
    timeDot_->SetAnchorPoint({ 0.5f, 0.5f });
    timeDot_->SetTranslate({ 700.0f + 2 * 36.0f + 5.0f, 346.0f });
    timeDot_->SetScale({ 6.0f, 6.0f });

    // 3. スコアテキスト & スコア数値スプライト（タイムテキストの下に同じサイズで配置）
    scoreTextSprite_ = std::make_unique<Sprite>();
    scoreTextSprite_->Initialize(scoreTextPath_);
    scoreTextSprite_->SetAnchorPoint({ 0.0f, 0.5f });
    scoreTextSprite_->SetTranslate({ 360.0f, 460.0f });
    scoreTextSprite_->SetScale({ 220.0f, 55.0f });

    int tempScore = clearScore_;
    if (tempScore > 9999) tempScore = 9999;
    if (tempScore < 0) tempScore = 0;
    int sDigits[4] = { tempScore / 1000, (tempScore % 1000) / 100, (tempScore % 100) / 10, tempScore % 10 };

    scoreDigits_.clear();
    for (int i = 0; i < 4; ++i) {
        auto unit = std::make_unique<Sprite>();
        unit->Initialize("numbers/" + std::to_string(sDigits[i]) + ".png");
        unit->SetAnchorPoint({ 0.0f, 0.5f });
        float x = 710.0f + i * 36.0f;
        unit->SetTranslate({ x, 455.0f });
        unit->SetScale({ 28.0f, 44.0f });
        scoreDigits_.push_back(std::move(unit));
    }
}

void ClearScene::Update()
{
    // 入力の更新
    Input::GetInstance()->Update();
    // 3. UI処理 (ImGuiの定義)
    UpdateImGui();

    // カメラの更新処理
    UpdateGameCamera();

    // ゲームオーバー時と同様のキー操作・分岐（ENTER / SPACE でタイトルへ）
    if (Input::GetInstance()->IsKeyTriggered(DIK_RETURN) || Input::GetInstance()->IsKeyTriggered(DIK_SPACE)) {
        SceneManager::GetInstance()->ChangeScene("TITLE");
    }

    // スプライトの更新
    if (backGroundSprite_) backGroundSprite_->Update();
    if (clearTextSprite_) clearTextSprite_->Update();
    if (timeTextSprite_) timeTextSprite_->Update();
    if (scoreTextSprite_) scoreTextSprite_->Update();
    for (const auto& unit : timeDigits_) {
        if (unit) unit->Update();
    }
    if (timeDot_) timeDot_->Update();
    for (const auto& unit : scoreDigits_) {
        if (unit) unit->Update();
    }
}

void ClearScene::Draw()
{
    // スプライトの描画
    SpriteCommon::GetInstance()->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode_));

    if (isShowSprite_) {
        // 背景スプライトを最背面に描画
        if (backGroundSprite_) backGroundSprite_->Draw();

        if (clearTextSprite_) clearTextSprite_->Draw();
        if (timeTextSprite_) timeTextSprite_->Draw();
        if (scoreTextSprite_) scoreTextSprite_->Draw();
        for (const auto& unit : timeDigits_) {
            if (unit) unit->Draw();
        }
        if (timeDot_) timeDot_->Draw();
        for (const auto& unit : scoreDigits_) {
            if (unit) unit->Draw();
        }
    }
}

void ClearScene::Finalize()
{
    Object3dCommon::GetInstance()->SetDefaultCamera(nullptr);

    backGroundSprite_.reset();
    clearTextSprite_.reset();
    timeTextSprite_.reset();
    scoreTextSprite_.reset();
    timeDigits_.clear();
    timeDot_.reset();
    scoreDigits_.clear();
}

void ClearScene::UpdateGameCamera()
{
    if (Input::GetInstance()->IsKeyTriggered(DIK_F1)) {
        useDebugCamera_ = !useDebugCamera_;
    }

    if (!useDebugCamera_) {
        camera_->SetTranslate(gameCameraTranslate_);
    } else {
        debugCamera_.Update();
        camera_->SetRotate(debugCamera_.GetRotation());
        camera_->SetTranslate(debugCamera_.GetTranslate());
    }
    camera_->Update();
}

void ClearScene::UpdateImGui()
{
#ifdef USE_IMGUI
    ImGui::Begin("Scene");
    ImGui::Text("Clear Scene");
    float elapsedTime = 60.0f - clearRemainingTime_;
    if (elapsedTime < 0.0f) elapsedTime = 0.0f;
    ImGui::Text("Clear Time : %.2f s", elapsedTime);
    ImGui::Text("Clear Score: %d", clearScore_);
    ImGui::Separator();
    ImGui::Text("Press ENTER or SPACE to return to Title");
    if (ImGui::Button("Return to Title")) {
        SceneManager::GetInstance()->ChangeScene("TITLE");
    }
    ImGui::SameLine();
    if (ImGui::Button("Retry Shooting")) {
        SceneManager::GetInstance()->ChangeScene("SHOOTING");
    }
    ImGui::End();
#endif
}