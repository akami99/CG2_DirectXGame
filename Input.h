#pragma once

#define DIRECTIONINPUT_VERSION    0x0800 // DirectInputのバージョン指定
#include <dinput.h> // DirectInputのヘッダーファイル
#include <Windows.h> // HWNDのために必要
#include <wrl.h>   // Microsoft::WRL::ComPtrのために必要

using namespace Microsoft::WRL; // ComPtrを使うために必要

// キーボード入力を管理するクラス
class Input {
public:
	// namespace省略のためにComPtrをusing宣言
	template <class T > using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
    //IDirectInput8* m_pDirectInput;     // DirectInputオブジェクト
    ComPtr<IDirectInputDevice8> keyboard_;        // DirectInputキーボードデバイス
    BYTE                m_currentKeyState[256]; // 現在のキー状態
    BYTE                m_previousKeyState[256]; // 前回のキー状態
    HWND                m_hWnd;             // ウィンドウハンドル

public:
    // 初期化
    // ウィンドウハンドルとHINSTANCEを引数として受け取る
    void Initialize(HINSTANCE hInstance, HWND hWnd);

    // デストラクタ
    // DirectInputオブジェクトとキーボードデバイスを解放する
    ~Input();

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