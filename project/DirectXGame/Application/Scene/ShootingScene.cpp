#include "ShootingScene.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "BlendMode.h"
#include "Input.h"
#include "Win32Window.h"
#include "MathUtils.h"
#include <cmath>

using namespace MathUtils;
using namespace BlendMode;

void ShootingScene::Initialize() {
	// カメラの初期化
	camera_ = std::make_unique<Camera>();
	camera_->Initialize();
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });

	Object3dCommon::GetInstance()->SetDefaultCamera(camera_.get());

	// テクスチャの読み込み
	TextureManager::GetInstance()->LoadTexture("monsterBall.png");

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("sphere", "sphere.obj");

	// スカイボックスの初期化
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize("skybox.dds");
	skybox_->SetCamera(camera_.get());

	Object3dCommon::GetInstance()->SetEnvironmentMap(TextureManager::GetInstance()->GetSrvIndex("skybox.dds"));

	// --- 3Dオブジェクト生成 ---
	// 的の設定
	std::unique_ptr <Object3d> target = std::make_unique<Object3d>();
	// 的の初期化
	target->Initialize();
	// 的モデルの設定
	target->SetModel(sphereModel_);
	// 的の位置を設定
	target->SetTranslate(targetBasePos_);
	target_ = std::move(target);


	// --- スプライト生成 ---
	std::unique_ptr<Sprite> crosshair = std::make_unique<Sprite>();
	crosshair->Initialize(monsterBallPath_);
	crosshair->SetAnchorPoint({ 0.5f, 0.5f });
	crosshair->SetScale({ 64.0f, 64.0f });
	crosshair_ = std::move(crosshair);

}

void ShootingScene::Update() {
	// 入力の更新
	Input::GetInstance()->Update();

	// ターゲットの移動（上下に振動）
	targetTimer_ += 0.05f;
	Vector3 pos = targetBasePos_;
	pos.y += std::sin(targetTimer_) * 4.0f;
	target_->SetTranslate(pos);

	// ヒットフィードバックの更新
	if (hitFeedbackTimer_ > 0) {
		hitFeedbackTimer_ -= 0.016f;
		target_->SetScale({ 1.5f, 1.5f, 1.5f });
	}
	else {
		target_->SetScale({ 1.0f, 1.0f, 1.0f });
	}
	target_->Update();

	// マウス座標の取得と照準の更新
	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(Win32Window::GetInstance()->GetHwnd(), &mousePos);
	crosshair_->SetTranslate({ (float)mousePos.x, (float)mousePos.y });
	crosshair_->Update();

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
}

void ShootingScene::Draw() {
	// 描画設定
	Object3dCommon::GetInstance()->SetCommonDrawSettings(
		static_cast<BlendState>(currentBlendMode_));


	// 2. 3Dオブジェクトの描画

	if (isShowMaterial_) {
		// 的を描画
		if (target_) {
			target_->Draw();
		}
	}

	// Skyboxの描画(object3dの描画が終わった後に描画するのが基本)
	if (skybox_ && isShowSkybox_) {
		skybox_->Draw();
	}

	// 4. スプライトの描画
	// 描画設定
	SpriteCommon::GetInstance()->SetCommonDrawSettings(
		static_cast<BlendState>(currentBlendMode_));

	if (isShowSprite_) {
		// 照準を描画
		if (crosshair_) {
			crosshair_->Draw();
		}
	}
}

void ShootingScene::Finalize() {
	// Object3dCommonの参照をクリア
	Object3dCommon::GetInstance()->SetDefaultCamera(nullptr);
	// Object3dを削除
	//object3ds_.clear();

	// スプライトを削除
	//sprites_.clear();
}
