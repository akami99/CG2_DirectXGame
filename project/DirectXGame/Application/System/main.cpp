#include <Windows.h>

#include "MyGame.h"

#include <cassert>
#include <dbghelp.h> // dump
#include <dxgi1_6.h> // dump
#include <strsafe.h> // dump
// エンジン
#include "D3DResourceLeakChecker.h"
#include "Logger.h"

#pragma region 関数定義
// 関数定義

static LONG WINAPI ExportDump(EXCEPTION_POINTERS *exception) {
  // ログと同じく、exeと同じ階層に「dumps」フォルダを作成する
  const wchar_t *dumpDir = L"dumps";

  CreateDirectoryW(dumpDir, nullptr);

  // 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリいかに出力
  SYSTEMTIME time;
  GetLocalTime(&time);
  wchar_t filePath[MAX_PATH] = {0};

  StringCchPrintfW(filePath, MAX_PATH, L"%s/%04d-%02d%02d-%02d%02d%02d.dmp",
                   dumpDir, time.wYear, time.wMonth, time.wDay, time.wHour,
                   time.wMinute, time.wSecond);

  HANDLE dumpFileHandle =
      CreateFile(filePath, GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

  // 4. ハンドルの有効性チェック
  if (dumpFileHandle == INVALID_HANDLE_VALUE) {
    // ファイル作成に失敗した場合（権限不足など）、ダンプ処理はスキップして終了
    // エラーコードをログに記録できれば理想だが、ここではシンプルな終了とする
    return EXCEPTION_EXECUTE_HANDLER;
  }

  // processId（このexeのID）とクラッシュ（例外）の発生したthreadIdを取得
  DWORD processId = GetCurrentProcessId();
  DWORD threadId = GetCurrentThreadId();
  // 設定情報を入力
  MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{0};
  minidumpInformation.ThreadId = threadId;
  minidumpInformation.ExceptionPointers = exception;
  minidumpInformation.ClientPointers = TRUE;
  // Dumpを出力、MiniDumpNormalは最低限の情報を出力するフラグ
  MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
                    MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

  // ファイルハンドルを閉じる
  CloseHandle(dumpFileHandle);

  // 他に関連付けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
  return EXCEPTION_EXECUTE_HANDLER;
}

#pragma endregion 関数定義ここまで

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  // 誰も捕捉しなかった場合に(Unhandled)、補足する関数を登録
  SetUnhandledExceptionFilter(ExportDump);
  CoInitializeEx(0, COINIT_MULTITHREADED);

  D3DResourceLeakChecker leakCheck; // リソースリークチェック用のオブジェクト
  Logger::InitializeFileLogging();  // ログを用意

  // ゲームオブジェクトの生成
  MyGame* game = new MyGame();

  // ゲームの実行
  game->Run();

  delete game;

  return 0;
}