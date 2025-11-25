#pragma once
#include "../MathTypes.h"

namespace MathGenerators {

	/// <summary>
	/// 単位行列の作成
	/// </summary>
	/// <returns>単位行列</returns>
	Matrix4x4 MakeIdentity4x4();

	// 回転行列の生成

	/// <summary>
	/// X軸回転行列
	/// </summary>
	/// <param name="angle">回転角</param>
	/// <returns>X軸回転行列</returns>
	Matrix4x4 MakeRotateXMatrix(float angle);

	/// <summary>
	/// Y軸回転行列
	/// </summary>
	/// <param name="angle">回転角</param>
	/// <returns>Y軸回転行列</returns>
	Matrix4x4 MakeRotateYMatrix(float angle);

	/// <summary>
	/// Z軸回転行列
	/// </summary>
	/// <param name="angle">回転角</param>
	/// <returns>Z軸回転行列</returns>
	Matrix4x4 MakeRotateZMatrix(float angle);

	// アフィン変換行列の生成

	/// <summary>
	/// 平行移動行列
	/// </summary>
	/// <param name="translate">座標</param>
	/// <returns>平行移動行列</returns>
	Matrix4x4 MakeTranslationMatrix(const Vector3& translate);

	/// <summary>
	/// 拡大縮小行列
	/// </summary>
	/// <param name="scale">大きさ</param>
	/// <returns>拡大縮小行列</returns>
	Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	/// <summary>
	/// 3次元アフィン変換行列
	/// </summary>
	/// <param name="scale">大きさ</param>
	/// <param name="rotate">回転角</param>
	/// <param name="translate">座標</param>
	/// <returns>3次元アフィン変換行列</returns>
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	/// <summary>
	/// 3次元アフィン変換行列(回転行列を使用)
	/// </summary>
	/// <param name="scale">大きさ</param>
	/// <param name="rotationMatrix">回転行列</param>
	/// <param name="translate">座標</param>
	/// <returns>3次元アフィン変換行列</returns>
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Matrix4x4& rotationMatrix, const Vector3& translate);

	// 射影行列の生成

	/// <summary>
	/// 透視投影行列
	/// </summary>
	/// <param name="fovY">視野角Y</param>
	/// <param name="aspectRatio">画面比率</param>
	/// <param name="nearClip">近くの描画範囲</param>
	/// <param name="farClip">遠くの描画範囲</param>
	/// <returns>透視投影行列</returns>
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

	/// <summary>
	/// 正射影行列
	/// </summary>
	/// <param name="left">左端</param>
	/// <param name="top">上端</param>
	/// <param name="right">右端</param>
	/// <param name="bottom">下端</param>
	/// <param name="nearClip">近くの描画範囲</param>
	/// <param name="farClip">遠くの描画範囲</param>
	/// <returns>正射影行列</returns>
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

	// ビューポート行列の生成

	/// <summary>
	/// ビューポート変換行列
	/// </summary>
	/// <param name="left">左端</param>
	/// <param name="top">上端</param>
	/// <param name="width">横幅</param>
	/// <param name="height">立幅</param>
	/// <param name="minDepth">最小深度</param>
	/// <param name="maxDepth">最大深度</param>
	/// <returns>ビューポート変換行列</returns>
	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

} // namespace MathGenerators