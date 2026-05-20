#include "GamePlayScene.h"
#include "Win32Window.h"
#include "DX12Context.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "ParticleManager.h"
#include "AudioManager.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "Model.h"
#include "LightManager.h"
#include "SceneManager.h"
#include "ApplicationConfig.h" // kDeltaTimeなど
// 計算用関数など
#include "MathUtils.h"
#include "MatrixGenerators.h"

// scene
#include "TitleScene.h"

// using
using namespace MathUtils;
using namespace MathGenerators;
using namespace BlendMode;

void GamePlayScene::Initialize() {
    // カメラ生成
    camera_ = std::make_unique<Camera>();
    camera_->Initialize();
    camera_->SetRotate(kDefaultCameraRotate);
    camera_->SetTranslate(kDefaultCameraTranslate);
    gameCameraRotate_ = camera_->GetRotate();

    gameCameraTranslate_ = camera_->GetTranslate();

    debugCamera_.Initialize();

    Object3dCommon::GetInstance()->SetDefaultCamera(camera_.get());

    // ライトの設定
    LightManager::GetInstance()->SetDirectionalLightIntensity(0.0f);
    LightManager::GetInstance()->SetPointLightIntensity(0.0f);

    // テクスチャ読み込み
    TextureManager::GetInstance()->LoadTexture(uvCheckerPath_);
    TextureManager::GetInstance()->LoadTexture(particleTexturePath_);
    TextureManager::GetInstance()->LoadTexture(monsterBallPath_);
    TextureManager::GetInstance()->LoadTexture(grassPath_);

    // モデル読み込み
    ModelManager::GetInstance()->LoadModel("sphere", sphereModel_);
    ModelManager::GetInstance()->LoadModel("plane", planeGltfModel_);
    // リングプリミティブの生成
    ModelManager::GetInstance()->CreateRing("ring", ringTexturePath_, 0.5f, 1.0f, 32);

    // パーティクル設定
    ParticleManager::GetInstance()->CreateParticleGroup(particleGroupName_, particleTexturePath_);
    // リングパーティクルの設定
    Model* ringModel = ModelManager::GetInstance()->FindModel("ring");
    ParticleManager::GetInstance()->CreateParticleGroup(ringParticleGroupName_, ringTexturePath_, ringModel);

    // エミッターの設定と登録
    ParticleEmitter defaultEmitter = ParticleEmitter(
        Transform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-1.0f, 1.0f, 0.0f} },
        3,   // count: 1回で3個生成
        0.2f // frequency: 0.2秒ごとに発生
    );
    ParticleManager::GetInstance()->SetEmitter(particleGroupName_,
        defaultEmitter);

    // リングエミッターの設定
    ParticleEmitter ringEmitter = ParticleEmitter(
        Transform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 2.0f, 0.0f} },
        2,
        0.2f
    );
    ringEmitter.ringSettings.isRing = true; // デフォルトでリング形状を使用
    ParticleManager::GetInstance()->SetEmitter(ringParticleGroupName_, ringEmitter);

    // 音声ファイルをロード
    if (!AudioManager::GetInstance()->LoadSound("alarm1", "Alarm01.mp3")) {
        // ロード失敗時の処理
        assert(false && "Failed to load sound: Alarm01.mp3");
    }

	// Skyboxの設定
	skybox_ = std::make_unique<Skybox>();
	// DDSテクスチャの読み込み (SrvManagerのインデックス指定で読み込む必要があるため、TextureManagerではなく直接SrvManagerを呼び出す)
	skybox_->Initialize("skybox.dds"); // DDSテクスチャを指定
	skybox_->SetCamera(camera_.get());

	// 環境マップとしてセット
	Object3dCommon::GetInstance()->SetEnvironmentMap(TextureManager::GetInstance()->GetSrvIndex("skybox.dds"));

    // --- 3Dオブジェクト生成 ---
    // マテリアル

    // 1つ目のオブジェクト
    std::unique_ptr <Object3d> object3d_1 = std::make_unique<Object3d>();
    object3d_1->Initialize();

    // モデルの設定
    object3d_1->SetModel(sphereModel_);
    // オブジェクトの位置を設定
    object3d_1->SetTranslate({ 0.0f, 0.0f, 0.0f });

     // 2つ目のオブジェクトを新規作成
    std::unique_ptr <Object3d> object3d_2 = std::make_unique<Object3d>();
     object3d_2->Initialize();

     // モデルの設定
     object3d_2->SetModel(planeGltfModel_);
     // オブジェクトの位置を設定
     object3d_2->SetTranslate({2.0f, 0.0f, 0.0f});

    // Object3dを格納する
    object3ds_.push_back(std::move(object3d_1));
    object3ds_.push_back(std::move(object3d_2));

    // --- スプライト生成 ---
    // 描画サイズ
    const float spriteCutSize = 64.0f;
    const float spriteScale = 64.0f;
    // 生成
    for (uint32_t i = 0; i < 10; ++i) {
        std::unique_ptr<Sprite> newSprite = std::make_unique<Sprite>();
        newSprite->Initialize(uvCheckerPath_);
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
        sprites_.push_back(std::move(newSprite));
    }

    // --- レベルデータ読み込み ---
    // ※ 実際のJSONファイルがある場合は、ファイル名を指定して読み込み
    // levelData_ = LevelLoader::LoadFile("testScene");
    if (levelData_) {
        CreateObjects(levelData_->objects);
    }
}

