#include "EnemyProjectile.h"
#include "ModelManager.h"

void EnemyProjectile::Initialize(const Vector3& position, const Vector3& velocity) {
    position_ = position;
    velocity_ = velocity;
    
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    object_->SetModel("sphere.obj");
    object_->SetTranslate(position_);
    object_->SetScale({radius_, radius_, radius_});
}

void EnemyProjectile::Update() {
    position_.x += velocity_.x;
    position_.y += velocity_.y;
    position_.z += velocity_.z;
    
    object_->SetTranslate(position_);
    object_->Update();
}

void EnemyProjectile::Draw() {
    object_->Draw();
}
