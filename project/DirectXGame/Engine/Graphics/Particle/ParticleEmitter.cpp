#include "ParticleEmitter.h"
#include "ParticleManager.h"

// 共通のコピー処理を行うヘルパー関数
void CopyEmitterMembers(const ParticleEmitter& src, ParticleEmitter& dest) {
	dest.transform = src.transform;
	dest.count = src.count;
	dest.frequency = src.frequency;
	dest.frequencyTime = src.frequencyTime;
	dest.isEmit = src.isEmit;
	dest.isEffectMode = src.isEffectMode;
	dest.isLoop = src.isLoop;
	dest.isPlaying = src.isPlaying;
	dest.generateSettings = src.generateSettings;
	dest.fieldSettings = src.fieldSettings;
	dest.uvAnimationSettings = src.uvAnimationSettings;

	// shapeのディープコピー
	if (src.shape) {
		dest.shape = src.shape->clone();
	}
	else {
		dest.shape = std::make_unique<BillboardShape>();
	}
}

// コピーコンストラクタ: shape の clone() を呼んで複製（ディープコピー）
ParticleEmitter::ParticleEmitter(const ParticleEmitter& other) {
	CopyEmitterMembers(other, *this);
}

// コピー代入演算子
ParticleEmitter& ParticleEmitter::operator=(const ParticleEmitter& other) {
	if (this != &other) {
		CopyEmitterMembers(other, *this);
	}
	return *this;
}

// 形状の動的切り替え
void ParticleEmitter::SetShapeType(ParticleShapeType type) {
	// 現在の形状と異なる場合のみインスタンスを再生成
	if (GetShapeType() == type) return;

	switch (type) {
	case ParticleShapeType::Billboard:
		shape = std::make_unique<BillboardShape>();
		break;
	case ParticleShapeType::Ring:
		shape = std::make_unique<RingShape>();
		break;
	case ParticleShapeType::Cylinder:
		shape = std::make_unique<CylinderShape>();
		break;
	case ParticleShapeType::Plane:
		shape = std::make_unique<PlaneShape>();
		break;
	}
}

void ParticleEmitter::Emit(const std::string& groupName) {
	// エミッタの設定値に従って、ParticleManager::GetInstance()->Emit を呼び出す
	ParticleManager::GetInstance()->Emit(
		groupName,
		this->transform.translate, // エミッタの現在位置
		this->count                // 設定された個数
	);
}
