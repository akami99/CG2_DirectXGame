#pragma once
#include "Object3d.h"
#include <memory>

class EnemyProjectile {
public:
    void Initialize(const Vector3& position, const Vector3& velocity);
	// メインビュー用の更新
    void Update();
	// 指定したビュー用の更新
    void Update(uint32_t viewIndex, Camera* camera);
    void Draw(uint32_t viewIndex = 0);

    const Vector3& GetPosition() const { return position_; }
    bool IsDead() const { return isDead_; }
    void Kill() { isDead_ = true; }

    float GetRadius() const { return radius_; }

private:
    std::unique_ptr<Object3d> object_;
    Vector3 position_;
    Vector3 velocity_;
    float radius_ = 0.5f;
    bool isDead_ = false;
};
