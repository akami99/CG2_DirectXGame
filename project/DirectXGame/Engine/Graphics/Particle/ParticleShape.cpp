#include "ParticleShape.h"
#include "Model/Model.h"

// ============================================================
// BillboardShape の実装
// ============================================================
std::unique_ptr<ParticleShape> BillboardShape::clone() const {
    return std::make_unique<BillboardShape>(*this);
}

void BillboardShape::ApplyMaterial(Material* mat) const {
    if (!mat) return;
    mat->isRing = 0;
    mat->isCylinder = 0;

    mat->isUvSwap = 0;
    mat->fadeRange = 0.0f;
}

// ============================================================
// RingShape の実装
// ============================================================
RingShape::RingShape() = default;
RingShape::~RingShape() = default;

std::unique_ptr<ParticleShape> RingShape::clone() const {
    // model_ (unique_ptr) はコピーできないため、設定(settings)のみを引き継いだ新しいインスタンスを生成
    auto copy = std::make_unique<RingShape>();
    copy->settings = this->settings;
    // コピー直後はモデルが未作成なのでダーティフラグを立てる
    copy->MarkDirty();
    return copy;
}

void RingShape::ApplyMaterial(Material* mat) const {
    if (!mat) return;
    mat->isRing = 1;
    mat->isCylinder = 0;

    mat->isUvSwap = settings.isUvSwap ? 1 : 0;
    mat->innerColor = settings.innerColor;
    mat->outerColor = settings.outerColor;
    mat->fadeStartAlpha = settings.fadeStartAlpha;
    mat->fadeEndAlpha = settings.fadeEndAlpha;
    mat->fadeRange = settings.fadeRange;
}

Model* RingShape::GetOrBuildModel(const std::string& texPath) {
    // ダーティフラグが立っている、またはテクスチャパスが変わった場合のみ再構築
    if (isDirty_ || lastTexPath_ != texPath) {
        if (!model_) {
            model_ = std::make_unique<Model>();
        }

        // 既存のCreateRingの処理を呼び出す
        model_->CreateRing(texPath, settings);

        isDirty_ = false;
        lastTexPath_ = texPath;
    }
    return model_.get();
}

// ============================================================
// CylinderShape の実装
// ============================================================
CylinderShape::CylinderShape() = default;
CylinderShape::~CylinderShape() = default;

std::unique_ptr<ParticleShape> CylinderShape::clone() const {
    auto copy = std::make_unique<CylinderShape>();
    copy->settings = this->settings;
    copy->MarkDirty();
    return copy;
}

void CylinderShape::ApplyMaterial(Material* mat) const {
    if (!mat) return;
    mat->isRing = 0;
    mat->isCylinder = 1;
    mat->alphaReference = settings.alphaReference;

    mat->isUvSwap = settings.isUvSwap ? 1 : 0;
    mat->innerColor = settings.topColor;      // topColor -> innerColor
    mat->outerColor = settings.bottomColor;   // bottomColor -> outerColor
    mat->fadeStartAlpha = settings.fadeStartAlpha;
    mat->fadeEndAlpha = settings.fadeEndAlpha;
    mat->fadeRange = settings.fadeRange;
}

Model* CylinderShape::GetOrBuildModel(const std::string& texPath) {
    if (isDirty_ || lastTexPath_ != texPath) {
        if (!model_) {
            model_ = std::make_unique<Model>();
        }

        // 既存のCreateCylinderの処理を呼び出す
        model_->CreateCylinder(texPath, settings);

        isDirty_ = false;
        lastTexPath_ = texPath;
    }
    return model_.get();
}