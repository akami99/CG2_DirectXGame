#include <dinput.h> // DirectInputのヘッダーファイル
#include <cassert>  // assertのために必要
#include <Windows.h> // HWNDのために必要
#include "KeyboardManager.h"

// コンストラクタ
    // ウィンドウハンドルとHINSTANCEを引数として受け取る
KeyboardManager::KeyboardManager(HWND hWnd, HINSTANCE hInstance)
    : m_pDirectInput(nullptr), m_pKeyboard(nullptr), m_hWnd(hWnd) {
    ZeroMemory(m_currentKeyState, sizeof(m_currentKeyState));
    ZeroMemory(m_previousKeyState, sizeof(m_previousKeyState));

    // DirectInputの初期化
    HRESULT hr = DirectInput8Create(
        hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
        reinterpret_cast<void**>(&m_pDirectInput), nullptr);
    assert(SUCCEEDED(hr));

    // キーボードデバイスの生成
    hr = m_pDirectInput->CreateDevice(GUID_SysKeyboard, &m_pKeyboard, NULL);
    assert(SUCCEEDED(hr));

    // 入力データ形式のセット
    hr = m_pKeyboard->SetDataFormat(&c_dfDIKeyboard); // c_dfDIKeyboardはDirectInputで定義済み
    assert(SUCCEEDED(hr));

    // 排他制御のレベルのセット
    hr = m_pKeyboard->SetCooperativeLevel(
        m_hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(hr));

    // デバイスの取得開始 (Acquire)
    // 初期化時に一度Acquireしておくと、初回UpdateでGetDeviceStateを呼ぶ際にエラーになりにくい
    m_pKeyboard->Acquire();
}

// デストラクタ
// DirectInputオブジェクトとキーボードデバイスを解放する
KeyboardManager::~KeyboardManager() {
    if (m_pKeyboard) {
        m_pKeyboard->Unacquire(); // デバイスの取得を解除
        m_pKeyboard->Release();   // デバイスを解放
        m_pKeyboard = nullptr;
    }
    if (m_pDirectInput) {
        m_pDirectInput->Release(); // DirectInputオブジェクトを解放
        m_pDirectInput = nullptr;
    }
}

// キーボードの状態を更新するメソッド (毎フレーム呼び出す)
void KeyboardManager::Update() {
    // 前回の状態を現在の状態として保存
    memcpy(m_previousKeyState, m_currentKeyState, sizeof(m_previousKeyState));

    // 現在のキー状態を取得
    HRESULT hr = m_pKeyboard->GetDeviceState(sizeof(m_currentKeyState), m_currentKeyState);

    // デバイスがロストした場合の処理 (フォアグラウンドから切り替わった場合など)
    if (FAILED(hr)) {
        // 再びAcquireを試みる
        hr = m_pKeyboard->Acquire();
        if (SUCCEEDED(hr)) {
            // 再Acquireに成功したら再度状態を取得
            m_pKeyboard->GetDeviceState(sizeof(m_currentKeyState), m_currentKeyState);
        } else {
            // 再Acquireにも失敗した場合は、キー状態を全てクリアするなどの対応が必要な場合がある
            ZeroMemory(m_currentKeyState, sizeof(m_currentKeyState));
        }
    }
}