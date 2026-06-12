#include "EnemyProjectile.h"
#include "ModelManager.h"

void EnemyProjectile::Initialize(const Vector3& position, const Vector3& velocity) {
    position_ = position;
    velocity_ = velocity;
    
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    object_->SetModel("bullet.obj");
    object_->SetTranslate(position_);
    object_->SetScale({radius_, radius_, radius_});
}

void EnemyProjectile::Update() {
    position_.x += velocity_.x;
    position_.y += velocity_.y;
    position_.z += velocity_.z;
    
    object_->SetTranslate(position_);
    // デフォルトのUpdate(全ビューをcamera_で更新)
    object_->Update();
}

void EnemyProjectile::Update(uint32_t viewIndex, Camera* camera) {
    // 位置などはUpdate()で更新済みなので、行列だけ再計算
    object_->Update(viewIndex, camera);
}

void EnemyProjectile::Draw(uint32_t viewIndex) {
    object_->Draw(viewIndex);
}
