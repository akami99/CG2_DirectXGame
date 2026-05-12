#pragma once
#include "BaseScene.h"
#include <memory>
#include <vector>
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"
#include "Skybox.h"

class ShootingScene : public BaseScene {
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

private:
    // カメラ
    std::unique_ptr<Camera> camera_;
    
    // ゲームオブジェクト
    std::unique_ptr<Skybox> skybox_;
    /*std::vector < std::unique_ptr<Object3d>> object3ds_;
    std::vector<std::unique_ptr<Sprite>> sprites_;*/
    
    // ターゲット（敵）
    std::unique_ptr<Object3d> target_;
    // 照準（スプライト）
    std::unique_ptr<Sprite> crosshair_;

    // ゲームロジック用変数
    float targetTimer_ = 0.0f;
    Vector3 targetBasePos_ = {0.0f, 0.0f, 10.0f};
    bool isHit_ = false;
    float hitFeedbackTimer_ = 0.0f;

	// パーティクル設定など
    int currentBlendMode_ = 1;  // NormalBlend
    bool isShowMaterial_ = true;
    bool isShowSprite_ = true;
    bool isShowSkybox_ = true;

    // パス定数
    // 3Dモデルのファイルパス
    const std::string sphereModel_ = "sphere.obj";
    // テクスチャファイルパスを保持
    const std::string monsterBallPath_ = "monsterBall.png";
};
