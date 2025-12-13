#pragma once
#include <string>

// 文字コードユーティリティ
namespace StringUtility {

// string->wstring
std::wstring ConvertString(const std::string &str);

// wstring->string
std::string ConvertString(const std::wstring &str);
}; // namespace StringUtility
