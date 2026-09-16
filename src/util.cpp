#include "util.h"

#include <shlobj.h>
#include <shlwapi.h>

#include <cstdarg>
#include <cwchar>

namespace util {

std::wstring Utf8ToW(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
    return out;
}

std::string WToUtf8(const std::wstring& s) {
    if (s.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string out((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring DecodeConsole(const std::string& s) {
    if (s.empty()) return {};
    // 先按严格 UTF-8 试；yt-dlp 在我们设了 PYTHONUTF8=1 之后应该总是 UTF-8。
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0);
    if (n > 0) {
        std::wstring out((size_t)n, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
        return out;
    }
    // 退回 ANSI，保证不会因为一个坏字节就丢掉整行日志。
    n = MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out((size_t)n, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), out.data(), n);
    return out;
}

std::wstring Trim(const std::wstring& s) {
    size_t b = s.find_first_not_of(L" \t\r\n\f\v");
    if (b == std::wstring::npos) return {};
    size_t e = s.find_last_not_of(L" \t\r\n\f\v");
    return s.substr(b, e - b + 1);
}

std::wstring ToLower(std::wstring s) {
    for (auto& c : s) c = (wchar_t)towlower(c);
    return s;
}

bool StartsWith(const std::wstring& s, const std::wstring& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool Contains(const std::wstring& s, const std::wstring& p) {
    return s.find(p) != std::wstring::npos;
}

bool ContainsNoCase(const std::wstring& s, const std::wstring& p) {
    return Contains(ToLower(s), ToLower(p));
}

std::wstring Replace(std::wstring s, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::wstring::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::vector<std::wstring> SplitLines(const std::wstring& s) {
    std::vector<std::wstring> out;
    std::wstring cur;
    for (wchar_t c : s) {
        if (c == L'\n') {
            out.push_back(cur);
            cur.clear();
        } else if (c != L'\r') {
            cur += c;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep) {
    std::vector<std::wstring> out;
    std::wstring cur;
    for (wchar_t c : s) {
        if (c == sep) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

std::wstring Format(const wchar_t* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = _vscwprintf(fmt, ap);
    va_end(ap);
    if (n <= 0) return {};
    std::wstring out((size_t)n, L'\0');
    va_start(ap, fmt);
    vswprintf_s(out.data(), (size_t)n + 1, fmt, ap);
    va_end(ap);
    return out;
}

std::wstring QuoteArg(const std::wstring& a) {
    // 不含空格、引号和制表符的参数可以原样传。
    if (!a.empty() && a.find_first_of(L" \t\n\v\"") == std::wstring::npos) return a;

    std::wstring out = L"\"";
    for (size_t i = 0;; ++i) {
        size_t slashes = 0;
        while (i < a.size() && a[i] == L'\\') { ++i; ++slashes; }
        if (i == a.size()) {
            // 结尾的反斜杠要加倍，否则会转义掉收尾的引号。
            out.append(slashes * 2, L'\\');
            break;
        }
        if (a[i] == L'"') {
            out.append(slashes * 2 + 1, L'\\');
            out += L'"';
        } else {
            out.append(slashes, L'\\');
            out += a[i];
        }
    }
    out += L'"';
    return out;
}

std::wstring JoinCommandLine(const std::vector<std::wstring>& args) {
    std::wstring out;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) out += L' ';
        out += QuoteArg(args[i]);
    }
    return out;
}

std::wstring ExeDir() {
    wchar_t buf[MAX_PATH * 2] = {};
    DWORD n = GetModuleFileNameW(nullptr, buf, (DWORD)std::size(buf));
    std::wstring p(buf, n);
    size_t slash = p.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"." : p.substr(0, slash);
}

std::wstring PathJoin(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == L'\\' || a.back() == L'/') return a + b;
    return a + L"\\" + b;
}

bool FileExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

bool DirExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

bool EnsureDir(const std::wstring& p) {
    if (p.empty()) return false;
    if (DirExists(p)) return true;
    int rc = SHCreateDirectoryExW(nullptr, p.c_str(), nullptr);
    return rc == ERROR_SUCCESS || rc == ERROR_ALREADY_EXISTS || rc == ERROR_FILE_EXISTS;
}

std::wstring KnownFolderDownloads() {
    PWSTR path = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &path))) {
        out = path;
        CoTaskMemFree(path);
    }
    return out;
}

std::wstring HomeDir() {
    PWSTR path = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path))) {
        out = path;
        CoTaskMemFree(path);
    }
    return out;
}

std::wstring FormatBytes(double b) {
    const wchar_t* units[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
    int u = 0;
    while (b >= 1024.0 && u < 4) { b /= 1024.0; ++u; }
    return Format(u == 0 ? L"%.0f %s" : L"%.1f %s", b, units[u]);
}

std::wstring FormatDuration(int s) {
    if (s < 0) return L"--:--";
    int h = s / 3600, m = (s % 3600) / 60, sec = s % 60;
    if (h > 0) return Format(L"%d:%02d:%02d", h, m, sec);
    return Format(L"%02d:%02d", m, sec);
}

}  // namespace util
