#pragma once
#include "EngineMath.h"
#include "MatrixGenerators.h"
#include "KeyboardManager.h"

/// <summary>
/// デバッグカメラ
/// </summary>
class DebugCamera {
private:
	// X,Y,Z周りのローカル回転角
	Vector3 rotation_ = { 0, 0, 0 };
	// ローカル座標
	Vector3 translation_ = { 0, 0, -50 };
	// 累積回転行列
	Matrix4x4 matRot_;
	// カメラのワールド行列
	Matrix4x4 worldMatrix_;
	// ビュー行列
	Matrix4x4 viewMatrix_;
	// カメラがオービットモードかどうか
	bool isOrbitMode_ = false;
	// オービットカメラのオイラー角 (ピボット回転用)
	Vector3 cameraEulerRotation_;
	// オービットカメラが注視するターゲット座標
	Vector3 target_;
	// オービットカメラのターゲットからの距離
	float orbitDistance_;

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	DebugCamera();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update(KeyboardManager& keyboardManager);

	// getter
	const Matrix4x4& GetViewMatrix() const {
		return viewMatrix_;
	}
	/*const Matrix4x4& GetViewProjectionMatrix() const {
		return viewProjectionMatrix_;
	}
	const Matrix4x4& GetViewportMatrix() const {
		return viewportMatrix_;
	}*/
};