void GamePlayScene::Finalize() {
    // Object3dCommonの参照をクリア
    Object3dCommon::GetInstance()->SetDefaultCamera(nullptr);
    // Object3dを削除
    object3ds_.clear();

    // スプライトを削除
    sprites_.clear();
}

void GamePlayScene::Update() {
    // 入力の更新
    Input::GetInstance()->Update();
    // 3. UI処理 (ImGuiの定義)
    UpdateImGui();

    // 4. ゲームロジックの更新

    // 音声のクリーンアップ
    AudioManager::GetInstance()->CleanupFinishedVoices();

    // カメラの更新処理
    UpdateGameCamera();
    
    // Skyboxの更新
    if (skybox_) {
        skybox_->Update();
    }

    // パーティクルの更新
    // 状態を反映
    ParticleManager::GetInstance()->SetIsUpdate(isUpdateParticle_);
    ParticleManager::GetInstance()->SetUseBillboard(useBillboard_);

    // ※ kDeltaTime は ApplicationConfig.h
    // から読み込むか、メンバ変数として管理する
    ParticleManager::GetInstance()->Update(*camera_, kDeltaTime);


    if (!object3ds_.empty()) {
        // オブジェクトの操作
        if (!useDebugCamera_ && isShowMaterial_) {
            Vector3 pos = object3ds_[objectControlIndex_]->GetTranslate();
            Vector3 rot = object3ds_[objectControlIndex_]->GetRotation();
            float moveSpeed = kMoveSpeed;
            float rotSpeed = kRotateSpeed;

            if (Input::GetInstance()->IsKeyDown(DIK_A)) {
                pos.x -= moveSpeed;
            }
            if (Input::GetInstance()->IsKeyDown(DIK_D)) {
                pos.x += moveSpeed;
            }
            if (Input::GetInstance()->IsKeyDown(DIK_W)) {
                pos.z += moveSpeed;
            }
            if (Input::GetInstance()->IsKeyDown(DIK_S)) {
                pos.z -= moveSpeed;
            }
            if (Input::GetInstance()->IsKeyDown(DIK_Q)) {
                pos.y += moveSpeed;
            }
            if (Input::GetInstance()->IsKeyDown(DIK_E)) {
                pos.y -= moveSpeed;
            }
            if (Input::GetInstance()->IsKeyDown(DIK_Z)) {
                rot.y -= rotSpeed;
            }
            if (Input::GetInstance()->IsKeyDown(DIK_C)) {
                rot.y += rotSpeed;
            }
            // 共通のリセットキー
            if (Input::GetInstance()->IsKeyDown(DIK_R)) {
                pos = { 0.0f, 0.0f, 0.0f };
                rot = { 0.0f, 0.0f, 0.0f };
            }

            object3ds_[objectControlIndex_]->SetTranslate(pos);
            object3ds_[objectControlIndex_]->SetRotation(rot);
        }
        // オブジェクトの更新
        for (const auto& obj : object3ds_) {
            obj->Update();
        }
    }


    // スプライトの更新
    for (const auto& sprite : sprites_) {
        sprite->Update();
    }
}

