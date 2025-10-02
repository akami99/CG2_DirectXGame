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
	HWND hwnd = nullptr;
	// ウィンドウクラスの設定
	WNDCLASS wc{};

public: // 静的メンバ変数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public: // メンバ関数
	// 初期化
	void Initialize();
	// 更新
	void Update();

	// hwndのgetter
	HWND GetHwnd() const {
		return hwnd;
	}
	// hinstanceのgetter
	HINSTANCE GetHinstance() const {
		return wc.hInstance;
	}

};

