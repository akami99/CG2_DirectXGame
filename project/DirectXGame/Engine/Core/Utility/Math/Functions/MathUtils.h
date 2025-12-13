#pragma once

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "../MathTypes.h" // Vector3, Matrix4x4などの構造体定義を含む

namespace MathUtils {

// 基本的なベクトル演算

/// <summary>
/// ベクトルの加算
/// </summary>
/// <param name="v1">ベクトル1</param>
/// <param name="v2">ベクトル2</param>
/// <returns>ベクトルの和</returns>
Vector3 Add(const Vector3 &v1, const Vector3 &v2);

/// <summary>
/// ベクトルの減算
/// </summary>
/// <param name="v1">引かれるベクトル</param>
/// <param name="v2">引くベクトル</param>
/// <returns>ベクトルの差</returns>
Vector3 Subtract(const Vector3 &v1, const Vector3 &v2);

/// <summary>
/// ベクトルの積（要素ごとの積）
/// </summary>
/// <param name="v1">ベクトル1</param>
/// <param name="v2">ベクトル2</param>
/// <returns>要素ごとの積</returns>
Vector3 Multiply(const Vector3 &v1, const Vector3 &v2);

/// <summary>
/// 内積
/// </summary>
/// <param name="v1">ベクトル1</param>
/// <param name="v2">ベクトル2</param>
/// <returns>内積</returns>
float Dot(const Vector3 &v1, const Vector3 &v2);

/// <summary>
/// クロス積
/// </summary>
/// <param name="v1">ベクトル1</param>
/// <param name="v2">ベクトル2</param>
/// <returns>クロス積</returns>
Vector3 Cross(const Vector3 &v1, const Vector3 &v2);

/// <summary>
/// 長さ（ノルム）
/// </summary>
/// <param name="v">ベクトル</param>
/// <returns>長さ（ノルム）</returns>
float Length(const Vector3 &v);

/// <summary>
/// 距離の二乗を計算する関数（ルート計算を省くことで高速化）
/// </summary>
/// <param name="v">ベクトル</param>
/// <returns>距離の二乗</returns>
float LengthSq(const Vector3 &v);

/// <summary>
/// 正規化
/// </summary>
/// <param name="v">ベクトル</param>
/// <returns>正規化されたベクトル</returns>
Vector3 Normalize(const Vector3 &v);

/// <summary>
/// 垂直なベクトルを求める関数
/// </summary>
/// <param name="vector">基となるベクトル</param>
/// <returns>垂直なベクトル</returns>
Vector3 Perpendicular(const Vector3 &vector);

/// <summary>
/// 反射ベクトルを求める関数
/// </summary>
/// <param name="input">入射ベクトル</param>
/// <param name="normal">法線(正規化していないもの)</param>
/// <returns>反射ベクトル</returns>
Vector3 Reflect(const Vector3 &input, const Vector3 &normal);

/// <summary>
/// Vector3をAABB内にクランプし、AABB上の最も近い点を求める
/// </summary>
/// <param name="aabb">対象のAABB</param>
/// <param name="point">対象の点</param>
/// <returns>AABB上の最も近い点</returns>
Vector3 GetClosestPointOnAABB(const AABB &aabb, const Vector3 &point);

/// <summary>
/// 球との衝突判定を行う関数
/// </summary>
/// <param name="sphere">判定対象のSphere</param>
/// <param name="plane">判定対象平面</param>
/// <returns>衝突判定</returns>
bool IsCollision(const Sphere &sphere, const Plane &plane);

/// <summary>
/// AABBとVector3（点）の衝突判定
/// </summary>
/// <param name="aabb">判定対象のAABB</param>
/// <param name="point">判定対象の点</param>
/// <returns></returns>
bool IsCollision(const AABB &aabb, const Vector3 &point);

/// <summary>
/// AABBとSphereの衝突判定
/// </summary>
/// <param name="aabb">判定対象のAABB</param>
/// <param name="sphere">判定対象のSphere</param>
/// <returns>衝突判定</returns>
bool IsCollision(const AABB &aabb, const Sphere &sphere);

/// <summary>
/// AABBとAABBの衝突判定関数
/// </summary>
/// <param name="aabb1">判定対象のAABB</param>
/// <param name="aabb2">判定対象のAABB</param>
/// <returns>衝突判定</returns>
bool IsCollision(const AABB &aabb1, const AABB &aabb2);

/// <summary>
/// ベクトルを法線方向に投影する関数
/// </summary>
/// <param name="vector">ベクトル</param>
/// <param name="normal">法線(正規化していないもの)</param>
/// <returns>投影ベクトル</returns>
Vector3 Project(const Vector3 &vector, const Vector3 &normal);

// 基本的な行列演算

/// <summary>
/// 4x4行列の加算
/// </summary>
/// <param name="v1">行列1</param>
/// <param name="v2">行列2</param>
/// <returns>行列の和</returns>
Matrix4x4 Add(const Matrix4x4 &m1, const Matrix4x4 &m2);

/// <summary>
/// 4x4行列の減算
/// </summary>
/// <param name="v1">引かれる4x4行列</param>
/// <param name="v2">引く4x4行列</param>
/// <returns>4x4行列の差</returns>
Matrix4x4 Subtract(const Matrix4x4 &m1, const Matrix4x4 &m2);

/// <summary>
/// 4x4行列の積
/// </summary>
/// <param name="m1">掛ける行列1</param>
/// <param name="m2">掛ける行列2</param>
/// <returns>行列の積</returns>
Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2);

