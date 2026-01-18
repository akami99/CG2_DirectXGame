#include "MyGame.h"

#include <cassert>

#include <numbers> //particleに使用
#include <random>  //particleに使用

// フレームワーク基盤
#include "RAFramework.h"
// エンジン
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"
#include "Model.h"
// 共通系
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "ModelCommon.h"
// 管理系
#include "ModelManager.h"
#include "TextureManager.h"
#include "ParticleManager.h"
#include "ImGuiManager.h"
#include "SrvManager.h"

// グラフィック関連の構造体
#include "LightTypes.h"
#include "ParticleTypes.h"

// 計算用関数など
#include "MathUtils.h"
#include "MatrixGenerators.h"

// ゲーム内設定用
#include "ApplicationConfig.h"

// .lidはヘッダに書いてはいけない
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace MathUtils;
using namespace MathGenerators;
using namespace BlendMode;

void MyGame::Initialize() {
  // --- 基盤システムの初期化は親クラスに任せる ---
  RAFramework::Initialize();

  // --- ゲーム内オブジェクトの初期化 ---

  // --- 音声系の初期化 ---
  audioManager_.Initialize();

  // カメラ
  camera_ = new Camera();
  camera_->Initialize(dxBase_);
  camera_->SetRotate({0.0f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 0.0f, -10.0f});
  object3dCommon_->SetDefaultCamera(camera_);

  gameCameraRotate_ = camera_->GetRotate();
  gameCameraTranslate_ = camera_->GetTranslate();

  // .objファイルからモデルを呼び込む
  ModelManager::GetInstance()->LoadModel("Plane", planeModel_);
  // modelManager_->LoadModel("Axis", axisModel_);
  ModelManager::GetInstance()->LoadModel("Teapot", teapotModel_);

  // パーティクルグループの作成
  // エミッターの設定と登録
  ParticleEmitter defaultEmitter = ParticleEmitter(
      Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}},
      3,   // count: 1回で3個生成
      0.2f // frequency: 0.2秒ごとに発生
  );
  TextureManager::GetInstance()->LoadTexture(particleTexturePath_);
  ParticleManager::GetInstance()->CreateParticleGroup(particleGroupName_,
                                        particleTexturePath_);
  ParticleManager::GetInstance()->SetEmitter(particleGroupName_, defaultEmitter);

  // テクスチャファイルパスを保持
  TextureManager::GetInstance()->LoadTexture(uvCheckerPath_);

  TextureManager::GetInstance()->LoadTexture(monsterBallPath_);

  // コマンド実行と完了待機
  dxBase_->ExecuteInitialCommandAndSync();
  TextureManager::GetInstance()->ReleaseIntermediateResources();
  ParticleManager::GetInstance()->ReleaseIntermediateResources();

  // AudioManager を初期化
  if (!audioManager_.Initialize()) {
      // 初期化失敗時の処理
    assert(false && "AudioManager is not Initialize");
  }

  // 音声ファイルをロード
  if (!audioManager_.LoadSound("alarm1", "Alarm01.mp3")) {
      // ロード失敗時の処理
    assert(false && "Failed to load sound: Alarm01.mp3");
  }

  // マテリアル

  // 1つ目のオブジェクト
  Object3d *object3d_1 = new Object3d(); // object3dをobject3d_1に変更
  object3d_1->Initialize(object3dCommon_);

  // モデルの設定
  object3d_1->SetModel(planeModel_);
  // オブジェクトの位置を設定
  object3d_1->SetTranslate({ -2.0f, 0.0f, 0.0f });

  // 2つ目のオブジェクトを新規作成
  Object3d *object3d_2 = new Object3d();
  object3d_2->Initialize(object3dCommon_);

  // モデルの設定
  object3d_2->SetModel(teapotModel_);
  // オブジェクトの位置を設定
  object3d_2->SetTranslate({ 2.0f, 0.0f, 0.0f });

  // Object3dを格納する
  object3ds_.push_back(object3d_1);
  object3ds_.push_back(object3d_2);

  // 描画サイズ
  const float spriteCutSize = 64.0f;
  const float spriteScale = 64.0f;
  // 生成
  for (uint32_t i = 0; i < 10; ++i) {
      Sprite *newSprite = new Sprite();
      newSprite->Initialize(spriteCommon_, uvCheckerPath_);
      newSprite->SetAnchorPoint({ 0.5f, 0.5f });
      if (i == 7) {
          newSprite->SetFlipX(1);
          newSprite->SetFlipY(1);
      } else if (i == 8) {
          newSprite->SetFlipX(1);
          newSprite->SetTextureLeftTop({ 0.0f, 0.0f });
          newSprite->SetTextureSize({ spriteCutSize, spriteCutSize });
          newSprite->SetScale({ spriteScale, spriteScale });
      } else if (i == 9) {
          newSprite->SetTextureLeftTop({ 0.0f, 0.0f });
          newSprite->SetTextureSize({ spriteCutSize, spriteCutSize });
          newSprite->SetScale({ spriteScale, spriteScale });
      }
      newSprite->SetTranslate(
          { float(i * spriteScale / 3), float(i * spriteScale / 3) });
      sprites_.push_back(newSprite);
  }
}

