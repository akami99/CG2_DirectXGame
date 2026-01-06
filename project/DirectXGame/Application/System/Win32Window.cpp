#include <Windows.h>
#include <imgui_impl_win32.h>

#include "../Core/ApplicationConfig.h"
#include "../System/Win32Window.h"


#pragma comment(lib, "winmm.lib")

#ifdef USE_IMGUI
// ImGuiのWin32用ウィンドウプロシージャハンドラの宣言
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);
#endif

// ウィンドウプロシージャ
LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                         LPARAM lparam) {
#ifdef USE_IMGUI
  // ImGuiのウィンドウプロシージャハンドラを呼び出す
  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
    return true;
  }
#endif

  // メッセージに応じてゲーム固有の処理を行う
  switch (msg) {
    // ウィンドウが破棄された
  case WM_DESTROY:
    // OSに対して、アプリの終了を伝える
    PostQuitMessage(0);
    return 0;
  }

  // 標準のメッセージ処理を行う
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

// 初期化
void Win32Window::Initialize() {
  // システムタイマーの分解能を上げる
  timeBeginPeriod(1);

  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

  // ウィンドウプロシージャ
  wc_.lpfnWndProc = WindowProc;
  // ウィンドウクラス名
  wc_.lpszClassName = L"CG2WindowClass";
  // インスタンスハンドル
  wc_.hInstance = GetModuleHandle(nullptr);
  // カーソル
  wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

  // ウィンドウクラスを登録する
  RegisterClass(&wc_);

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, kClientWidth, kClientHeight};

  // クライアント領域を元に実際のサイズにwrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

  // ウィンドウの生成
  hwnd_ = CreateWindow(
      wc_.lpszClassName,        // 利用するクラス名
      L"LE2B_01_アカミネ_レン", // タイトルバーの文字（なんでも良い）
      WS_OVERLAPPEDWINDOW,      // よく見るウィンドウスタイル
      CW_USEDEFAULT,            // 表示X座標（Windowsに任せる）
      CW_USEDEFAULT,            // 表示Y座標（WindowsOSに任せる）
      wrc.right - wrc.left,     // ウィンドウ横幅
      wrc.bottom - wrc.top,     // ウィンドウ縦幅
      nullptr,                  // 親ウィンドウハンドル
      nullptr,                  // メニューハンドル
      wc_.hInstance,            // インスタンスハンドル
      nullptr);                 // オプション

  // ウィンドウを表示する
  ShowWindow(hwnd_, SW_SHOW);
}

// 終了
void Win32Window::Finalize() {
  // ウィンドウ破棄
  CloseWindow(hwnd_);
  CoUninitialize();
}

// メッセージ処理
bool Win32Window::ProcessMessage() {
  // メッセージがあるかどうかをチェックする
  MSG msg{};

  // メッセージがある場合は取り出して処理する
  if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    // 閉じるボタンが押されたら終了
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // 終了メッセージが来ていたらtrueを返す
  if (msg.message == WM_QUIT) {
    return true;
  }

  return false;
}
