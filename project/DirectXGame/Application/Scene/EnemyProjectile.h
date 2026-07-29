#pragma once
#include "Object3d.h"
#include <memory>

class EnemyProjectile {
public:
    void Initialize(const Vector3& position, const Vector3& velocity, bool isExplosive = false);
	// メインビュー用の更新
    void Update();
	// 指定したビュー用の更新
    void Update(uint32_t viewIndex, Camera* camera);
    void Draw(uint32_t viewIndex = 0);

    const Vector3& GetPosition() const { return position_; }
    const Vector3& GetVelocity() const { return velocity_; }
    bool IsDead() const { return isDead_; }
    void Kill() { isDead_ = true; }
    bool IsExplosive() const { return isExplosive_; }

    float GetRadius() const { return radius_; }

#ifdef USE_IMGUI
    // ImGui用のゲッター
    Object3d &GetObjectDebug() const { return *object_; }
#endif // USE_IMGUI

private:
    std::unique_ptr<Object3d> object_;
    Vector3 position_;
    Vector3 velocity_;
    float radius_ = 0.5f;
    bool isDead_ = false;
    std::unique_ptr<Model> customModel_;
    bool isExplosive_ = false;
};
