#pragma once
#include "BaseScene.h"
#include <vector>
#include <memory>
#include "MathTypes.h"

// 必要なクラスをインクルード or 前方宣言
#include "DebugCamera.h"
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"

class GamePlayScene : public BaseScene {
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

private:
    // ゲームカメラの更新
    void UpdateGameCamera();

    // ImGui操作の更新
    void UpdateImGui();

private:
    // --- ここから下は MyGame.h から移動してきたゲーム固有変数 ---

    // カメラ
    std::unique_ptr<Camera> camera_;

    DebugCamera debugCamera_;
    bool useDebugCamera_ = false;
    Vector3 gameCameraRotate_{};
    Vector3 gameCameraTranslate_{};

    // ゲームオブジェクト
    std::vector < std::unique_ptr<Object3d>> object3ds_;
    std::vector<std::unique_ptr<Sprite>> sprites_;

    // パーティクル設定など
    Vector3 particleRotation_{};
    bool isShowParticle_ = false;
    bool isUpdateParticle_ = true;
    bool useBillboard_ = true;
    int objectControlIndex_ = 0;
    int currentBlendMode_ = 1;  // NormalBlend
    int particleBlendMode_ = 2; // AddBlend
    bool isShowMaterial_ = true;
    bool isShowSprite_ = false;

    // パス定数
    // 3Dモデルのファイルパス
    const std::string planeModel_ = "plane.obj";
    const std::string planeGltfModel_ = "plane.gltf";
    const std::string teapotModel_ = "teapot.obj";
    const std::string terrainModel_ = "terrain.obj";
    const std::string sphereModel_ = "sphere.obj";
    // テクスチャファイルパスを保持
    const std::string uvCheckerPath_ = "uvChecker.png";
    const std::string monsterBallPath_ = "monsterBall.png";
    const std::string grassPath_ = "grass.png";
    const std::string particleTexturePath_ = "circle.png";
    // パーティクルグループの作成
    const std::string particleGroupName_ = "TestGroup";
};