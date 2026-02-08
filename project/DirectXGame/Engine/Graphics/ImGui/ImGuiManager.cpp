#include "ImGuiManager.h"
#include "Base/DX12Context.h"
#include "Base/SrvManager.h"
#include "Base/Win32Window.h"

// 初期化
void ImGuiManager::Initialize() {
#ifdef USE_IMGUI

  // ヒープが既に生成されているかチェック
  // もしここで止まるなら、srvManager->Initialize() が呼ばれていない
  assert(SrvManager::GetInstance()->GetDescriptorHeap() != nullptr);

  // ImGuiのコンテキストを生成
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  uint32_t index = SrvManager::GetInstance()->Allocate();

  //  ImGuiのスタイルを設定
  ImGui::StyleColorsDark();
  // ImGuiをWin32に対応させる初期化
  ImGui_ImplWin32_Init(Win32Window::GetInstance()->GetHwnd());
  // ImGuiをDirectX12に対応させる初期化
  ImGui_ImplDX12_Init(
      DX12Context::GetInstance()->GetDevice(), // デバイス
      static_cast<int>(
          DX12Context::GetInstance()
              ->GetSwapChainResourceCount()),         // スワップチェーンの数
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,                // RTVのフォーマット
      SrvManager::GetInstance()->GetDescriptorHeap(), // デスクリプタヒープ
      SrvManager::GetInstance()->GetCPUDescriptorHandle(
          index), // CPU側ハンドルの先頭
      SrvManager::GetInstance()->GetGPUDescriptorHandle(
          index) // GPU側ハンドルの先頭
  );

  // Initializeの最後に追加
  ImGuiIO &io = ImGui::GetIO();
  // フォントアトラスを強制的にビルドする
  // ※imguiのアップデートが進むと使えなくなるなくなる可能性あり
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

#endif
}

// 終了
void ImGuiManager::Finalize() {
#ifdef USE_IMGUI

  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

#endif
}

// ImGuiの受付開始
void ImGuiManager::Begin() {
#ifdef USE_IMGUI

  assert(ImGui::GetIO().BackendRendererName != nullptr &&
         "backend setting is not found! CreateContext is double called?");
  // ImGuiフレーム開始
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

#endif
}

// ImGuiの受付終了
void ImGuiManager::End() {
#ifdef USE_IMGUI

  // 描画前準備
  ImGui::Render();

#endif
}

// 描画
void ImGuiManager::Draw() {
#ifdef USE_IMGUI

  ID3D12GraphicsCommandList *commandList =
      DX12Context::GetInstance()->GetCommandList();

  // デスクリプタヒープの配列をセットするコマンド
  ID3D12DescriptorHeap *ppHeaps[] = {
      SrvManager::GetInstance()->GetDescriptorHeap()};
  commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  // 描画コマンドを発行
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

#endif
}