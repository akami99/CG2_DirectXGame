#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>

#include "../API/D3DResourceLeakChecker.h"

using namespace Microsoft::WRL;

D3DResourceLeakChecker::~D3DResourceLeakChecker() {
  // リソースリークチェック
  ComPtr<IDXGIDebug1> debug;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
    // 詳細なリーク情報を出力
    OutputDebugStringA("=== DirectX Resource Leak Check ===\n");

    // すべてのDXGIオブジェクトのリークをレポート
    OutputDebugStringA("--- DXGI_DEBUG_ALL ---\n");
    debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

    // アプリケーション固有のリークをレポート
    OutputDebugStringA("--- DXGI_DEBUG_APP ---\n");
    debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);

    // Direct3D 12のリークをレポート
    OutputDebugStringA("--- DXGI_DEBUG_D3D12 ---\n");
    debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);

    OutputDebugStringA("=== End of Leak Check ===\n");
  }
}
