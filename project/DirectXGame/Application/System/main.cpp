#include <Windows.h>
#include "MyGame.h"

#include "D3DResourceLeakChecker.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  CoInitializeEx(0, COINIT_MULTITHREADED);
  D3DResourceLeakChecker leakCheck; // リソースリークチェック用のオブジェクト

  // ゲームオブジェクトの生成
  RAFramework* game = new MyGame();

  // ゲームの実行
  game->Run();

  delete game;
  CoUninitialize();

  return 0;
}