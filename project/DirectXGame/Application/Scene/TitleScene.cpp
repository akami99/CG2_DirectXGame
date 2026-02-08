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
    TextureManager::GetInstance()->LoadTexture(grassPath_);

    // --- スプライト生成 ---
    // 描画サイズ
    const float spriteScale = 64.0f;
    // 生成
    for (uint32_t i = 0; i < 1; ++i) {
        // 一旦ユニークポインタで作る
        std::unique_ptr<Sprite> newSprite = std::make_unique<Sprite>();
        newSprite->Initialize(grassPath_);
        newSprite->SetAnchorPoint({ 0.5f, 0.5f });
        newSprite->SetTranslate(
            { float(i * spriteScale / 3), float(i * spriteScale / 3) });
        // 配列に「所有権を移動（move）」して追加する
        sprites_.push_back(std::move(newSprite));
    }
}

void TitleScene::Update() {
    // 入力の更新
    Input::GetInstance()->Update();
    // 3. UI処理 (ImGuiの定義)
    UpdateImGui();

    // カメラの更新処理
    UpdateGameCamera();

    if (Input::GetInstance()->IsKeyDown(DIK_RETURN)) {
        // ★ 文字列で指定するだけ！
        SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
    }

    // --- スプライトの更新 ---
     // unique_ptrが入っている配列を回すときは const auto& を使うと良い
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
        // Drawも同様
        for (const auto& sprite : sprites_) {
            sprite->Draw();
        }
    }
}

void TitleScene::Finalize() {
    // Object3dCommonの参照をクリア（次のシーン切り替え前に）
    Object3dCommon::GetInstance()->SetDefaultCamera(nullptr);

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
    ImGui::Text("Title");
    ImGui::End();

#endif
}