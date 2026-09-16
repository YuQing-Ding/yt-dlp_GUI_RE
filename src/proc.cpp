#include "proc.h"

#include <map>

namespace proc {

namespace {

// 一次 ReadFile 的缓冲。太小会让高频进度行变成大量小包，太大没意义。
constexpr DWORD kReadChunk = 16 * 1024;

std::vector<std::wstring> ExtraPathDirs() {
    std::vector<std::wstring> dirs;
    std::wstring home = util::HomeDir();
    if (!home.empty()) {
        std::wstring deno = util::PathJoin(home, L".deno\\bin");
        if (util::DirExists(deno)) dirs.push_back(deno);
    }
    for (const wchar_t* p : {L"C:\\Program Files\\ffmpeg\\bin",
                             L"C:\\ffmpeg\\bin",
                             L"C:\\Program Files (x86)\\ffmpeg\\bin"}) {
        if (util::DirExists(p)) dirs.push_back(p);
    }
    dirs.push_back(util::ExeDir());  // 允许把 ffmpeg.exe 直接丢在程序目录
    return dirs;
}

std::wstring FindOnPath(const std::wstring& exe) {
    // 先看我们额外注入的目录，再看系统 PATH
    for (auto& d : ExtraPathDirs()) {
        std::wstring p = util::PathJoin(d, exe);
        if (util::FileExists(p)) return p;
    }
    wchar_t buf[MAX_PATH * 2] = {};
    if (SearchPathW(nullptr, exe.c_str(), nullptr, (DWORD)std::size(buf), buf, nullptr))
        return buf;
    return {};
}

}  // namespace

std::wstring BuildChildEnv() {
    std::map<std::wstring, std::wstring> env;  // 键用大写，避免 Path/PATH 重复

    LPWCH block = GetEnvironmentStringsW();
    if (block) {
        for (LPWCH p = block; *p; p += wcslen(p) + 1) {
            std::wstring entry(p);
            // "=C:=C:\..." 这种驱动器当前目录条目开头就是 '='，跳过
            size_t eq = entry.find(L'=', 1);
            if (eq == std::wstring::npos) continue;
            env[util::ToLower(entry.substr(0, eq))] = entry.substr(eq + 1);
        }
        FreeEnvironmentStringsW(block);
    }

    std::wstring prefix;
    for (auto& d : ExtraPathDirs()) {
        if (!prefix.empty()) prefix += L';';
        prefix += d;
    }
    std::wstring oldPath = env.count(L"path") ? env[L"path"] : L"";
    env[L"path"] = prefix.empty() ? oldPath
                                  : (oldPath.empty() ? prefix : prefix + L";" + oldPath);

    // yt-dlp 是 PyInstaller 打包的，这两个变量能保证它往管道里写 UTF-8
    env[L"pythonutf8"] = L"1";
    env[L"pythonioencoding"] = L"utf-8";

    std::wstring out;
    for (auto& [k, v] : env) {
        out += k;
        out += L'=';
        out += v;
        out += L'\0';
    }
    out += L'\0';
    return out;
}

ToolStatus DetectTools() {
    ToolStatus s;
    s.denoPath = FindOnPath(L"deno.exe");
    s.hasDeno = !s.denoPath.empty();
    s.ffmpegPath = FindOnPath(L"ffmpeg.exe");
    s.hasFfmpeg = !s.ffmpegPath.empty();
    return s;
}

// ---------------------------------------------------------------- Runner

Runner::~Runner() {
    Kill();
    if (hThread_) {
        WaitForSingleObject(hThread_, 3000);
        CloseHandle(hThread_);
        hThread_ = nullptr;
    }
    CloseAll();
}

void Runner::CloseAll() {
    if (hRead_)  { CloseHandle(hRead_);  hRead_ = nullptr; }
    if (hProc_)  { CloseHandle(hProc_);  hProc_ = nullptr; }
    if (job_)    { CloseHandle(job_);    job_ = nullptr; }
}

bool Runner::Start(const std::vector<std::wstring>& args, const std::wstring& cwd,
                   HWND notify, UINT msgLines, UINT msgDone) {
    if (running_.load()) return false;

    // 上一轮的线程句柄要先收掉
    if (hThread_) {
        WaitForSingleObject(hThread_, 3000);
        CloseHandle(hThread_);
        hThread_ = nullptr;
    }
    CloseAll();
    killed_.store(false);

    notify_ = notify;
    msgLines_ = msgLines;
    msgDone_ = msgDone;
    cmdline_ = util::JoinCommandLine(args);

    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE wr = nullptr;
    if (!CreatePipe(&hRead_, &wr, &sa, 0)) return false;
    // 读端不能被子进程继承，否则子进程退出后管道不会 EOF
    SetHandleInformation(hRead_, HANDLE_FLAG_INHERIT, 0);

    // 给子进程一个空 stdin。不给的话 yt-dlp 某些分支会卡在等输入。
    HANDLE nul = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &sa, OPEN_EXISTING, 0, nullptr);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = wr;
    si.hStdInput = nul;

