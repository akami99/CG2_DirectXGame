#pragma once

#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <Windows.h>               // HWNDのために必要
#include <dinput.h>                // DirectInputのヘッダーファイル
#include <wrl.h>                   // ComPtr
#include <memory>                  // unique_ptr

// キーボード入力を管理するクラス
class Input {
public:
    // namespace省略のためにComPtrをusing宣言
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    // 【Passkey Idiom】
    // make_unique からコンストラクタを呼べるようにしつつ、
    // 他のクラスからの作成を禁止するための「鍵」
    struct Token {
    private:
        friend class Input;
        Token() {}
    };

private: // メンバ変数
    // スマートポインタで管理
    static std::unique_ptr<Input> instance_;

    // unique_ptr が private なデストラクタを呼べるようにする
    friend std::default_delete<Input>;

public: // メンバ関数
    // コンストラクタ (Tokenがないと呼べない = 実質private)
    explicit Input(Token);

    // デストラクタ
    ~Input();

    // コピー禁止 / ムーブ禁止
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    Input(Input&&) = delete;
    Input& operator=(Input&&) = delete;

    // シングルトンインスタンスの取得
    static Input* GetInstance();

    // 終了処理 (シングルトンインスタンスの破棄)
    static void Destroy();

    // 初期化
    void Initialize();

    // 状態を更新するメソッド (毎フレーム呼び出す)
    void Update();

#pragma region Keyboard
private: // キーボード関連のメンバ変数
    ComPtr<IDirectInput8> directInput_;    // DirectInputオブジェクト
    ComPtr<IDirectInputDevice8> keyboard_; // DirectInputキーボードデバイス
    BYTE key_[256] = {};                   // 現在のキー状態
    BYTE preKey_[256] = {};                // 前回のキー状態

public: // キーボード関連のメンバ関数
    /// <summary>
    /// キーの押下をチェック
    /// </summary>
    bool IsKeyDown(BYTE keyNumber);

    /// <summary>
    /// キーのトリガーチェック
    /// </summary>
    bool IsKeyTriggered(BYTE keyNumber);

    // 指定されたキーが離された瞬間か
    /// <summary>
    /// キーが離された瞬間かをチェック
    /// </summary>
    /// <param name="keyNumber">キー番号</param>
    /// <returns>離された瞬間か</returns>
    bool IsKeyReleased(BYTE keyNumber);
#pragma endregion Keyboardここまで

#pragma region Mouse
private: // マウス関連のメンバ変数
    ComPtr<IDirectInputDevice8> mouse_;    // マウスデバイス
    DIMOUSESTATE2 mouseState_;             // 現在のマウス状態
    DIMOUSESTATE2 preMouseState_;          // 前回のマウス状態

public: // マウス関連のメンバ関数
    /// <summary>
    /// 移動量を取得
    /// </summary>
    /// <param name="x">X方向の移動量</param>
    /// <param name="y">Y方向の移動量</param>
    void GetMouseMove(long& x, long& y);

    /// <summary>
	/// マウスボタンの押下をチェック
    /// </summary>
    /// <param name="button">ボタン番号 (0:左, 1:右, 2:中)</param>
    /// <returns>押下されているか</returns>
    bool IsMouseButtonDown(int button);

	/// <summary>
	/// マウスボタンのトリガーチェック
	/// </summary>
	/// <param name="button">ボタン番号 (0:左, 1:右, 2:中)</param>
	/// <returns>押下された瞬間か</returns>
    bool IsMouseButtonTriggered(int button);
#pragma endregion Mouseここまで
};