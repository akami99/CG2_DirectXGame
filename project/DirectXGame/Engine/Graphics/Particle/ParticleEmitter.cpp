#include "ParticleEmitter.h"
#include "ParticleManager.h"

void ParticleEmitter::Emit(const std::string &groupName) {
    // エミッタの設定値に従って、ParticleManager::GetInstance()->Emit を呼び出す
    ParticleManager::GetInstance()->Emit(
        groupName,
        this->transform.translate, // エミッタの現在位置
        this->count                // 設定された個数
    );
}
