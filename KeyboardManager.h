#pragma once

// キーボード入力を管理するクラス
class KeyboardManager {
private:
    IDirectInput8* m_pDirectInput;     // DirectInputオブジェクト
    IDirectInputDevice8* m_pKeyboard;        // DirectInputキーボードデバイス
    BYTE                m_currentKeyState[256]; // 現在のキー状態
    BYTE                m_previousKeyState[256]; // 前回のキー状態
    HWND                m_hWnd;             // ウィンドウハンドル

public:
    // コンストラクタ
    // ウィンドウハンドルとHINSTANCEを引数として受け取る
    KeyboardManager(HWND hWnd, HINSTANCE hInstance);

    // デストラクタ
    // DirectInputオブジェクトとキーボードデバイスを解放する
    ~KeyboardManager();

    // キーボードの状態を更新するメソッド (毎フレーム呼び出す)
    void Update();

    // 指定されたキーが押されているか (押しっぱなしも含む)
    bool IsKeyDown(int dikCode) const {
        return (m_currentKeyState[dikCode] & 0x80) ? true : false;
    }

    // 指定されたキーが押された瞬間か (トリガー)
    bool IsTriggered(int dikCode) const {
        return (!(m_previousKeyState[dikCode] & 0x80) && (m_currentKeyState[dikCode] & 0x80));
    }

    // 指定されたキーが離された瞬間か
    bool IsReleased(int dikCode) const {
        return ((m_previousKeyState[dikCode] & 0x80) && !(m_currentKeyState[dikCode] & 0x80));
    }
};