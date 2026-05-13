#include "ShootingScene.h"
#include "DX12Context.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "BlendMode.h"
#include "Input.h"
#include "Win32Window.h"
#include "MathUtils.h"
#include <cmath>
#include <numbers>

using namespace MathUtils;
using namespace BlendMode;

void ShootingScene::Initialize() {
	// カメラの初期化
	camera_ = std::make_unique<Camera>();
	camera_->Initialize();
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });

	Object3dCommon::GetInstance()->SetDefaultCamera(camera_.get());

    // サイドカメラの初期化
    leftCamera_ = std::make_unique<Camera>();
    leftCamera_->Initialize();
    rightCamera_ = std::make_unique<Camera>();
    rightCamera_->Initialize();

	// テクスチャの読み込み
	TextureManager::GetInstance()->LoadTexture(crosshairPath_);

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("sphere", sphereModel_);

	// スカイボックスの初期化
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize("skybox.dds");
	skybox_->SetCamera(camera_.get());

	leftSkybox_ = std::make_unique<Skybox>();
	leftSkybox_->Initialize("skybox.dds");
	leftSkybox_->SetCamera(leftCamera_.get());

	rightSkybox_ = std::make_unique<Skybox>();
	rightSkybox_->Initialize("skybox.dds");
	rightSkybox_->SetCamera(rightCamera_.get());

	Object3dCommon::GetInstance()->SetEnvironmentMap(TextureManager::GetInstance()->GetSrvIndex("skybox.dds"));

	// --- 3Dオブジェクト生成 ---
	target_ = std::make_unique<Object3d>();
	target_->Initialize();
	target_->SetModel(sphereModel_);
    target_->SetCamera(camera_.get());

	leftTarget_ = std::make_unique<Object3d>();
	leftTarget_->Initialize();
	leftTarget_->SetModel(sphereModel_);
    leftTarget_->SetCamera(leftCamera_.get());

	rightTarget_ = std::make_unique<Object3d>();
	rightTarget_->Initialize();
	rightTarget_->SetModel(sphereModel_);
    rightTarget_->SetCamera(rightCamera_.get());

	// --- スプライト生成 ---
	crosshair_ = std::make_unique<Sprite>();
	crosshair_->Initialize(crosshairPath_);
	crosshair_->SetAnchorPoint({ 0.5f, 0.5f });
	crosshair_->SetScale({ 64.0f, 64.0f });

    // --- オフスクリーン用リソース ---
    leftRT_ = std::make_unique<RenderTexture>();
    leftRT_->Initialize(320, 180);
    rightRT_ = std::make_unique<RenderTexture>();
    rightRT_->Initialize(320, 180);

    leftSideSprite_ = std::make_unique<Sprite>();
    leftSideSprite_->Initialize(crosshairPath_);
    leftSideSprite_->SetTexture(leftRT_->GetSrvIndex());
    leftSideSprite_->SetTranslate({0.0f, 0.0f});
    leftSideSprite_->SetScale({320.0f, 180.0f});

    rightSideSprite_ = std::make_unique<Sprite>();
    rightSideSprite_->Initialize(crosshairPath_);
    rightSideSprite_->SetTexture(rightRT_->GetSrvIndex());
    rightSideSprite_->SetTranslate({ Win32Window::kClientWidth - 320.0f, 0.0f});
    rightSideSprite_->SetScale({320.0f, 180.0f});
}

