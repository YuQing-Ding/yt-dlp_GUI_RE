// proc.h —— 子进程启动、管道读取、整棵进程树终止
#pragma once

#include "util.h"

#include <atomic>
#include <string>
#include <vector>

namespace proc {

// 给子进程用的环境块：在 PATH 前面插上 Deno / ffmpeg 的常见安装位置，
// 再强制 Python 侧用 UTF-8，这样管道里读到的就一定是 UTF-8。
std::wstring BuildChildEnv();

// Deno / ffmpeg 的探测结果（UI 上要显示状态）
struct ToolStatus {
    bool hasDeno = false;
    bool hasFfmpeg = false;
    std::wstring denoPath;
    std::wstring ffmpegPath;
};
ToolStatus DetectTools();

// 异步跑一个进程，输出按行投递到窗口。
//   msgLines: WPARAM=0, LPARAM=new std::vector<std::wstring>*（接收方负责 delete）
//   msgDone : WPARAM=退出码, LPARAM=0
class Runner {
public:
    Runner() = default;
    ~Runner();
    Runner(const Runner&) = delete;
    Runner& operator=(const Runner&) = delete;

    bool Start(const std::vector<std::wstring>& args, const std::wstring& cwd,
               HWND notify, UINT msgLines, UINT msgDone);

    // 终止整棵进程树（yt-dlp 会拉起 ffmpeg，只杀父进程会留下孤儿）
    void Kill();

    bool Running() const { return running_.load(); }
    bool KillRequested() const { return killed_.load(); }
    const std::wstring& LastCommandLine() const { return cmdline_; }

private:
    static DWORD WINAPI ThreadProc(LPVOID self);
    void Pump();

    HANDLE job_ = nullptr;
    HANDLE hProc_ = nullptr;
    HANDLE hRead_ = nullptr;
    HANDLE hThread_ = nullptr;
    HWND   notify_ = nullptr;
    UINT   msgLines_ = 0, msgDone_ = 0;
    std::wstring cmdline_;
    std::atomic<bool> running_{false};
    std::atomic<bool> killed_{false};

    void CloseAll();
};

// 同步跑完并收集全部输出。用于 --version 探测和 -J 拉元数据。
// 返回 false 表示进程没能启动或超时。exitCode 可为空。
bool RunCapture(const std::vector<std::wstring>& args, const std::wstring& cwd,
                std::string& out, DWORD timeoutMs, DWORD* exitCode = nullptr);

}  // namespace proc
