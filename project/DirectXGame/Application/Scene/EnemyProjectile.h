#pragma once
#include "Object3d.h"
#include <memory>

class EnemyProjectile {
public:
    void Initialize(const Vector3& position, const Vector3& velocity);
    void Update();
    void Draw();

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
