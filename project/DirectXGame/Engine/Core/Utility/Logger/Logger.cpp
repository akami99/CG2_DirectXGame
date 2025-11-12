#include <dxgidebug.h>
#include <iostream>

#include "../Logger/Logger.h"

namespace Logger {
	// ログファイルを指すポインタ。デフォルトはnullptr。
	static std::ofstream* fileStream = nullptr;
	
	void SetFileStream(std::ofstream* stream) {
		fileStream = stream;
	}

	void Log(const std::string& message) {
		// VS出力に出力
		OutputDebugStringA(message.c_str());

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
}