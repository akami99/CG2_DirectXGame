#include "Camera.h"
#include "API/DX12Context.h"

#include "Math/Functions/MathUtils.h"
#include "Math/Matrix/MatrixGenerators.h"

#include <assert.h>

using namespace MathUtils;
using namespace MathGenerators;

Camera::Camera()
    : transform_(
          {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}}),
      fovY_(0.45f), aspectRatio_(float(Win32Window::kClientWidth) /
                                 float(Win32Window::kClientHeight)),
      nearClip_(0.1f), farClip_(100.0f),
      worldMatrix_(MakeAffineMatrix(transform_.scale, transform_.rotate,
                                    transform_.translate)),
    viewMatrix_(Inverse(worldMatrix_)),
    projectionMatrix_(MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_)),
    viewProjectionMatrix_(viewMatrix_ *projectionMatrix_)
{}

// 更新処理
void Camera::Update() {
  // transformからworldMatrixを作る
  worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                  transform_.translate);
  // worldMatrixからViewMatrixを作る
  viewMatrix_ = Inverse(worldMatrix_);

  // ProjectionMatrixを作って透視投影行列を書き込む
  projectionMatrix_ =
      MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);

  // 合成しておく
  viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
}