#pragma once
#include <cstdint>

// WindowsAPI
class WindowWrapper {
public: // 定数
	// クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

private: // メンバ変数
	// ウィンドウハンドル
	HWND hwnd_ = nullptr;
	// ウィンドウクラスの設定
	WNDCLASS wc_{};

public: // 静的メンバ関数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public: // メンバ関数
	// 初期化
	void Initialize();
	// 更新
	void Update();
	// 終了
	void Finalize();

	// hwndのgetter
	HWND GetHwnd() const {
		return hwnd_;
	}
	// hinstanceのgetter
	HINSTANCE GetHinstance() const {
		return wc_.hInstance;
	}

};

