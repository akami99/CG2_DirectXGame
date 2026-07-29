#include "EnemyProjectile.h"
#include "ModelManager.h"
#include "Model.h"

void EnemyProjectile::Initialize(const Vector3& position, const Vector3& velocity, bool isExplosive) {
    position_ = position;
    velocity_ = velocity;
    isExplosive_ = isExplosive;
    
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    
    if (isExplosive_) {
        // 爆発弾用に個別のモデルインスタンスを作成
        customModel_ = std::make_unique<Model>();
        customModel_->Initialize("Resources/Assets/Models/bullet", "bullet.obj");
        customModel_->SetColor({1.0f, 0.8f, 0.0f, 1.0f}); // 黄色
        object_->SetModel(customModel_.get());
    } else {
        object_->SetModel("bullet.obj");
    }
    
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
