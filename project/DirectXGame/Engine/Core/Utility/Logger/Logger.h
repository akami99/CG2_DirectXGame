#pragma once

#include <string>
#include <fstream>

// ログ出力
namespace Logger {
	void InitializeFileLogging();

	void Log(const std::string& message);

    void Log(std::ostream& os, const std::string& message);

	void SetFileStream(std::ofstream* stream);
};



