#include "../Logger/Logger.h"

#include <chrono>
#include <dxgidebug.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace Logger {
	// 内部管理用の静的変数
	// 【ファイルストリームの実体】初期化関数内でオープンし、Log関数へアドレスを渡す
	static std::ofstream logStream;
    // 【ファイル書き込みに使うポインタ】Log関数で使用
	static std::ofstream* fileStream = nullptr;

	// 設定定数
	const std::string LOG_DIR = "logs";
	const std::string CURRENT_FILE = "current.log";
	const int MAX_BACKUPS = 5;

	void SetFileStream(std::ofstream* stream) { fileStream = stream; }

	/// <summary>
	/// 古いログファイルをリネームして整理する
	/// </summary>
	void RotateLogFiles() {
		// 古いバックアップから順に後ろへずらす
		// backup_1.log -> backup_2.log
		for (int i = MAX_BACKUPS; i > 1; --i) {
			std::string oldPath = LOG_DIR + "/backup_" + std::to_string(i - 1) + ".log";
			std::string newPath = LOG_DIR + "/backup_" + std::to_string(i) + ".log";

			if (fs::exists(oldPath)) {
				// 既存の backup_2 があれば削除して置換
				if (fs::exists(newPath)) fs::remove(newPath);
				fs::rename(oldPath, newPath);
			}
		}

		// 現在のメインログをバックアップへ移動
		// current.log -> backup_1.log
		std::string currentPath = LOG_DIR + "/" + CURRENT_FILE;
		std::string firstBackupPath = LOG_DIR + "/backup_1.log";

		if (fs::exists(currentPath)) {
			if (fs::exists(firstBackupPath)) fs::remove(firstBackupPath);
			fs::rename(currentPath, firstBackupPath);
		}
	}

	void InitializeFileLogging() {
		// ディレクトリの作成
		if (!fs::exists(LOG_DIR)) {
			fs::create_directories(LOG_DIR);
		}

		// ファイルの世代交代を実行
		RotateLogFiles();

		// 新しいログファイルをオープン
		std::string logPath = LOG_DIR + "/" + CURRENT_FILE;
		logStream.open(logPath, std::ios::out);

		if (logStream.is_open()) {
			SetFileStream(&logStream);

			// ログのヘッダーに開始時刻を記録しておく
			auto now = std::chrono::system_clock::now();
			std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
				nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
			std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
			std::string dateStr = std::format("{:%Y-%m-%d %H:%M:%S}", localTime);
			Log("\n--- Logging Started: " + dateStr + " ---\n");
		}
	}

	void Log(const std::string& message) {
		// VS出力に出力
		OutputDebugStringA((message + "\n").c_str());

		// ファイルストリームが有効ならファイルにも書き込む
		if (fileStream && fileStream->is_open()) {
			(*fileStream) << message << std::endl;
			fileStream->flush(); // 即時書き込みを保証
		}
	}

	void Log(std::ostream& os, const std::string& message) {
		os << message << std::endl;
		OutputDebugStringA(message.c_str());
	}
} // namespace Logger