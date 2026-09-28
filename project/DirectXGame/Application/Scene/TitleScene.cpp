#include "TitleScene.h"
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

void TitleScene::Initialize() {
    // カメラ生成
    camera_ = std::make_unique<Camera>(); // メモリ確保と同時にスマートポインタ化
    camera_->Initialize();
    camera_->SetRotate({ 0.3f, 0.0f, 0.0f });
    camera_->SetTranslate({ 0.0f, 4.0f, -10.0f });

    gameCameraRotate_ = camera_->GetRotate();
    gameCameraTranslate_ = camera_->GetTranslate();

    debugCamera_.Initialize();

    // テクスチャ読み込み
    TextureManager::GetInstance()->LoadTexture(backGroundPath_);
    TextureManager::GetInstance()->LoadTexture(titleTextPath_);

    // --- 背景スプライト生成 ---
    backGroundSprite_ = std::make_unique<Sprite>();
    backGroundSprite_->Initialize(backGroundPath_);
    backGroundSprite_->SetAnchorPoint({ 0.0f, 0.0f });
    backGroundSprite_->SetTranslate({ 0.0f, 0.0f });
    backGroundSprite_->SetScale({ 1280.0f, 720.0f });

    // --- タイトルテキストスプライト生成 ---
    titleTextSprite_ = std::make_unique<Sprite>();
    titleTextSprite_->Initialize(titleTextPath_);
    titleTextSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    titleTextSprite_->SetTranslate({ 640.0f, 180.0f });
}

void TitleScene::Update() {
    // 入力の更新
    Input::GetInstance()->Update();
    // 3. UI処理 (ImGuiの定義)
    UpdateImGui();

    // カメラの更新処理
    UpdateGameCamera();

    if (Input::GetInstance()->IsKeyTriggered(DIK_RETURN) || Input::GetInstance()->IsKeyTriggered(DIK_SPACE)) {
        SceneManager::GetInstance()->ChangeScene("SHOOTING");
    }

    // --- 背景スプライトの更新 ---
    if (backGroundSprite_) {
        backGroundSprite_->Update();
    }

    // --- タイトルテキストスプライトの更新 ---
    if (titleTextSprite_) {
        titleTextSprite_->Update();
    }

    // --- スプライトの更新 ---
    for (const auto& sprite : sprites_) {
        sprite->Update();
    }
}

void TitleScene::Draw() {
    // スプライトの描画
    // 描画設定
    SpriteCommon::GetInstance()->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode_));

    if (isShowSprite_) {
        // 背景スプライトを最背面に描画
        if (backGroundSprite_) {
            backGroundSprite_->Draw();
        }

        // タイトルテキストを描画
        if (titleTextSprite_) {
            titleTextSprite_->Draw();
        }

        // Drawも同様
        for (const auto& sprite : sprites_) {
            sprite->Draw();
        }
    }
}

void TitleScene::Finalize() {
    // Object3dCommonの参照をクリア（次のシーン切り替え前に）
    Object3dCommon::GetInstance()->SetDefaultCamera(nullptr);

    backGroundSprite_.reset();
    titleTextSprite_.reset();
    sprites_.clear();
}

void TitleScene::UpdateGameCamera() {

    if (Input::GetInstance()->IsKeyTriggered(DIK_F1)) {
        useDebugCamera_ = !useDebugCamera_;
    }

    if (!useDebugCamera_) {
        // キー入力によるカメラ操作
        if (Input::GetInstance()->IsKeyDown(DIK_LEFT)) {
            gameCameraTranslate_.x -= 0.1f;
        }
        if (Input::GetInstance()->IsKeyDown(DIK_RIGHT)) {
            gameCameraTranslate_.x += 0.1f;
        }
        if (Input::GetInstance()->IsKeyDown(DIK_UP)) {
            gameCameraTranslate_.y += 0.1f;
        }
        if (Input::GetInstance()->IsKeyDown(DIK_DOWN)) {
            gameCameraTranslate_.y -= 0.1f;
        }
        // 共通のリセットキー
        if (Input::GetInstance()->IsKeyDown(DIK_R)) {
            gameCameraTranslate_ = { 0.0f, 2.0f, -15.0f };
        }

        camera_->SetTranslate(gameCameraTranslate_);
    } else {
        debugCamera_.Update();
        camera_->SetRotate(debugCamera_.GetRotation());
        camera_->SetTranslate(debugCamera_.GetTranslate());
    }
    camera_->Update();
}

void TitleScene::UpdateImGui() {
#ifdef USE_IMGUI
    // シーンの表示
    ImGui::Begin("Scene");
    ImGui::Text("Title Scene");
    ImGui::Text("Press ENTER or SPACE to start");
    if (ImGui::Button("Start Shooting")) {
        SceneManager::GetInstance()->ChangeScene("SHOOTING");
    }
    ImGui::End();
#endif
}