/// <summary>
/// 逆行列
/// </summary>
/// <param name="m">逆行列に変換する行列</param>
/// <returns>逆行列</returns>
Matrix4x4 Inverse(const Matrix4x4 &m);

// スカラー倍

/// <summary>
/// スカラーとベクトルの積
/// </summary>
/// <param name="scalar">掛けるスカラー</param>
/// <param name="vector">掛けるベクトル</param>
/// <returns>ベクトルの積</returns>
Vector3 Multiply(const float &scalar, const Vector3 &vector);

/// <summary>
/// スカラーと4x4行列の積
/// </summary>
/// <param name="scalar">掛けるスカラー</param>
/// <param name="matrix">掛ける4x4行列</param>
/// <returns>4x4行列の積</returns>
Matrix4x4 Multiply(const float &scalar,
                   const Matrix4x4 &matrix); // ここはMatrix4x4にしておきました

// 変換

/// <summary>
/// 座標変換
/// </summary>
/// <param name="vector">変換したいベクトル</param>
/// <param name="matrix">変換させる行列</param>
/// <returns>変換させた座標</returns>
Vector3 TransformPoint(const Vector3 &vector, const Matrix4x4 &matrix);

/// <summary>
/// ベクトル変換
/// </summary>
/// <param name="vector">変換したいベクトル</param>
/// <param name="matrix">変換させる行列</param>
/// <returns>変換させたベクトル</returns>
Vector3 TransformVector(const Vector3 &vector, const Matrix4x4 &matrix);

/// <summary>
/// 球面をデカルト座標に変換
/// </summary>
/// <param name="radius">半径</param>
/// <param name="lat">緯度</param>
/// <param name="lon">経度</param>
/// <returns>デカルト座標</returns>
Vector3 SphericalToCartesian(float radius, float lat, float lon);

// 演算子オーバーロード（インライン関数としてヘッダーに定義）
// 演算子の実装は、上記の関数を呼び出すだけなので、通常ヘッダーファイルに直接記述します。
inline Vector3 operator+(const Vector3 &v1, const Vector3 &v2) {
  return Add(v1, v2);
}
inline Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
  return Subtract(v1, v2);
}
inline Vector3 operator*(float s, const Vector3 &v) { return Multiply(s, v); }
inline Vector3 operator*(const Vector3 &v, float s) { return s * v; }
inline Vector3 operator*(const Vector3 &v1, const Vector3 &v2) {
  return Multiply(v1, v2);
}
inline Vector3 operator/(const Vector3 &v, float s) {
  return {Multiply(1.0f / s, v)};
}
inline Vector3 operator+=(Vector3 &v1, const Vector3 &v2) {
  v1 = Add(v1, v2);
  return v1;
}
inline Vector3 operator-=(Vector3 &v1, const Vector3 &v2) {
  v1 = Subtract(v1, v2);
  return v1;
}
inline Vector3 operator*=(Vector3 &v, const float &s) {
  v = Multiply(s, v);
  return v;
}
inline Vector3 operator*=(Vector3 &v1, const Vector3 &v2) {
  v1 = Multiply(v1, v2);
  return v1;
}
inline Matrix4x4 operator+(const Matrix4x4 &m1, const Matrix4x4 &m2) {
  return Add(m1, m2);
}
inline Matrix4x4 operator-(const Matrix4x4 &m1, const Matrix4x4 &m2) {
  return Subtract(m1, m2);
}
inline Matrix4x4 operator*(const Matrix4x4 &m1, const Matrix4x4 &m2) {
  return Multiply(m1, m2);
}
inline Matrix4x4 operator+=(Matrix4x4 &m1, const Matrix4x4 &m2) {
  m1 = Add(m1, m2);
  return m1;
}
inline Matrix4x4 operator-=(Matrix4x4 &m1, const Matrix4x4 &m2) {
  m1 = Subtract(m1, m2);
  return m1;
}
inline Matrix4x4 operator*=(Matrix4x4 &m1, const Matrix4x4 &m2) {
  m1 = Multiply(m1, m2);
  return m1;
}

} // namespace MathUtils

#endif // MATH_UTILS_H