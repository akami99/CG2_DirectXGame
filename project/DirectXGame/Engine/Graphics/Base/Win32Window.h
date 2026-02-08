#pragma once
#include <cstdint>
#include <memory> // std::unique_ptr

#define WIN32_LEAN_AND_MEAN // 不要なWindows機能を省いてビルドを速くする
#include <Windows.h>

// WindowsAPI
class Win32Window {
public: // 静的定数
    // クライアント領域のサイズ
    static const int32_t kClientWidth = 1280;
    static const int32_t kClientHeight = 720;

    // 【Passkey Idiom】
    // make_uniqueからのみ呼べるようにするための「鍵」構造体
    struct Token {
    private:
        friend class Win32Window;
        Token() {}
    };

public: // 静的メンバ関数
    // ウィンドウプロシージャ
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    // シングルトンインスタンスの取得
    static Win32Window* GetInstance();
    // 終了処理（インスタンスの破棄）
    static void Destroy();

public: // メンバ関数
    // コンストラクタ (Tokenが必要なので実質private)
    Win32Window(Token);
    // デストラクタ
    ~Win32Window();

    // コピー・ムーブ禁止
    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;
    Win32Window(Win32Window&&) = delete;
    Win32Window& operator=(Win32Window&&) = delete;

    // 初期化（ウィンドウの生成など）
    void Initialize();

    // メッセージ処理
    bool ProcessMessage();

    // hwndのgetter
    HWND GetHwnd() const { return hwnd_; }
    // hinstanceのgetter
    HINSTANCE GetHinstance() const { return wc_.hInstance; }

private: // メンバ変数
    // スマートポインタで管理
    static std::unique_ptr<Win32Window> instance_;
    // unique_ptrがprivateデストラクタを呼べるようにする
    friend std::default_delete<Win32Window>;

    // ウィンドウハンドル
    HWND hwnd_ = nullptr;
    // ウィンドウクラスの設定
    WNDCLASS wc_{};
};
