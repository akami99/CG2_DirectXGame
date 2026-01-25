#pragma once

// 前方宣言（シーンマネージャーを作る時に必要になるかも）
class IScene {
public:
    virtual ~IScene() = default;

    // 初期化
    virtual void Initialize() = 0;

    // 更新
    virtual void Update() = 0;

    // 描画
    virtual void Draw() = 0;

    // 終了処理
    virtual void Finalize() = 0;
};