#include "ImGuiManager.h"
#include "API/DX12Context.h"
#include "SRV/SrvManager.h"
#include "Win32Window.h"

// 静的メンバ変数の初期化
DX12Context *ImGuiManager::dxBase_ = nullptr;
Win32Window *ImGuiManager::window_ = nullptr;
SrvManager *ImGuiManager::srvManager_ = nullptr;

// 初期化
void ImGuiManager::Initialize([[maybe_unused]] DX12Context *dxBase,
                              [[maybe_unused]] Win32Window *window,
                              [[maybe_unused]] SrvManager *srvManager) {
#ifdef USE_IMGUI

  // 渡されたポインタが有効かチェック
  assert(dxBase);
  assert(window);
  assert(srvManager);

  dxBase_ = dxBase;
  window_ = window;
  srvManager_ = srvManager;

  // ヒープが既に生成されているかチェック
  // もしここで止まるなら、srvManager->Initialize() が呼ばれていない
  assert(srvManager_->GetDescriptorHeap() != nullptr);

  // ImGuiのコンテキストを生成
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  uint32_t index = srvManager_->GetNewIndex();

  //  ImGuiのスタイルを設定
  ImGui::StyleColorsDark();
  // ImGuiをWin32に対応させる初期化
  ImGui_ImplWin32_Init(window_->GetHwnd());
  // ImGuiをDirectX12に対応させる初期化
  ImGui_ImplDX12_Init(
      dxBase_->GetDevice(), // デバイス
      static_cast<int>(
          dxBase_->GetSwapChainResourceCount()),  // スワップチェーンの数
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,            // RTVのフォーマット
      srvManager_->GetDescriptorHeap(),           // デスクリプタヒープ
      srvManager_->GetCPUDescriptorHandle(index), // CPU側ハンドルの先頭
      srvManager_->GetGPUDescriptorHandle(index)  // GPU側ハンドルの先頭
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

  ID3D12GraphicsCommandList *commandList = dxBase_->GetCommandList();

  // デスクリプタヒープの配列をセットするコマンド
  ID3D12DescriptorHeap *ppHeaps[] = {srvManager_->GetDescriptorHeap()};
  commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  // 描画コマンドを発行
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

  #endif
}