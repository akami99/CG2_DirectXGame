#include <chrono>
#include <dxgidebug.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#include "../Logger/Logger.h"

namespace Logger {
// 【ファイル書き込みに使うポインタ】Log関数で使用
static std::ofstream *fileStream = nullptr;

// 【ファイルストリームの実体】初期化関数内でオープンし、Log関数へアドレスを渡す
static std::ofstream logStream;

void SetFileStream(std::ofstream *stream) { fileStream = stream; }

void InitializeFileLogging() {
  // ログのディレクトリを用意
  const std::string logDir = "logs";

  // ディレクトリ作成を試みる
  if (std::filesystem::create_directories(logDir) ||
      std::filesystem::exists(logDir)) {
    // 現在時刻を取得からファイル名生成はそのまま...
    std::chrono::system_clock::time_point now =
        std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
        nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    std::chrono::zoned_time localTime{std::chrono::current_zone(), nowSeconds};
    std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);

    // 時刻を使ってファイル名を決定
    std::string logFilePath = logDir + "/" + dateString + ".log";

    // ファイルを作って書き込み準備
    logStream.open(logFilePath);

    // ファイルオープンに成功した場合のみ、ロガーを有効化する
    if (logStream.is_open()) {
      // Logger.cpp内部の静的ストリームポインタ（fileStream）を設定
      Logger::SetFileStream(&logStream);

      Logger::Log("--- Program Initialized Logging Successfully ---\n");
    }
  }
}

void Log(const std::string &message) {
  // VS出力に出力
  OutputDebugStringA(message.c_str());

  // ファイルストリームが有効ならファイルにも書き込む
  if (fileStream && fileStream->is_open()) {
    (*fileStream) << message << std::endl;
    fileStream->flush(); // 即時書き込みを保証
  }
}

void Log(std::ostream &os, const std::string &message) {
  os << message << std::endl;
  OutputDebugStringA(message.c_str());
}
} // namespace Logger