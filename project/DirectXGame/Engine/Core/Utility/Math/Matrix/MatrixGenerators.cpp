#include "../Matrix/MatrixGenerators.h"
#include "../Functions/MathUtils.h"

#include <cmath>

using namespace MathUtils;

namespace MathGenerators {

// 単位行列の作成
Matrix4x4 MakeIdentity4x4() {
  Matrix4x4 result;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        result.m[i][j] = 1.0f;
      } else {
        result.m[i][j] = 0.0f;
      }
    }
  }
  return result;
}

// 回転行列の生成

// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float angle) {
  Matrix4x4 result = {};
  result.m[0][0] = 1.0f;
  result.m[3][3] = 1.0f;
  result.m[1][1] = std::cos(angle);
  result.m[1][2] = std::sin(angle);
  result.m[2][1] = -std::sin(angle);
  result.m[2][2] = std::cos(angle);
  return result;
}

// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float angle) {
  Matrix4x4 result = {};
  result.m[1][1] = 1.0f;
  result.m[3][3] = 1.0f;
  result.m[0][0] = std::cos(angle);
  result.m[0][2] = -std::sin(angle);
  result.m[2][0] = std::sin(angle);
  result.m[2][2] = std::cos(angle);
  return result;
}

// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float angle) {
  Matrix4x4 result = {};
  result.m[2][2] = 1.0f;
  result.m[3][3] = 1.0f;
  result.m[0][0] = std::cos(angle);
  result.m[0][1] = std::sin(angle);
  result.m[1][0] = -std::sin(angle);
  result.m[1][1] = std::cos(angle);
  return result;
}

// XYZ回転行列
Matrix4x4 MakeRotateXYZMatrix(const Vector3 &rotate) {
  // 回転行列の作成と結合
  Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
  Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
  Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);

  // X,Y,Z軸の回転をまとめる
  Matrix4x4 rotateMatrix = rotateX * (rotateY * rotateZ);

  return rotateMatrix;
}

// アフィン変換行列の生成

// 平行移動行列
Matrix4x4 MakeTranslationMatrix(const Vector3 &translate) {
  Matrix4x4 result = MakeIdentity4x4();
  result.m[3][0] = translate.x;
  result.m[3][1] = translate.y;
  result.m[3][2] = translate.z;
  return result;
}

// 拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3 &scale) {
  Matrix4x4 result = MakeIdentity4x4();
  result.m[0][0] = scale.x;
  result.m[1][1] = scale.y;
  result.m[2][2] = scale.z;
  return result;
}

// 3次元アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                           const Vector3 &translate) {
  // 各変換に対応する行列を作成
  Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

  // 回転行列の作成と結合
  Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
  Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
  Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);

  // X,Y,Z軸の回転をまとめる
  Matrix4x4 rotateMatrix = rotateX * (rotateY * rotateZ);

  Matrix4x4 translateMatrix = MakeTranslationMatrix(translate);

  Matrix4x4 result = scaleMatrix * rotateMatrix;
  result *= translateMatrix;

  return result;
}

// 3次元アフィン変換行列(回転行列を使用)
Matrix4x4 MakeAffineMatrix(const Vector3 &scale,
                           const Matrix4x4 &rotationMatrix,
                           const Vector3 &translate) {
  // スケールを適用
  Matrix4x4 result = MakeScaleMatrix(scale);

  // 回転行列を乗算 (スケール後に適用)
  result *= rotationMatrix;

  // 平行移動を適用
  result *= MakeTranslationMatrix(translate);

  return result;
}

// 射影行列の生成

// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {
  Matrix4x4 result = {};
  result.m[0][0] = 1.0f / (aspectRatio * std::tan(fovY / 2.0f));
  result.m[1][1] = 1.0f / std::tan(fovY / 2.0f);
  result.m[2][2] = farClip / (farClip - nearClip);
  result.m[2][3] = 1.0f;
  result.m[3][2] = -(farClip * nearClip) / (farClip - nearClip);
  return result;
}

// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip) {
  Matrix4x4 result{};
  result.m[0][0] = 2.0f / (right - left);
  result.m[1][1] = 2.0f / (top - bottom);
  result.m[2][2] = 1.0f / (farClip - nearClip);
  result.m[3][0] = (left + right) / (left - right);
  result.m[3][1] = (top + bottom) / (bottom - top);
  result.m[3][2] = nearClip / (nearClip - farClip);
  result.m[3][3] = 1.0f;
  return result;
}

// ビューポート行列の生成

// ビューポート変換行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height,
                             float minDepth, float maxDepth) {
  Matrix4x4 result{};
  result.m[0][0] = width / 2.0f;
  result.m[1][1] = -(height / 2.0f);
  result.m[2][2] = maxDepth - minDepth;
  result.m[3][0] = left + (width / 2.0f);
  result.m[3][1] = top + (height / 2.0f);
  result.m[3][2] = minDepth;
  result.m[3][3] = 1.0f;
  return result;
}

} // namespace MathGenerators