void MyGame::Finalize() {
  // --- 終了処理 ---

  // 個別オブジェクトの解放
  for (auto s : sprites_) {
    delete s;
  }
  for (auto o : object3ds_) {
    delete o;
  }
  delete camera_;

  // --- 基盤システムの終了処理は親クラスに任せる ---
  RAFramework::Finalize();
}

void MyGame::Update() {
    // 1. ImGuiの受付開始
    imGuiManager_->Begin();

    // 2. 入力の更新
    input_->Update();

    // 3. UI処理 (ImGuiの定義)
#ifdef USE_IMGUI
    ImGui::SetNextWindowPos(ImVec2(Win32Window::kClientWidth - 10.0f, 10.0f), ImGuiCond_Once, ImVec2(1.0f, 0.0f));
    ImGui::Begin("Settings");

    if (ImGui::TreeNode("Global Settings")) {
        ImGui::Checkbox("useDebugCamera", &useDebugCamera_);
        ImGui::Combo("BlendMode", &currentBlendMode_, "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");
        ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("GameCamera")) {
        ImGui::DragFloat3("rotate", &gameCameraRotate_.x, 0.01f);
        ImGui::DragFloat3("translate", &gameCameraTranslate_.x, 0.01f);
        ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Object3d")) {
        ImGui::Checkbox("showMaterial", &showMaterial_);

        // ここからループ処理
        for (size_t i = 0; i < object3ds_.size(); ++i) {
            Object3d *object3d = object3ds_[i]; // 現在のオブジェクトを取得

            // ノードのラベルを動的に生成
            std::string label = "Object3d " + std::to_string(i + 1);

            if (ImGui::TreeNode(label.c_str())) {
                Vector3 scale = object3d->GetScale();
                Vector3 rotate = object3d->GetRotation();
                Vector3 translate = object3d->GetTranslate();

                ImGui::DragFloat3("scale", &scale.x, 0.01f);
                ImGui::DragFloat3("rotate", &rotate.x, 0.01f);
                ImGui::DragFloat3("translate", &translate.x, 0.01f);

                object3d->SetScale(scale);
                object3d->SetRotation(rotate);
                object3d->SetTranslate(translate);

                ImGui::Separator();

                if (ImGui::TreeNode(("Light_" + std::to_string(i + 1)).c_str())) {
                    Vector4 color = object3d->GetDirectionalLightColor();
                    Vector3 direction = object3d->GetDirectionalLightDirection();
                    float intensity = object3d->GetDirectionalLightIntensity();

                    ImGui::ColorEdit4("colorLight", &color.x);
                    ImGui::DragFloat3("directionLight", &direction.x, 0.01f);
                    ImGui::DragFloat("intensityLight", &intensity, 0.01f);

                    object3d->SetDirectionalLightColor(color);
                    object3d->SetRotation(rotate);
                    object3d->SetDirectionalLightDirection(direction);
                    object3d->SetDirectionalLightIntensity(intensity);

                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        // ループ処理ここまで

        ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Particle")) {
        ImGui::Checkbox("update", &isUpdateParticle_);
        ImGui::Checkbox("useBillboard", &useBillboard_);
        // ImGui::Checkbox("changeTexture", &changeTexture);
        ImGui::SliderFloat3("rotateParticleZ", &particleRotation_.x, -6.28f,
            6.28f);
        if (ImGui::Button("Generate Particle(SPACE)")) {
            // エミッターの Transform, count を直接指定して、一度だけEmitを呼び出す

            // 例: プレイヤー位置 {10.0f, 0.0f, 0.0f}
            // から、10個のパーティクルを一度に生成
            Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
            uint32_t burstCount = 10;

            ParticleManager::GetInstance()->Emit(particleGroupName_, playerPos,
                burstCount);
        }
        ImGui::Combo("ParticleBlendMode", &particleBlendMode_,
            "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");
        ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Sprite")) {

        ImGui::Checkbox("showSprite", &showSprite_);

        for (size_t i = 0; i < sprites_.size(); ++i) {
            std::string label = "Sprite " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str())) {
                Sprite *sprite = sprites_[i];

                Vector2 spritePosition = sprite->GetTranslate();
                float spriteRotation = sprite->GetRotation();
                Vector2 spriteScale = sprite->GetScale();
                Vector4 spriteColor = sprite->GetColor();

                ImGui::SliderFloat2("translateSprite", &spritePosition.x, 0.0f,
                    1000.0f);
                ImGui::SliderFloat("rotateSprite", &spriteRotation, -6.28f, 6.28f);
                ImGui::SliderFloat2("scaleSprite", &spriteScale.x, 1.0f,
                    Win32Window::kClientWidth);
                ImGui::ColorEdit4("colorSprite", &spriteColor.x);
                sprite->SetTranslate(spritePosition);
                sprite->SetRotation(spriteRotation);
                sprite->SetScale(spriteScale);
                sprite->SetColor(spriteColor);

                ImGui::Separator();
                ImGui::Text("UV");

                // UVTransform用の変数を用意
                Transform uvTransformSprite = {};
                uvTransformSprite.translate = sprite->GetUvTranslate();
                uvTransformSprite.scale = sprite->GetUvScale();
                uvTransformSprite.rotate = sprite->GetUvRotation();

                ImGui::DragFloat2("uvTranslate", &uvTransformSprite.translate.x,
                    0.01f, 0.0f, 1.0f);
                ImGui::DragFloat2("uvScale", &uvTransformSprite.scale.x, 0.01f,
                    -10.0f, 10.0f);
                ImGui::SliderAngle("uvRotate", &uvTransformSprite.rotate.z);
                sprite->SetUvTranslate(uvTransformSprite.translate);
                sprite->SetUvScale(uvTransformSprite.scale);
                sprite->SetUvRotation(uvTransformSprite.rotate);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Sound")) {
        if (ImGui::Button("Play Sound")) {
            // サウンドの再生
            audioManager_.PlaySound("alarm1");
        }
        ImGui::TreePop();
    }

    ImGui::End(); // "Settings" ウィンドウの終了
#endif

    // 4. ゲームロジックの更新

    // 音声のクリーンアップ
    audioManager_.CleanupFinishedVoices();

    // カメラの更新処理
    if (!useDebugCamera_) {
        // キー入力によるカメラ操作
        if (input_->IsKeyDown(DIK_A)) { gameCameraTranslate_.x -= 0.1f; }
        if (input_->IsKeyDown(DIK_D)) { gameCameraTranslate_.x += 0.1f; }
        if (input_->IsKeyDown(DIK_W)) { gameCameraTranslate_.y += 0.1f; }
        if (input_->IsKeyDown(DIK_S)) { gameCameraTranslate_.y -= 0.1f; }
        if (input_->IsKeyDown(DIK_Q)) { gameCameraTranslate_.z += 0.1f; }
        if (input_->IsKeyDown(DIK_E)) { gameCameraTranslate_.z -= 0.1f; }
        if (input_->IsKeyDown(DIK_C)) { gameCameraRotate_.y += 0.02f; }
        if (input_->IsKeyDown(DIK_Z)) { gameCameraRotate_.y -= 0.02f; }
        if (input_->IsKeyReleased(DIK_X)) { gameCameraRotate_.y = 0.0f; }

        camera_->SetRotate(gameCameraRotate_);
        camera_->SetTranslate(gameCameraTranslate_);
    } else {
        debugCamera_.Update(*input_);
        camera_->SetRotate(debugCamera_.GetRotation());
        camera_->SetTranslate(debugCamera_.GetTranslate());
    }
    camera_->Update();

    // パーティクルの更新
    // ※ kDeltaTime は ApplicationConfig.h から読み込むか、メンバ変数として管理する
    ParticleManager::GetInstance()->Update(*camera_, isUpdateParticle_, useBillboard_, kDeltaTime);

    // スペースキーでのパーティクル生成テスト
    if (input_->IsKeyTriggered(DIK_SPACE)) {
        Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
        ParticleManager::GetInstance()->Emit(particleGroupName_, playerPos, 10);
    }

    // オブジェクトの更新
    for (auto *obj : object3ds_) {
        obj->Update();
    }

    // スプライトの更新
    for (auto *sprite : sprites_) {
        sprite->Update();
    }

    // 5. ImGuiの受付終了 (内部的な終了処理)
    imGuiManager_->End();
}

void MyGame::Draw() {
    // 1. 描画前処理
    dxBase_->PreDraw();
    srvManager_->PreDraw(); // SRV用のヒープをセット

    // 2. 3Dオブジェクトの描画
    object3dCommon_->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_), pipelineManager_);

    if (showMaterial_) {
        for (auto *obj : object3ds_) {
            obj->Draw();
        }
    }

    // 3. パーティクルの描画
    ParticleManager::GetInstance()->Draw(static_cast<BlendState>(particleBlendMode_));

    // 4. スプライトの描画
    spriteCommon_->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_), pipelineManager_);

    if (showSprite_) {
        for (size_t i = 0; i < sprites_.size(); ++i) {
            // テクスチャの差し替え（特定の要素だけ変える処理）
            if (i == 6) {
                // ※ パスを持っているならここでセット
                sprites_[i]->SetTexture(monsterBallPath_);
            }
            sprites_[i]->Draw();
        }
    }

    // 5. ImGuiの描画 (Updateで作られたUIを描画コマンドに乗せる)
    imGuiManager_->Draw();

    // 6. 描画後処理 (画面の入れ替え)
    dxBase_->PostDraw();
}