void GamePlayScene::Draw() {
    // MyGame::Draw の中身を移動
    // ※ dxBase_->PreDraw() などは MyGame 側で行うので、ここでは「シーンの中身の描画」だけを行う

    // 描画設定
    Object3dCommon::GetInstance()->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode_));
    

    // 2. 3Dオブジェクトの描画

    if (isShowMaterial_ && !object3ds_.empty()) {
        for (const auto& obj : object3ds_) {
            obj->Draw();
        }
    }

	// Skyboxの描画(object3dの描画が終わった後に描画するのが基本)
    if (skybox_ && isShowSkybox_) {
        skybox_->Draw();
    }

    // 3. パーティクルの描画
    if (isShowParticle_) {
        ParticleManager::GetInstance()->Draw(
            static_cast<BlendState>(particleBlendMode_));
    }


    // 4. スプライトの描画
    // 描画設定
    SpriteCommon::GetInstance()->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode_));

    if (isShowSprite_) {
        for (size_t i = 0; i < sprites_.size(); ++i) {
            // テクスチャの差し替え（特定の要素だけ変える処理）
            if (i == 6) {
                // ※ パスを持っているならここでセット
                sprites_[i]->SetTexture(monsterBallPath_);
            }
            sprites_[i]->Draw();
        }
    }
}

void GamePlayScene::CreateObjects(const std::vector<LevelData::ObjectData>& data) {
    for (const auto& objData : data) {
        // MESHタイプの場合のみ、エンジンの3Dオブジェクトをインスタンス化
        if (objData.type == "MESH") {
            std::unique_ptr<Object3d> newObj = std::make_unique<Object3d>();
            newObj->Initialize();

            // モデルの設定 (ModelManager等で事前にロードされている必要がある、または内部でロードされる)
            newObj->SetModel(objData.fileName);

            // ローダーで変換済みのトランスフォームを適用
            newObj->SetTranslate(objData.translation);
            newObj->SetRotation(objData.rotation);
            newObj->SetScale(objData.scaling);

            // リストに追加
            object3ds_.push_back(std::move(newObj));
        }

        // 子要素が存在する場合は再帰的に処理
        if (!objData.children.empty()) {
            CreateObjects(objData.children);
        }
    }
}

void GamePlayScene::UpdateGameCamera() {

    if (Input::GetInstance()->IsKeyTriggered(DIK_F1)) {
        useDebugCamera_ = !useDebugCamera_;
        if (!useDebugCamera_) {
            camera_->SetRotate(kDefaultCameraRotate);
        }
    }

    if (!useDebugCamera_) {
        // キー入力によるカメラ操作
        if (Input::GetInstance()->IsKeyDown(DIK_LEFT)) {
            gameCameraTranslate_.x -= kMoveSpeed;
        }
        if (Input::GetInstance()->IsKeyDown(DIK_RIGHT)) {
            gameCameraTranslate_.x += kMoveSpeed;
        }
        if (Input::GetInstance()->IsKeyDown(DIK_UP)) {
            gameCameraTranslate_.y += kMoveSpeed;
        }
        if (Input::GetInstance()->IsKeyDown(DIK_DOWN)) {
            gameCameraTranslate_.y -= kMoveSpeed;
        }
        // 共通のリセットキー
        if (Input::GetInstance()->IsKeyDown(DIK_R)) {
            camera_->SetRotate(kDefaultCameraRotate);
            gameCameraTranslate_ = kDefaultCameraTranslate;
        }

        camera_->SetTranslate(gameCameraTranslate_);
    } else {
        debugCamera_.Update();
        camera_->SetRotate(debugCamera_.GetRotation());
        camera_->SetTranslate(debugCamera_.GetTranslate());
    }
    camera_->Update();
}

