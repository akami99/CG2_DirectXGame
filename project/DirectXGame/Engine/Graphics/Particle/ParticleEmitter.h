#pragma once

#include "Types/GraphicsTypes.h"
#include "Types/ParticleTypes.h"
#include "ParticleShape.h"

#include <string>
#include <numbers>
#include <memory>

struct ParticleGenerateSettings {
	bool isRandomScale = true;
	Vector3 scaleMin = { 0.05f, 0.4f, 1.0f };
	Vector3 scaleMax = { 0.05f, 1.5f, 1.0f };
	Vector3 fixedScale = { 1.0f, 1.0f, 1.0f };

	bool isRandomRotate = true;
	Vector3 rotateMin = { 0.0f, 0.0f, -std::numbers::pi_v<float> };
	Vector3 rotateMax = { 0.0f, 0.0f, std::numbers::pi_v<float> };
	Vector3 fixedRotate = { 0.0f, 0.0f, 0.0f };

	bool isRandomVelocity = false;
	Vector3 velocityMin = { -1.0f, -1.0f, -1.0f };
	Vector3 velocityMax = { 1.0f, 1.0f, 1.0f };
	Vector3 fixedVelocity = { 0.0f, 0.0f, 0.0f };

	bool isRandomLifeTime = false;
	float lifeTimeMin = 1.0f;
	float lifeTimeMax = 3.0f;
	float fixedLifeTime = 1.0f;

	bool isRandomColor = true;
	Vector4 colorMin = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4 colorMax = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector4 fixedColor = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct ParticleFieldSettings {
	bool isAccelerationFieldActive = false;
	AccelerationField accelerationField = { {15.0f, 0.0f, 0.0f}, {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}} };

	bool isGravityFieldActive = false;
	Vector3 gravity = { 0.0f, -9.8f, 0.0f };
};

struct ParticleUVAnimationSettings {
	bool isActive = false;
	bool isIndividual = false;            // 各パーティクル個別でアニメーションを行うか (エフェクト形式)
	Vector2 scrollSpeed = { 0.0f, 0.0f }; // U, V scroll speed per second
	float rotateSpeed = 0.0f;             // Rotation speed (rad/sec)
	Vector2 scaleSpeed = { 0.0f, 0.0f };  // Scale change speed per second

	// Current state values (for global animation)
	Vector2 currentTranslate = { 0.0f, 0.0f };
	float currentRotate = 0.0f;
	Vector2 currentScale = { 1.0f, 1.0f };
};

class ParticleEmitter {
public:
	// デフォルトコンストラクタ（shapeの初期化を追加）
	ParticleEmitter()
		: transform{}, count(1), frequency(0.1f), frequencyTime(0.0f), isEmit(true),
		isEffectMode(false), isLoop(false), isPlaying(false),
		shape(std::make_unique<BillboardShape>()) {
	}

	// 引数付きコンストラクタ（shapeの初期化を追加）
	ParticleEmitter(const Transform& t, uint32_t c, float freq)
		: transform(t), count(c), frequency(freq), frequencyTime(0.0f), isEmit(true),
		isEffectMode(false), isLoop(false), isPlaying(false),
		shape(std::make_unique<BillboardShape>()) {
	}

	// クローン（ディープコピー）のためのコピーコンストラクタとコピー代入演算子
	ParticleEmitter(const ParticleEmitter& other);
	ParticleEmitter& operator=(const ParticleEmitter& other);

	// ムーブコンストラクタとムーブ代入演算子も明示的にデフォルト定義
	ParticleEmitter(ParticleEmitter&& other) noexcept = default;
	ParticleEmitter& operator=(ParticleEmitter&& other) noexcept = default;

	~ParticleEmitter() = default;

	// 形状を取得 (型識別用)
	ParticleShapeType GetShapeType() const { return shape ? shape->GetType() : ParticleShapeType::Billboard; }

	// 形状を切り替える (既存の Model リソースも解放される)
	void SetShapeType(ParticleShapeType type);

	// 型を安全にキャストして取得するヘルパー
	RingShape* GetRingShape() { return dynamic_cast<RingShape*>(shape.get()); }
	CylinderShape* GetCylinderShape() { return dynamic_cast<CylinderShape*>(shape.get()); }
	PlaneShape* GetPlaneShape() { return dynamic_cast<PlaneShape*>(shape.get()); }

	void Emit(const std::string& groupName);

public:
	// エミッターの設定値
	Transform transform{}; // 位置、回転、スケール
	uint32_t count;      // 1回で発生させる個数
	float frequency;     // 発生頻度 (秒)
	float frequencyTime; // 頻度計算用の時刻 (初期値 0.0f)
	bool isEmit;         // 自動発生を有効にするかどうかのフラグ
	bool isEffectMode = false; // エフェクトモード（単発/ループ再生）にするか
	bool isLoop = false;       // アニメーション完了時にループ再生するか
	bool isPlaying = false;    // エフェクト再生中フラグ
	ParticleGenerateSettings generateSettings; // 生成時の設定
	ParticleFieldSettings fieldSettings;       // フィールドの設定
	ParticleUVAnimationSettings uvAnimationSettings; // UVアニメーションの設定

	// 多態性を持った形状クラスのポインタ
	std::unique_ptr<ParticleShape> shape;
};
