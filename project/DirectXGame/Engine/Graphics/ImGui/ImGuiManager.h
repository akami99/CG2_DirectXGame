#pragma once

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#endif

class DX12Context;
class Win32Window;
class SrvManager;

// ImGuiの管理
class ImGuiManager {
private:
  static DX12Context *dxBase_;
  static Win32Window *window_;
  static SrvManager *srvManager_;

public:
  // 初期化
  static void Initialize(DX12Context *dxBase, Win32Window *window,
                         SrvManager *srvManager);
  // 終了
  void Finalize();

  // ImGuiの受付開始
  void Begin();
  // ImGuiの受付終了
  void End();
  // 描画
  void Draw();
};