void ShootingScene::Update() {
	// 入力の更新
	Input::GetInstance()->Update();

    // A/Dキーでカメラを90度ずつ回転
    const float kHalfPI = std::numbers::pi_v<float> / 2.0f; // π/2 (90度)
    if (Input::GetInstance()->IsKeyDown(DIK_A)) {
        cameraYaw_ -= kHalfPI / 100.0f;
    }
    if (Input::GetInstance()->IsKeyDown(DIK_D)) {
        cameraYaw_ += kHalfPI / 100.0f;
    }
    camera_->SetRotate({ 0.0f, cameraYaw_, 0.0f });

	// ターゲットの移動（周回 ＋ 上下に振動）
    orbitAngle_ += 0.005f;
	targetTimer_ += 0.05f;
    
    Vector3 camPos = camera_->GetTranslate();
	Vector3 pos;
    pos.x = camPos.x + std::sin(orbitAngle_) * orbitRadius_;
    pos.z = camPos.z + std::cos(orbitAngle_) * orbitRadius_;
	pos.y = targetBasePos_.y + std::sin(targetTimer_) * 4.0f;
	target_->SetTranslate(pos);

	// ヒットフィードバックの更新
	if (hitFeedbackTimer_ > 0) {
		hitFeedbackTimer_ -= 0.016f;
		target_->SetScale({ 1.5f, 1.5f, 1.5f });
	}
	else {
		target_->SetScale({ 1.0f, 1.0f, 1.0f });
	}

    // 他の視点用の的も同期
    leftTarget_->SetTranslate(target_->GetTranslate());
    leftTarget_->SetScale(target_->GetScale());
    rightTarget_->SetTranslate(target_->GetTranslate());
    rightTarget_->SetScale(target_->GetScale());

    // 更新
    target_->Update();
    leftTarget_->Update();
    rightTarget_->Update();

	// マウス座標の取得と照準の更新
	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(Win32Window::GetInstance()->GetHwnd(), &mousePos);
	crosshair_->SetTranslate({ (float)mousePos.x, (float)mousePos.y });
	crosshair_->Update();

    // サイドビュースプライトの更新（頂点バッファを毎フレーム書き込む）
    leftSideSprite_->Update();
    rightSideSprite_->Update();

    // サイドカメラの更新
    leftCamera_->SetTranslate(camera_->GetTranslate());
	leftCamera_->SetRotate({ camera_->GetRotate().x, camera_->GetRotate().y - kHalfPI, camera_->GetRotate().z });
    leftCamera_->Update();

    rightCamera_->SetTranslate(camera_->GetTranslate());
    rightCamera_->SetRotate({camera_->GetRotate().x, camera_->GetRotate().y + kHalfPI, camera_->GetRotate().z});
    rightCamera_->Update();

	// 射撃処理
	if (Input::GetInstance()->IsMouseButtonTriggered(0)) {
		// NDC座標に変換
		float x = (float)mousePos.x / Win32Window::kClientWidth * 2.0f - 1.0f;
		float y = 1.0f - (float)mousePos.y / Win32Window::kClientHeight * 2.0f;

		// ビュープロジェクション行列の逆行列を求める
		Matrix4x4 vp = camera_->GetViewProjectionMatrix();
		Matrix4x4 invVP = Inverse(vp);

		// Near平面とFar平面の座標をワールド空間に変換
		Vector3 nearPos = TransformPoint({ x, y, 0.0f }, invVP);
		Vector3 farPos = TransformPoint({ x, y, 1.0f }, invVP);
		Vector3 rayDir = Normalize(Subtract(farPos, nearPos));

		// ターゲットとの当たり判定（簡易レイ・球判定）
		Vector3 targetPos = target_->GetTranslate();
		Vector3 toTarget = Subtract(targetPos, nearPos);
		float t = Dot(toTarget, rayDir);

		if (t > 0) {
			Vector3 closestPoint = Add(nearPos, Multiply(t, rayDir));
			float distance = Length(Subtract(targetPos, closestPoint));

			// 球の半径を1.0と仮定
			if (distance < 1.0f) {
				isHit_ = true;
				hitFeedbackTimer_ = 0.2f; // ヒット時に一瞬大きくする
			}
		}
	}

	camera_->Update();
	skybox_->Update();
    leftSkybox_->Update();
    rightSkybox_->Update();
}

void ShootingScene::DrawOffscreen() {
    auto* cmd = DX12Context::GetInstance()->GetCommandList();

    // 左側カメラの描画
    leftRT_->PreDraw(cmd);
    // 3Dモデル用の設定をリセット（スカイボックス描画後に RS が変わるため）
    Object3dCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));
    if (isShowMaterial_) leftTarget_->Draw();
    if (isShowSkybox_) leftSkybox_->Draw();
    leftRT_->PostDraw(cmd);

    // 右側カメラの描画
    rightRT_->PreDraw(cmd);
    // 3Dモデル用の設定をリセット
    Object3dCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));
    if (isShowMaterial_) rightTarget_->Draw();
    if (isShowSkybox_) rightSkybox_->Draw();
    rightRT_->PostDraw(cmd);
}

void ShootingScene::Draw() {
    // 描画設定
    Object3dCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));

    // メインカメラで描画
    if (isShowMaterial_) target_->Draw();
    if (isShowSkybox_) skybox_->Draw();

	// 4. スプライトの描画
	SpriteCommon::GetInstance()->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode_));

	if (isShowSprite_) {
		if (crosshair_) {
			crosshair_->Draw();
		}
        // サイドビューを表示
        leftSideSprite_->Draw();
        rightSideSprite_->Draw();
	}
}

void ShootingScene::Finalize() {
	// Object3dCommonの参照をクリア
	Object3dCommon::GetInstance()->SetDefaultCamera(nullptr);
}