void GamePlayScene::UpdateImGui() {
#ifdef USE_IMGUI
    // ImGuiを使ったUI処理
    // ウィンドウの位置を設定
    ImGui::SetNextWindowPos(ImVec2(Win32Window::kClientWidth - 10.0f, 10.0f),
        ImGuiCond_Once, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(450.0f, 600.0f), ImGuiCond_Once);
    // ウィンドウの開始
    ImGui::Begin("Settings");

    if (ImGui::TreeNode("Global Settings")) {
        ImGui::Checkbox("useDebugCamera (F1)", &useDebugCamera_);
        ImGui::Checkbox("showParticle", &isShowParticle_);
        ImGui::Checkbox("showMaterial", &isShowMaterial_);
        ImGui::Checkbox("showSprite", &isShowSprite_);
        // 1.
        // 現在選択されているオブジェクトのラベルを作成（コンボボックスのプレビュー用）
        std::string currentLabel = "Object " + std::to_string(objectControlIndex_);

        // 2. コンボボックスの開始
        // "Select Object" はラベル名です。##
        // を使うことでラベルを表示せずIDとして扱うことも可能です
        if (ImGui::BeginCombo("Select Object", currentLabel.c_str())) {

            for (size_t i = 0; i < object3ds_.size(); ++i) {
                // 各項目のラベルを作成
                std::string label = "Object " + std::to_string(i);

                // 現在選択されている項目かどうかを判定
                const bool isSelected = (objectControlIndex_ == static_cast<int>(i));

                // 3. 選択肢を表示
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    // クリックされたらインデックスを更新
                    objectControlIndex_ = static_cast<int>(i);
                }

                // 4.
                // 最初から選択されている項目にスクロールを合わせる（アクセシビリティのため）
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            // コンボボックスの終了
            ImGui::EndCombo();
        }

        ImGui::Combo("BlendMode", &currentBlendMode_,
            "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");

        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::TreeNode("Light Settings")) {
        if (ImGui::TreeNode("DirectioalLight_")) {
            // LightManagerからデータを参照して操作
            Vector4& color = LightManager::GetInstance()->GetDirectionalLightData().color;
            Vector3& direction = LightManager::GetInstance()->GetDirectionalLightData().direction;
            float& intensity = LightManager::GetInstance()->GetDirectionalLightData().intensity;

            ImGui::ColorEdit4("color", &color.x);
            ImGui::DragFloat3("direction", &direction.x, 0.01f);
            ImGui::DragFloat("intensity", &intensity, 0.01f);

            // 必要に応じて正規化
            // direction = Normalize(direction);

            ImGui::TreePop();
        }
        ImGui::Separator();

        if (ImGui::TreeNode("PointLight_")) {
            PointLight& pointLight = LightManager::GetInstance()->GetPointLightData();

            ImGui::ColorEdit4("color", &pointLight.color.x);
            ImGui::DragFloat3("position", &pointLight.position.x, 0.01f);
            ImGui::DragFloat("intensity", &pointLight.intensity, 0.01f);
            ImGui::DragFloat("radius", &pointLight.radius, 0.01f);
            ImGui::DragFloat("decay", &pointLight.decay, 0.01f);

            ImGui::TreePop();
        }
        ImGui::Separator();

        if (ImGui::TreeNode("SpotLight_")) {
            SpotLight& spotLight = LightManager::GetInstance()->GetSpotLightData();

            ImGui::ColorEdit4("color", &spotLight.color.x);
            ImGui::DragFloat3("position", &spotLight.position.x, 0.01f);
            ImGui::DragFloat("distance", &spotLight.distance, 0.01f);
            ImGui::DragFloat3("direction", &spotLight.direction.x, 0.01f);
            ImGui::DragFloat("intensity", &spotLight.intensity, 0.01f);
            ImGui::DragFloat("decay", &spotLight.decay, 0.01f);
            ImGui::DragFloat("cosAngle", &spotLight.cosAngle, 0.01f);
            ImGui::DragFloat("cosFalloffStart", &spotLight.cosFalloffStart, 0.01f);

            ImGui::TreePop();
        }
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
        ImGui::Checkbox("showMaterial", &isShowMaterial_);

        // ここからループ処理
        for (size_t i = 0; i < object3ds_.size(); ++i) {
            Object3d* object3d = object3ds_[i].get(); // 現在のオブジェクトを取得

            // ノードのラベルを動的に生成
            std::string label = "Object3d " + std::to_string(i + 1);

            if (ImGui::TreeNode(label.c_str())) {

                ImGui::DragFloat3("scale", &object3d->GetTransformDebug().scale.x,
                    0.01f);
                ImGui::DragFloat3("rotate", &object3d->GetTransformDebug().rotate.x,
                    0.01f);
                ImGui::DragFloat3("translate",
                    &object3d->GetTransformDebug().translate.x, 0.01f);

                ImGui::Separator();
                bool enableLighting = object3d->GetModelDebug().IsEnableLighting();

                ImGui::ColorEdit4("color",
                    &object3d->GetModelDebug().GetColorDebug().x);
                ImGui::Checkbox("enableLighting", &enableLighting);
                object3d->GetModelDebug().SetEnableLighting(enableLighting);

                float environmentCoefficient = object3d->GetModelDebug().GetEnvironmentCoefficient();
                ImGui::DragFloat("environmentCoefficient", &environmentCoefficient, 0.01f, 0.0f, 1.0f);
                object3d->GetModelDebug().SetEnvironmentCoefficient(environmentCoefficient);

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
        ImGui::Combo("ParticleBlendMode", &particleBlendMode_,
            "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");

        ImGui::Separator();

        // 登録されている全パーティクルグループを取得して個別に設定
        std::vector<std::string> groupNames = ParticleManager::GetInstance()->GetParticleGroupNames();
        for (const auto& groupName : groupNames) {
            if (ImGui::TreeNode(groupName.c_str())) {
                ParticleEmitter* emitter = ParticleManager::GetInstance()->GetEmitter(groupName);
                if (emitter) {
                    ImGui::DragFloat3("Translate", &emitter->transform.translate.x, 0.01f);
                    ImGui::DragFloat3("Rotate", &emitter->transform.rotate.x, 0.01f);
                    ImGui::DragFloat3("Scale", &emitter->transform.scale.x, 0.01f);

                    int count = static_cast<int>(emitter->count);
                    if (ImGui::DragInt("Count", &count, 1, 1, 100)) {
                        emitter->count = static_cast<uint32_t>(count);
                    }
                    
                    ImGui::Checkbox("Enable Auto Emit", &emitter->isEmit);
                    ImGui::DragFloat("Frequency", &emitter->frequency, 0.01f, 0.01f, 10.0f);
                    
                    ImGui::Checkbox("Effect Mode (Single/Loop Effect)", &emitter->isEffectMode);
                    if (emitter->isEffectMode) {
                        ImGui::Checkbox("Loop Effect", &emitter->isLoop);
                        ImGui::Text("Playing State: %s", emitter->isPlaying ? "Playing" : "Stopped");
                    }

                    if (ImGui::TreeNode("Generate Settings")) {
                        auto& gs = emitter->generateSettings;
                        
                        // Scale
                        ImGui::Checkbox("Random Scale", &gs.isRandomScale);
                        if (gs.isRandomScale) {
                            ImGui::DragFloat3("Scale Min", &gs.scaleMin.x, 0.01f);
                            ImGui::DragFloat3("Scale Max", &gs.scaleMax.x, 0.01f);
                        } else {
                            ImGui::DragFloat3("Fixed Scale", &gs.fixedScale.x, 0.01f);
                        }
                        
                        // Rotate
                        ImGui::Checkbox("Random Rotate", &gs.isRandomRotate);
                        if (gs.isRandomRotate) {
                            ImGui::DragFloat3("Rotate Min", &gs.rotateMin.x, 0.01f);
                            ImGui::DragFloat3("Rotate Max", &gs.rotateMax.x, 0.01f);
                        } else {
                            ImGui::DragFloat3("Fixed Rotate", &gs.fixedRotate.x, 0.01f);
                        }
                        
                        // Velocity
                        ImGui::Checkbox("Random Velocity", &gs.isRandomVelocity);
                        if (gs.isRandomVelocity) {
                            ImGui::DragFloat3("Velocity Min", &gs.velocityMin.x, 0.01f);
                            ImGui::DragFloat3("Velocity Max", &gs.velocityMax.x, 0.01f);
                        } else {
                            ImGui::DragFloat3("Fixed Velocity", &gs.fixedVelocity.x, 0.01f);
                        }
                        
                        // LifeTime
                        ImGui::Checkbox("Random LifeTime", &gs.isRandomLifeTime);
                        if (gs.isRandomLifeTime) {
                            ImGui::DragFloat("LifeTime Min", &gs.lifeTimeMin, 0.01f);
                            ImGui::DragFloat("LifeTime Max", &gs.lifeTimeMax, 0.01f);
                        } else {
                            ImGui::DragFloat("Fixed LifeTime", &gs.fixedLifeTime, 0.01f);
                        }
                        
                        // Color
                        ImGui::Checkbox("Random Color", &gs.isRandomColor);
                        if (gs.isRandomColor) {
                            ImGui::ColorEdit4("Color Min", &gs.colorMin.x);
                            ImGui::ColorEdit4("Color Max", &gs.colorMax.x);
                        } else {
                            ImGui::ColorEdit4("Fixed Color", &gs.fixedColor.x);
                        }
                        
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Field Settings")) {
                        auto& fs = emitter->fieldSettings;

                        // Acceleration Field
                        ImGui::Checkbox("Acceleration Field Active", &fs.isAccelerationFieldActive);
                        if (fs.isAccelerationFieldActive) {
                            ImGui::DragFloat3("Acceleration", &fs.accelerationField.acceleration.x, 0.1f);
                            ImGui::DragFloat3("Area Min", &fs.accelerationField.area.min.x, 0.1f);
                            ImGui::DragFloat3("Area Max", &fs.accelerationField.area.max.x, 0.1f);
                        }

                        ImGui::Separator();

                        // Gravity Field
                        ImGui::Checkbox("Gravity Field Active", &fs.isGravityFieldActive);
                        if (fs.isGravityFieldActive) {
                            ImGui::DragFloat3("Gravity Vector", &fs.gravity.x, 0.1f);
                        }

                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("UV Animation Settings")) {
                        auto& uvas = emitter->uvAnimationSettings;

                        ImGui::Checkbox("UV Animation Active", &uvas.isActive);
                        if (uvas.isActive) {
                            ImGui::Checkbox("Individual UV Animation (Effect Mode)", &uvas.isIndividual);
                            ImGui::DragFloat2("Scroll Speed (U, V)", &uvas.scrollSpeed.x, 0.01f);
                            ImGui::DragFloat("Rotate Speed (rad/s)", &uvas.rotateSpeed, 0.01f);
                            ImGui::DragFloat2("Scale Speed (U, V)", &uvas.scaleSpeed.x, 0.01f);

                            if (ImGui::Button("Reset UV States")) {
                                uvas.currentTranslate = { 0.0f, 0.0f };
                                uvas.currentRotate = 0.0f;
                                uvas.currentScale = { 1.0f, 1.0f };
                            }
                        }

                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Ring Shape Settings")) {
                        auto& rs = emitter->ringSettings;

                        ImGui::Checkbox("Use Ring Shape", &rs.isRing);
                        if (rs.isRing) {
                            ImGui::DragFloat("Inner Radius", &rs.innerRadius, 0.01f, 0.0f, 20.0f);
                            ImGui::DragFloat("Start Outer Radius", &rs.startOuterRadius, 0.01f, 0.0f, 20.0f);
                            ImGui::DragFloat("Mid Outer Radius", &rs.midOuterRadius, 0.01f, 0.0f, 20.0f);
                            ImGui::DragFloat("End Outer Radius", &rs.endOuterRadius, 0.01f, 0.0f, 20.0f);
                            
                            ImGui::DragFloat("Start Angle (deg)", &rs.startAngle, 1.0f, 0.0f, 360.0f);
                            ImGui::DragFloat("End Angle (deg)", &rs.endAngle, 1.0f, 0.0f, 360.0f);
                            
                            int division = static_cast<int>(rs.division);
                            if (ImGui::DragInt("Division", &division, 1, 3, 256)) {
                                rs.division = static_cast<uint32_t>(division);
                            }

                            ImGui::Separator();
                            ImGui::Text("Shader Blend & UV");
                            ImGui::Checkbox("Swap UV (U <-> V)", &rs.isUvSwap);
                            ImGui::ColorEdit4("Inner Vertex Color", &rs.innerColor.x);
                            ImGui::ColorEdit4("Outer Vertex Color", &rs.outerColor.x);
                            
                            ImGui::DragFloat("Fade Start Alpha", &rs.fadeStartAlpha, 0.01f, 0.0f, 1.0f);
                            ImGui::DragFloat("Fade End Alpha", &rs.fadeEndAlpha, 0.01f, 0.0f, 1.0f);
                            ImGui::DragFloat("Fade Range", &rs.fadeRange, 0.01f, 0.0f, 0.5f);
                        }

                        ImGui::TreePop();
                    }

                    // 手動でEmitさせるボタン
                    std::string emitLabel = "Emit " + groupName;
                    if (ImGui::Button(emitLabel.c_str())) {
                        Vector3 pos = emitter->transform.translate; // エミッタの位置から発生
                        ParticleManager::GetInstance()->Emit(groupName, pos, emitter->count);
                    }

                    // 選択したオブジェクトの位置からEmitするボタン
                    std::string emitSelectedLabel = "Emit " + groupName + " from Selected Object";
                    if (ImGui::Button(emitSelectedLabel.c_str())) {
                        if (!object3ds_.empty()) {
                            Vector3 pos = object3ds_[objectControlIndex_]->GetTranslate();
                            ParticleManager::GetInstance()->Emit(groupName, pos, emitter->count);
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Sprite")) {

        ImGui::Checkbox("showSprite", &isShowSprite_);

        for (size_t i = 0; i < sprites_.size(); ++i) {
            std::string label = "Sprite " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str())) {
                Sprite *sprite = sprites_[i].get();

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

                ImGui::DragFloat2("uvTranslate", &uvTransformSprite.translate.x, 0.01f,
                    0.0f, 1.0f);
                ImGui::DragFloat2("uvScale", &uvTransformSprite.scale.x, 0.01f, -10.0f,
                    10.0f);
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
            AudioManager::GetInstance()->PlaySound("alarm1");
        }
        ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Skybox")) {
        ImGui::Checkbox("showSkybox", &isShowSkybox_);
        if (skybox_) {
            ImGui::DragFloat3("scale", &skybox_->GetScale().x, 0.1f);
            ImGui::DragFloat3("rotate", &skybox_->GetRotate().x, 0.01f);
            ImGui::DragFloat3("translate", &skybox_->GetTranslate().x, 0.1f);
        }
        ImGui::TreePop();
    }

    ImGui::End(); // "Settings" ウィンドウの終了

    // ヘルプウィンドウ
    // ウィンドウの位置を設定
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(370.0f, 190.0f), ImGuiCond_Once);
    ImGui::Begin("Help");
    ImGui::Text("F1: Toggle Debug Camera");
    ImGui::Text("R: Reset Camera / Object Position");
    ImGui::Separator();
    ImGui::Text("Arrow Keys: Move Camera");
    ImGui::Separator();
    ImGui::Text("WASDQE: Move Object");
    ImGui::Text("ZC: Rotate Object");
    ImGui::Separator();
    ImGui::Text("SPACE: Emit Particles");
    ImGui::Separator();
    ImGui::Text("Use the combo box to select which object to control.");
    ImGui::Text("Adjust parameters in the 'Settings' window.");
    ImGui::End();

#endif
}