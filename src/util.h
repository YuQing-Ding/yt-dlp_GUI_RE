// util.h —— 字符串、路径、进程相关的小工具
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>
#include <vector>

namespace util {

// ---- 编码 ----
std::wstring Utf8ToW(const std::string& s);
std::string  WToUtf8(const std::wstring& s);
// yt-dlp 管道输出优先按 UTF-8 解，非法字节序列则退回系统 ANSI 代码页。
std::wstring DecodeConsole(const std::string& s);

// ---- 字符串 ----
std::wstring Trim(const std::wstring& s);
std::wstring ToLower(std::wstring s);
bool StartsWith(const std::wstring& s, const std::wstring& p);
bool Contains(const std::wstring& s, const std::wstring& p);
bool ContainsNoCase(const std::wstring& s, const std::wstring& p);
std::wstring Replace(std::wstring s, const std::wstring& from, const std::wstring& to);
std::vector<std::wstring> SplitLines(const std::wstring& s);
std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep);
std::wstring Format(const wchar_t* fmt, ...);

// ---- 命令行 ----
// 按 CommandLineToArgvW 的反斜杠/引号规则转义单个参数。
std::wstring QuoteArg(const std::wstring& a);
std::wstring JoinCommandLine(const std::vector<std::wstring>& args);

// ---- 路径 ----
std::wstring ExeDir();
std::wstring PathJoin(const std::wstring& a, const std::wstring& b);
bool FileExists(const std::wstring& p);
bool DirExists(const std::wstring& p);
bool EnsureDir(const std::wstring& p);           // 递归创建
std::wstring KnownFolderDownloads();
std::wstring HomeDir();

// ---- 显示 ----
std::wstring FormatBytes(double bytes);
std::wstring FormatDuration(int seconds);

}  // namespace util
