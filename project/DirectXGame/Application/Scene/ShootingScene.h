#pragma once
#include "BaseScene.h"
#include <memory>
#include <vector>
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"
#include "Skybox.h"

#include "RenderTexture.h"

class ShootingScene : public BaseScene {
public:
    void Initialize() override;
    void Update() override;
    void DrawOffscreen() override;
    void Draw() override;
    void Finalize() override;

private:
    // シーン描画のヘルパー
    void DrawScene(Camera* camera);

private:
    // カメラ
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Camera> leftCamera_;
    std::unique_ptr<Camera> rightCamera_;
    
    // ゲームオブジェクト
    std::unique_ptr<Skybox> skybox_;
    std::unique_ptr<Skybox> leftSkybox_;
    std::unique_ptr<Skybox> rightSkybox_;
    
    // ターゲット（敵）
    std::unique_ptr<Object3d> target_;
    std::unique_ptr<Object3d> leftTarget_;
    std::unique_ptr<Object3d> rightTarget_;
    
    // 照準（スプライト）
    std::unique_ptr<Sprite> crosshair_;

    // オフスクリーン用
    std::unique_ptr<RenderTexture> leftRT_;
    std::unique_ptr<RenderTexture> rightRT_;
    std::unique_ptr<Sprite> leftSideSprite_;
    std::unique_ptr<Sprite> rightSideSprite_;

    // ゲームロジック用変数
    float targetTimer_ = 0.0f;
    float orbitAngle_ = 0.0f;
    const float orbitRadius_ = 15.0f;
    Vector3 targetBasePos_ = {0.0f, 0.0f, 10.0f};
    bool isHit_ = false;
    float hitFeedbackTimer_ = 0.0f;
    float cameraYaw_ = 0.0f; // カメラのY軸回転（ラジアン、90度ずつ変化）

	// パーティクル設定など
    int currentBlendMode_ = 1;  // NormalBlend
    bool isShowMaterial_ = true;
    bool isShowSprite_ = true;
    bool isShowSkybox_ = true;

    // パス定数
    // 3Dモデルのファイルパス
    const std::string sphereModel_ = "sphere.obj";
    // テクスチャファイルパスを保持
    const std::string crosshairPath_ = "crosshair.png";
};
