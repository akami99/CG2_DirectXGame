#include "RAFramework.h"
// 必要なエンジンのインクルード
#include "Win32Window.h"
#include "DX12Context.h"
#include "Input.h"
#include "SrvManager.h"
#include "PipelineManager.h"
#include "ImGuiManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "Logger.h"

// dump用
#include <dbghelp.h>
#include <strsafe.h>

#pragma comment(lib, "Dbghelp.lib")

// ダンプ出力関数（static関数としてこのファイル内だけで使う）
static LONG WINAPI ExportDump(EXCEPTION_POINTERS *exception) {
    // ログと同じく、exeと同じ階層に「dumps」フォルダを作成する
    const wchar_t *dumpDir = L"dumps";

    CreateDirectoryW(dumpDir, nullptr);

    // 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリいかに出力
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t filePath[MAX_PATH] = { 0 };

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
    MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
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

void RAFramework::Run() {
    Initialize();

    // ウィンドウのxボタンが押されるまでループ
    while (true) {
        // メッセージ処理
        if (window_->ProcessMessage() || IsEndRequest()) {
            break;
        }

        // 更新
        Update();

        // 描画
        Draw();
    }

    Finalize();
}

void RAFramework::Initialize() {
    // ログ初期化
    Logger::InitializeFileLogging();

    /// システム基盤の初期化
    // Window
    window_ = new Win32Window();
    window_->Initialize();
    // DirectXBase
    dxBase_ = new DX12Context();
    dxBase_->Initialize(window_);
    // SRVManager
    srvManager_ = new SrvManager();
    srvManager_->Initialize(dxBase_);
    // DirectInput
    input_ = new Input();
    input_->Initialize(window_);
    // PipelineManager
    pipelineManager_ = new PipelineManager();
    pipelineManager_->Initialize(dxBase_);
    // ImGuiManager
    imGuiManager_ = new ImGuiManager();
    imGuiManager_->Initialize(dxBase_, window_, srvManager_);

    /// マネージャー類の初期化（シングルトン）
    TextureManager::GetInstance()->Initialize(dxBase_, srvManager_);
    ModelManager::GetInstance()->Initialize(dxBase_);
    ParticleManager::GetInstance()->Initialize(dxBase_, srvManager_, pipelineManager_);

    /// 共通描画設定の生成
    // SpriteCommon
    spriteCommon_ = new SpriteCommon();
    spriteCommon_->Initialize(dxBase_, pipelineManager_);
    // Object3dCommon
    object3dCommon_ = new Object3dCommon();
    object3dCommon_->Initialize(dxBase_, pipelineManager_);
}

void RAFramework::Update() {
    // 共通の更新処理があればここに書く（基本は継承先でオーバーライド）
}

void RAFramework::Finalize() {
    // 基盤の解放処理
    delete object3dCommon_;
    delete spriteCommon_;

    // マネージャーの終了
    ParticleManager::GetInstance()->Finalize();
    ModelManager::GetInstance()->Finalize();
    TextureManager::GetInstance()->Finalize();

    imGuiManager_->Finalize();
    delete imGuiManager_;

    delete pipelineManager_;
    delete input_;
    delete srvManager_;
    delete dxBase_;

    window_->Finalize();
    delete window_;
}