    // Job Object：关掉 job 就能连 ffmpeg 一起带走，比 taskkill 可靠
    job_ = CreateJobObjectW(nullptr, nullptr);
    if (job_) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION li{};
        li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job_, JobObjectExtendedLimitInformation, &li, sizeof(li));
    }

    std::wstring env = BuildChildEnv();
    std::wstring cl = cmdline_;  // CreateProcessW 会改写这个缓冲，必须可写
    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        nullptr, cl.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED,
        (LPVOID)env.data(), cwd.empty() ? nullptr : cwd.c_str(), &si, &pi);

    CloseHandle(wr);  // 父进程这边必须关掉写端，不然永远等不到 EOF
    if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);

    if (!ok) {
        CloseHandle(hRead_);
        hRead_ = nullptr;
        if (job_) { CloseHandle(job_); job_ = nullptr; }
        return false;
    }

    if (job_) AssignProcessToJobObject(job_, pi.hProcess);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    hProc_ = pi.hProcess;
    running_.store(true);
    hThread_ = CreateThread(nullptr, 0, &Runner::ThreadProc, this, 0, nullptr);
    if (!hThread_) {
        Kill();
        running_.store(false);
        return false;
    }
    return true;
}

DWORD WINAPI Runner::ThreadProc(LPVOID self) {
    static_cast<Runner*>(self)->Pump();
    return 0;
}

void Runner::Pump() {
    std::string pending;                 // 不完整的尾行留到下一轮
    std::vector<char> buf(kReadChunk);

    for (;;) {
        DWORD got = 0;
        if (!ReadFile(hRead_, buf.data(), (DWORD)buf.size(), &got, nullptr) || got == 0)
            break;
        pending.append(buf.data(), got);

        auto* batch = new std::vector<std::wstring>();
        size_t start = 0;
        for (size_t i = 0; i < pending.size(); ++i) {
            if (pending[i] != '\n') continue;
            size_t end = i;
            if (end > start && pending[end - 1] == '\r') --end;
            batch->push_back(util::DecodeConsole(pending.substr(start, end - start)));
            start = i + 1;
        }
        pending.erase(0, start);

        // 一次 ReadFile 的内容打包成一条消息，避免进度行把消息队列淹了
        if (!batch->empty() && notify_) PostMessageW(notify_, msgLines_, 0, (LPARAM)batch);
        else delete batch;
    }

    if (!pending.empty() && notify_) {
        auto* batch = new std::vector<std::wstring>{util::DecodeConsole(pending)};
        PostMessageW(notify_, msgLines_, 0, (LPARAM)batch);
    }

    DWORD code = 0;
    if (hProc_) {
        WaitForSingleObject(hProc_, INFINITE);
        GetExitCodeProcess(hProc_, &code);
    }
    running_.store(false);
    if (notify_) PostMessageW(notify_, msgDone_, (WPARAM)code, 0);
}

void Runner::Kill() {
    if (!running_.load()) return;
    killed_.store(true);
    if (job_) TerminateJobObject(job_, 1);
    else if (hProc_) TerminateProcess(hProc_, 1);
}

// ---------------------------------------------------------------- RunCapture

bool RunCapture(const std::vector<std::wstring>& args, const std::wstring& cwd,
                std::string& out, DWORD timeoutMs, DWORD* exitCode) {
    out.clear();

    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE rd = nullptr, wr = nullptr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return false;
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    HANDLE nul = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &sa, OPEN_EXISTING, 0, nullptr);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = wr;
    si.hStdInput = nul;

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION li{};
        li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &li, sizeof(li));
    }

    std::wstring env = BuildChildEnv();
    std::wstring cl = util::JoinCommandLine(args);
    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        nullptr, cl.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED,
        (LPVOID)env.data(), cwd.empty() ? nullptr : cwd.c_str(), &si, &pi);

    CloseHandle(wr);
    if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
    if (!ok) {
        CloseHandle(rd);
        if (job) CloseHandle(job);
        return false;
    }
    if (job) AssignProcessToJobObject(job, pi.hProcess);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    DWORD deadline = GetTickCount() + timeoutMs;
    char buf[16384];
    bool timedOut = false;
    for (;;) {
        // 管道读是阻塞的，先用 PeekNamedPipe 确认有数据，才能兼顾超时
        DWORD avail = 0;
        if (!PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr)) break;
        if (avail == 0) {
            if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {
                // 进程已退出，把管道里剩下的读干净
                DWORD left = 0;
                if (PeekNamedPipe(rd, nullptr, 0, nullptr, &left, nullptr) && left == 0) break;
            }
            if (GetTickCount() > deadline) { timedOut = true; break; }
            Sleep(20);
            continue;
        }
        DWORD got = 0;
        if (!ReadFile(rd, buf, sizeof(buf), &got, nullptr) || got == 0) break;
        out.append(buf, got);
        if (GetTickCount() > deadline) { timedOut = true; break; }
    }

    if (timedOut && job) TerminateJobObject(job, 1);
    WaitForSingleObject(pi.hProcess, timedOut ? 1000 : 5000);
    if (exitCode) GetExitCodeProcess(pi.hProcess, exitCode);

    CloseHandle(rd);
    CloseHandle(pi.hProcess);
    if (job) CloseHandle(job);
    return !timedOut;
}

}  // namespace proc
