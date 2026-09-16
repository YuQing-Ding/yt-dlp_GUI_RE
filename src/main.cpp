// main.cpp —— 窗口、布局、事件
#include "config.h"
#include "fmtpicker.h"
#include "i18n.h"
#include "proc.h"
#include "resource.h"
#include "theme.h"
#include "util.h"

#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <shlobj.h>
#include <shellapi.h>
#include <urlmon.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <algorithm>
#include <memory>
#include <regex>
#include <string>
#include <vector>

// 视觉样式的 Common-Controls v6 依赖声明在 res/app.manifest 里，
// 不要在这里再写 #pragma manifestdependency，会和资源里的清单打架。

// ---------------------------------------------------------------- 常量

namespace {

constexpr UINT WM_APP_LINES      = WM_APP + 1;  // LPARAM = vector<wstring>*
constexpr UINT WM_APP_DONE       = WM_APP + 2;  // WPARAM = 退出码
constexpr UINT WM_APP_PROBEDONE  = WM_APP + 3;  // LPARAM = ProbeResult*
constexpr UINT WM_APP_TOOLS      = WM_APP + 4;  // 工具探测完成
constexpr UINT WM_APP_GETDONE    = WM_APP + 5;  // yt-dlp 下载完成，WPARAM=成功

const wchar_t* kMainClass = L"YtDlpGuiMain";
inline const wchar_t* AppTitle() { return i18n::T(S_APP_TITLE); }

// 标签页文案。切换语言时要整组重取，所以做成函数。
inline std::vector<std::wstring> TabLabels() {
    return {i18n::T(S_TAB_COOKIES), i18n::T(S_TAB_SUBS), i18n::T(S_TAB_NETWORK),
            i18n::T(S_TAB_PLAYLIST), i18n::T(S_TAB_ADVANCED)};
}

// 通用对话框的过滤器是用 '\0' 分段、'\0\0' 收尾的，没法写成普通字面量，
// 只能按当前语言在运行时拼。
inline std::wstring MakeFilter(StrId desc, const wchar_t* pattern) {
    std::wstring f;
    f += i18n::T(desc);
    f += L'\0';
    f += pattern;
    f += L'\0';
    f += i18n::T(S_FILTER_ALL);
    f += L'\0';
    f += L"*.*";
    f += L'\0';
    f += L'\0';
    return f;
}

// 日志超过这个字符数就掐掉前半截，免得跑一整天把内存吃光
constexpr int kLogLimit = 2 * 1024 * 1024;

// ---------------------------------------------------------------- 全局状态

HINSTANCE g_inst = nullptr;
HWND      g_main = nullptr;
Settings  g_cfg;
proc::Runner g_runner;
proc::ToolStatus g_tools;

std::wstring g_ytDlpPath;       // 实际可用的 yt-dlp
std::wstring g_ytDlpVersion;
std::wstring g_configPath;
bool g_busy = false;            // 正在下载
bool g_probing = false;

// 进度状态
struct RunState {
    double pct = 0;
    std::wstring speed, eta, file, size;
    int item = 0, total = 0;
    int errors = 0, completed = 0;
} g_run;

struct UI {
    // 顶栏
    HWND lang;
    // 链接
    HWND url, btnPaste, btnClearUrl, btnProbe, lblUrlCount;
    // 输出
    HWND lblOut, outDir, btnBrowse, btnOpenDir;
    HWND lblTmpl, tmplPreset, tmpl;
    // 画质
    HWND lblQuality, quality, lblResCap, resCap, lblContainer, container, recode;
    HWND audioOnly, aFormat, aQuality, useCustomFmt, customFmt;
    // 标签页
    HWND tabs;
    // cookies
    HWND ckMode, ckPath, ckPick, ckBrowser, ckProfile, lblCkHint;
    // 字幕
    HWND subWrite, subAuto, subEmbed, subSrt, lblSubLangs, subLangs;
    HWND embThumb, embMeta, embChap, sponsor, sponsorCats;
    // 网络
    HWND lblProxy, proxy, lblRate, rate, lblFrags, frags, lblRetries, retries, polite;
    // 播放列表
    HWND lblPlItems, plItems, plFolder, archive, ignErr, noOver;
    // 高级
    HWND lblExtra, extra, lblYtDlp, ytDlpPath, btnGetYtDlp, btnUpdateYtDlp, btnShowCmd;
    // 运行
    HWND btnStart, btnStop, progress, lblStatus;
    // 日志
    HWND log, autoScroll, btnCopyLog, btnClearLog, btnSaveLog;
} ui{};

std::vector<HWND> g_pages[5];
RECT g_cardRects[4]{};
int  g_langBoxW = 0;   // 顶栏语言下拉框的实际宽度，画徽章时要避开它
std::wstring g_cardTitles[4];  // 由 ApplyLanguage() 填充
RECT g_headerRect{};

// ---------------------------------------------------------------- 小工具

std::wstring GetText(HWND h) {
    int n = GetWindowTextLengthW(h);
    if (n <= 0) return {};
    std::wstring s((size_t)n, L'\0');
    GetWindowTextW(h, s.data(), n + 1);
    return s;
}

bool Checked(HWND h) { return Button_GetCheck(h) == BST_CHECKED; }
void SetCheck(HWND h, bool v) { Button_SetCheck(h, v ? BST_CHECKED : BST_UNCHECKED); }

std::vector<std::wstring> UrlList() {
    std::vector<std::wstring> out;
    for (auto& line : util::SplitLines(GetText(ui.url))) {
        std::wstring t = util::Trim(line);
        if (!t.empty() && t[0] != L'#') out.push_back(t);
    }
    return out;
}

// ---------------------------------------------------------------- 日志

void LogRaw(const std::wstring& text, COLORREF color) {
    // 快到上限就砍掉前 1/3，保留最近的内容
    if (GetWindowTextLengthW(ui.log) > kLogLimit) {
        CHARRANGE cut{0, kLogLimit / 3};
        SendMessageW(ui.log, EM_EXSETSEL, 0, (LPARAM)&cut);
        SendMessageW(ui.log, EM_REPLACESEL, FALSE, (LPARAM)L"");
    }

    CHARRANGE end{-1, -1};
    SendMessageW(ui.log, EM_EXSETSEL, 0, (LPARAM)&end);

    CHARFORMAT2W cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    SendMessageW(ui.log, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    std::wstring line = text + L"\r\n";
    SendMessageW(ui.log, EM_REPLACESEL, FALSE, (LPARAM)line.c_str());
}

void LogLine(const std::wstring& text, COLORREF color) {
    LogRaw(text, color);
    if (Checked(ui.autoScroll)) SendMessageW(ui.log, WM_VSCROLL, SB_BOTTOM, 0);
}

// 日志一律白字，只有警告和错误上色 —— 这两种颜色是有功能意义的，
// 出问题时要能一眼扫到是哪几行。
void LogInfo(const std::wstring& s)  { LogLine(s, theme::P.text); }
void LogOk(const std::wstring& s)    { LogLine(s, theme::P.text); }
void LogWarn(const std::wstring& s)  { LogLine(s, theme::P.warn); }
void LogErr(const std::wstring& s)   { LogLine(s, theme::P.err); }
void LogPlain(const std::wstring& s) { LogLine(s, theme::P.text); }

// ---------------------------------------------------------------- yt-dlp 探测

// 依次试各个候选，真正跑一遍 --version 才算数。
// 这样能自动绕开残缺的 onedir 版 yt-dlp.exe（缺 _internal 目录，一跑就报错）。
void DetectYtDlpWorker() {
    std::wstring dir = util::ExeDir();
    std::vector<std::wstring> cands;
    // 用户在「高级」里手填的优先
    std::wstring manual = util::Trim(g_cfg.extraArgs);  // 占位，实际值下面覆盖
    for (const wchar_t* n : {L"yt-dlp.exe", L"yt-dlp_x64.exe", L"yt-dlp_x86.exe",
                             L"yt-dlp_min.exe"}) {
        std::wstring p = util::PathJoin(dir, n);
        if (util::FileExists(p)) cands.push_back(p);
    }
    cands.push_back(L"yt-dlp.exe");  // 交给 PATH 去找

    for (auto& c : cands) {
        std::string out;
        DWORD code = 1;
        if (!proc::RunCapture({c, L"--version"}, dir, out, 8000, &code)) continue;
        if (code != 0) continue;
        std::wstring v = util::Trim(util::DecodeConsole(out));
        if (v.empty() || v.find(L'\n') != std::wstring::npos) {
            // 版本号应该就一行；多行说明是报错信息
            v = util::Trim(util::SplitLines(v).empty() ? L"" : util::SplitLines(v).back());
        }
        if (v.empty()) continue;
        g_ytDlpPath = c;
        g_ytDlpVersion = v;
        break;
    }
    g_tools = proc::DetectTools();
    PostMessageW(g_main, WM_APP_TOOLS, 0, 0);
}

DWORD WINAPI DetectThread(LPVOID) {
    DetectYtDlpWorker();
    return 0;
}

// ---------------------------------------------------------------- 设置同步

void ComboFill(HWND c, const NamedValue* items, int n, int sel) {
    ComboBox_ResetContent(c);
    for (int i = 0; i < n; ++i) ComboBox_AddString(c, NVLabel(items[i]));
    ComboBox_SetCurSel(c, sel);
}

void UiFromSettings() {
    SetWindowTextW(ui.outDir, g_cfg.outDir.c_str());
    SetWindowTextW(ui.tmpl, g_cfg.outputTemplate.c_str());

    ComboBox_SetCurSel(ui.quality, g_cfg.qualityPreset);
    ComboBox_SetCurSel(ui.resCap, g_cfg.resCap);
    ComboBox_SetCurSel(ui.container, g_cfg.container);
    SetCheck(ui.recode, g_cfg.recode);

    SetCheck(ui.audioOnly, g_cfg.audioOnly);
    ComboBox_SetCurSel(ui.aFormat, g_cfg.audioFormat);
    ComboBox_SetCurSel(ui.aQuality, g_cfg.audioQuality);
    SetCheck(ui.useCustomFmt, g_cfg.useCustomFormat);
    SetWindowTextW(ui.customFmt, g_cfg.customFormat.c_str());

    ComboBox_SetCurSel(ui.ckMode, g_cfg.cookieMode);
    SetWindowTextW(ui.ckPath, g_cfg.cookieFile.c_str());
    ComboBox_SetCurSel(ui.ckBrowser, g_cfg.browserIndex);
    SetWindowTextW(ui.ckProfile, g_cfg.browserProfile.c_str());

    SetCheck(ui.subWrite, g_cfg.writeSubs);
    SetCheck(ui.subAuto, g_cfg.autoSubs);
    SetCheck(ui.subEmbed, g_cfg.embedSubs);
    SetCheck(ui.subSrt, g_cfg.convertSrt);
    // 没设过就按界面语言给个合理默认（日文界面默认下日文字幕，以此类推）
    SetWindowTextW(ui.subLangs, g_cfg.subLangs.empty() ? i18n::T(S_DEFAULT_SUBLANGS)
                                                       : g_cfg.subLangs.c_str());
    SetCheck(ui.embThumb, g_cfg.embedThumb);
    SetCheck(ui.embMeta, g_cfg.embedMeta);
    SetCheck(ui.embChap, g_cfg.embedChapters);
    SetCheck(ui.sponsor, g_cfg.sponsorBlock);
    SetWindowTextW(ui.sponsorCats, g_cfg.sponsorCats.c_str());

    SetWindowTextW(ui.proxy, g_cfg.proxy.c_str());
    SetWindowTextW(ui.rate, g_cfg.rateLimit.c_str());
    SetWindowTextW(ui.frags, std::to_wstring(g_cfg.concurrentFrags).c_str());
    SetWindowTextW(ui.retries, std::to_wstring(g_cfg.retries).c_str());
    SetCheck(ui.polite, g_cfg.politeMode);

    SetWindowTextW(ui.plItems, g_cfg.playlistItems.c_str());
    SetCheck(ui.plFolder, g_cfg.playlistFolder);
    SetCheck(ui.archive, g_cfg.useArchive);
    SetCheck(ui.ignErr, g_cfg.ignoreErrors);
    SetCheck(ui.noOver, g_cfg.noOverwrites);

    SetWindowTextW(ui.extra, g_cfg.extraArgs.c_str());
    SetCheck(ui.autoScroll, g_cfg.autoScroll);
}

int ParseIntOr(const std::wstring& s, int def) {
    try {
        return std::stoi(util::Trim(s));
    } catch (...) {
        return def;
    }
}

void SettingsFromUi() {
    g_cfg.outDir = util::Trim(GetText(ui.outDir));
    g_cfg.outputTemplate = util::Trim(GetText(ui.tmpl));

    g_cfg.qualityPreset = (std::max)(0, ComboBox_GetCurSel(ui.quality));
    g_cfg.resCap = (std::max)(0, ComboBox_GetCurSel(ui.resCap));
    g_cfg.container = (std::max)(0, ComboBox_GetCurSel(ui.container));
    g_cfg.recode = Checked(ui.recode);

    g_cfg.audioOnly = Checked(ui.audioOnly);
    g_cfg.audioFormat = (std::max)(0, ComboBox_GetCurSel(ui.aFormat));
    g_cfg.audioQuality = (std::max)(0, ComboBox_GetCurSel(ui.aQuality));
    g_cfg.useCustomFormat = Checked(ui.useCustomFmt);
    g_cfg.customFormat = util::Trim(GetText(ui.customFmt));

    g_cfg.cookieMode = (std::max)(0, ComboBox_GetCurSel(ui.ckMode));
    g_cfg.cookieFile = util::Trim(GetText(ui.ckPath));
    g_cfg.browserIndex = (std::max)(0, ComboBox_GetCurSel(ui.ckBrowser));
    g_cfg.browserProfile = util::Trim(GetText(ui.ckProfile));

    g_cfg.writeSubs = Checked(ui.subWrite);
    g_cfg.autoSubs = Checked(ui.subAuto);
    g_cfg.embedSubs = Checked(ui.subEmbed);
    g_cfg.convertSrt = Checked(ui.subSrt);
    g_cfg.subLangs = util::Trim(GetText(ui.subLangs));
    g_cfg.embedThumb = Checked(ui.embThumb);
    g_cfg.embedMeta = Checked(ui.embMeta);
    g_cfg.embedChapters = Checked(ui.embChap);
    g_cfg.sponsorBlock = Checked(ui.sponsor);
    g_cfg.sponsorCats = util::Trim(GetText(ui.sponsorCats));

    g_cfg.proxy = util::Trim(GetText(ui.proxy));
    g_cfg.rateLimit = util::Trim(GetText(ui.rate));
    g_cfg.concurrentFrags = (std::max)(1, (std::min)(32, ParseIntOr(GetText(ui.frags), 4)));
    g_cfg.retries = (std::max)(0, (std::min)(100, ParseIntOr(GetText(ui.retries), 10)));
    g_cfg.politeMode = Checked(ui.polite);

    g_cfg.playlistItems = util::Trim(GetText(ui.plItems));
    g_cfg.playlistFolder = Checked(ui.plFolder);
    g_cfg.useArchive = Checked(ui.archive);
    g_cfg.ignoreErrors = Checked(ui.ignErr);
    g_cfg.noOverwrites = Checked(ui.noOver);

    g_cfg.extraArgs = util::Trim(GetText(ui.extra));
    g_cfg.autoScroll = Checked(ui.autoScroll);
}

// 仅音频 / 自定义格式 打开时，把互斥的控件灰掉
void SyncEnabled() {
    bool audio = Checked(ui.audioOnly);
    bool custom = Checked(ui.useCustomFmt);
    bool videoOpts = !audio && !custom;

    EnableWindow(ui.quality, videoOpts);
    EnableWindow(ui.resCap, videoOpts);
    EnableWindow(ui.container, videoOpts);
    EnableWindow(ui.recode, videoOpts);
    EnableWindow(ui.aFormat, audio && !custom);
    EnableWindow(ui.aQuality, audio && !custom);
    EnableWindow(ui.customFmt, custom);
    EnableWindow(ui.audioOnly, !custom);

    int ck = ComboBox_GetCurSel(ui.ckMode);
    EnableWindow(ui.ckPath, ck == 1);
    EnableWindow(ui.ckPick, ck == 1);
    EnableWindow(ui.ckBrowser, ck == 2);
    EnableWindow(ui.ckProfile, ck == 2);

    bool subs = Checked(ui.subWrite) || Checked(ui.subAuto);
    EnableWindow(ui.subEmbed, subs);
    EnableWindow(ui.subSrt, subs);
    EnableWindow(ui.subLangs, subs);
    EnableWindow(ui.sponsorCats, Checked(ui.sponsor));
}

void UpdateUrlCount() {
    size_t n = UrlList().size();
    SetWindowTextW(ui.lblUrlCount,
                   n ? util::Format(i18n::T(S_URL_COUNT), n).c_str() : L"");
}

// ---------------------------------------------------------------- 进度解析

const std::wregex kRePct(LR"(\[download\]\s+([0-9]+(?:\.[0-9]+)?)%)");
const std::wregex kReSpeed(LR"(at\s+([0-9.]+\s*[KMG]?i?B/s))");
const std::wregex kReEta(LR"(ETA\s+([0-9:]+))");
const std::wregex kReOf(LR"(of\s+~?\s*([0-9.]+\s*[KMG]?i?B))");
const std::wregex kReItem(LR"(Downloading item (\d+) of (\d+))");
const std::wregex kReDest(LR"(\[download\] Destination:\s*(.+))");
const std::wregex kReAlready(LR"(\[download\]\s+(.+) has already been downloaded)");

void RefreshProgressUi() {
    std::wstring bar;
    if (g_run.pct >= 0) bar = util::Format(L"%.1f%%", g_run.pct);
    if (!g_run.speed.empty()) bar += L"   " + g_run.speed;
    if (!g_run.eta.empty()) bar += L"   " + std::wstring(i18n::T(S_STATUS_ETA)) + g_run.eta;
    theme::SetProgress(ui.progress, g_run.pct, bar);

    std::wstring st;
    if (g_run.total > 0) st = util::Format(L"[%d/%d] ", g_run.item, g_run.total);
    if (!g_run.file.empty()) st += g_run.file;
    if (!g_run.size.empty()) st += L"   " + g_run.size;
    if (g_run.errors > 0) st += util::Format(i18n::T(S_STATUS_FAILEDN), g_run.errors);
    SetWindowTextW(ui.lblStatus, st.c_str());
}

// 返回 true 表示这行是纯进度刷新，不必写进日志
bool HandleLine(const std::wstring& line) {
    std::wsmatch m;

    if (std::regex_search(line, m, kRePct)) {
        g_run.pct = _wtof(m[1].str().c_str());
        g_run.speed.clear();
        g_run.eta.clear();
        if (std::regex_search(line, m, kReSpeed)) g_run.speed = m[1].str();
        if (std::regex_search(line, m, kReEta)) g_run.eta = m[1].str();
        if (std::regex_search(line, m, kReOf)) g_run.size = m[1].str();
        RefreshProgressUi();
        return true;
    }
    if (std::regex_search(line, m, kReItem)) {
        g_run.item = _wtoi(m[1].str().c_str());
        g_run.total = _wtoi(m[2].str().c_str());
        g_run.pct = 0;
        RefreshProgressUi();
        return false;
    }
    if (std::regex_search(line, m, kReDest)) {
        std::wstring p = m[1].str();
        size_t s = p.find_last_of(L"\\/");
        g_run.file = (s == std::wstring::npos) ? p : p.substr(s + 1);
        g_run.pct = 0;
        RefreshProgressUi();
        return false;
    }
    if (std::regex_search(line, m, kReAlready)) {
        g_run.completed++;
        return false;
    }
    return false;
}

COLORREF ClassifyLine(const std::wstring& line) {
    std::wstring low = util::ToLower(line);
    if (util::StartsWith(low, L"error") || util::Contains(low, L" error:")) {
        g_run.errors++;
        return theme::P.err;
    }
    if (util::StartsWith(low, L"warning")) return theme::P.warn;
    return theme::P.text;   // 其余一律白字
}

// ---------------------------------------------------------------- 下载

void SetBusy(bool busy) {
    g_busy = busy;
    EnableWindow(ui.btnStart, !busy);
    EnableWindow(ui.btnStop, busy);
    EnableWindow(ui.btnProbe, !busy);
    EnableWindow(ui.btnGetYtDlp, !busy);
    EnableWindow(ui.btnUpdateYtDlp, !busy);
}

bool EnsureReady() {
    if (g_ytDlpPath.empty()) {
        MessageBoxW(g_main, i18n::T(S_MSG_NOYTDLP), AppTitle(), MB_ICONWARNING);
        return false;
    }
    return true;
}

void StartDownload() {
    if (g_busy || !EnsureReady()) return;

    auto urls = UrlList();
    if (urls.empty()) {
        MessageBoxW(g_main, i18n::T(S_MSG_NOURL), AppTitle(),
                    MB_ICONINFORMATION);
        SetFocus(ui.url);
        return;
    }

    SettingsFromUi();
    if (g_cfg.outDir.empty()) {
        MessageBoxW(g_main, i18n::T(S_MSG_NODIR), AppTitle(), MB_ICONWARNING);
        return;
    }
    if (!util::EnsureDir(g_cfg.outDir)) {
        MessageBoxW(g_main, (i18n::T(S_MSG_MKDIRFAIL) + g_cfg.outDir).c_str(), AppTitle(),
                    MB_ICONERROR);
        return;
    }
    if (g_cfg.cookieMode == 1 && !util::FileExists(g_cfg.cookieFile)) {
        MessageBoxW(g_main, i18n::T(S_MSG_NOCOOKIE), AppTitle(),
                    MB_ICONWARNING);
        return;
    }
    g_cfg.Save(g_configPath);

    auto args = g_cfg.BuildArgs(g_ytDlpPath);
    for (auto& u : urls) args.push_back(u);

    g_run = RunState{};
    g_run.total = (int)urls.size() > 1 ? (int)urls.size() : 0;
    SetWindowTextW(ui.lblStatus, i18n::T(S_STATUS_RESOLVING));
    theme::SetProgress(ui.progress, -1, L"");

    LogInfo(L"> " + util::JoinCommandLine(args));
    LogPlain(L"");

    if (!g_runner.Start(args, util::ExeDir(), g_main, WM_APP_LINES, WM_APP_DONE)) {
        LogErr(i18n::T(S_LOG_NOSTART));
        theme::SetProgress(ui.progress, 0, L"");
        return;
    }
    SetBusy(true);
}

// ---------------------------------------------------------------- 探测格式

struct ProbeCtx {
    std::wstring url;
    std::vector<std::wstring> args;
};

DWORD WINAPI ProbeThread(LPVOID p) {
    std::unique_ptr<ProbeCtx> ctx((ProbeCtx*)p);
    std::string out;
    DWORD code = 1;
    auto* pr = new ProbeResult();
    if (!proc::RunCapture(ctx->args, util::ExeDir(), out, 120000, &code)) {
        pr->error = i18n::T(S_FP_ERR_TIMEOUT);
    } else if (out.empty()) {
        pr->error = i18n::T(S_FP_ERR_NOOUTPUT);
    } else {
        *pr = ParseProbeJson(out);
        if (!pr->ok && pr->error.empty()) pr->error = i18n::T(S_FP_ERR_PARSE);
        if (!pr->ok) {
            // 把 yt-dlp 自己的报错带回去，比干巴巴一句"失败"有用
            std::wstring raw = util::Trim(util::DecodeConsole(out));
            auto lines = util::SplitLines(raw);
            for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
                if (util::StartsWith(util::ToLower(*it), L"error")) {
                    pr->error += L"\n\n" + *it;
                    break;
                }
            }
        }
    }
    PostMessageW(g_main, WM_APP_PROBEDONE, 0, (LPARAM)pr);
    return 0;
}

void StartProbe() {
    if (g_busy || g_probing || !EnsureReady()) return;
    auto urls = UrlList();
    if (urls.empty()) {
        MessageBoxW(g_main, i18n::T(S_MSG_NOURL_PROBE), AppTitle(), MB_ICONINFORMATION);
        return;
    }

    SettingsFromUi();
    auto* ctx = new ProbeCtx();
    ctx->url = urls[0];
    ctx->args = {g_ytDlpPath, L"-J", L"--no-playlist", L"--no-warnings", L"--ignore-config"};
    // cookie 设置要带上，否则一探测就撞 bot 检测
    if (g_cfg.cookieMode == 1 && util::FileExists(g_cfg.cookieFile)) {
        ctx->args.push_back(L"--cookies");
        ctx->args.push_back(g_cfg.cookieFile);
    } else if (g_cfg.cookieMode == 2) {
        std::wstring spec = BROWSERS[g_cfg.browserIndex].value;
        if (!g_cfg.browserProfile.empty()) spec += L":" + g_cfg.browserProfile;
        ctx->args.push_back(L"--cookies-from-browser");
        ctx->args.push_back(spec);
    }
    if (!g_cfg.proxy.empty()) {
        ctx->args.push_back(L"--proxy");
        ctx->args.push_back(g_cfg.proxy);
    }
    ctx->args.push_back(ctx->url);

    g_probing = true;
    EnableWindow(ui.btnProbe, FALSE);
    SetWindowTextW(ui.lblStatus, i18n::T(S_STATUS_PROBING));
    theme::SetProgress(ui.progress, -1, L"");
    LogInfo(i18n::T(S_LOG_PROBING) + ctx->url);

    HANDLE t = CreateThread(nullptr, 0, ProbeThread, ctx, 0, nullptr);
    if (t) CloseHandle(t);
    else {
        delete ctx;
        g_probing = false;
        EnableWindow(ui.btnProbe, TRUE);
    }
}

// ---------------------------------------------------------------- 下载 yt-dlp

DWORD WINAPI GetYtDlpThread(LPVOID) {
    std::wstring dst = util::PathJoin(util::ExeDir(), L"yt-dlp.exe");
    std::wstring tmp = dst + L".download";
    DeleteFileW(tmp.c_str());
    HRESULT hr = URLDownloadToFileW(
        nullptr,
        L"https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe",
        tmp.c_str(), 0, nullptr);
    bool ok = SUCCEEDED(hr);
    if (ok) {
        DeleteFileW(dst.c_str());
        ok = MoveFileW(tmp.c_str(), dst.c_str()) != 0;
    }
    DeleteFileW(tmp.c_str());
    PostMessageW(g_main, WM_APP_GETDONE, ok ? 1 : 0, 0);
    return 0;
}

void StartGetYtDlp() {
    if (g_busy) return;
    int r = MessageBoxW(
        g_main,
        i18n::T(S_MSG_GETCONFIRM), AppTitle(), MB_OKCANCEL | MB_ICONQUESTION);
    if (r != IDOK) return;

    SetBusy(true);
    SetWindowTextW(ui.lblStatus, i18n::T(S_STATUS_GETTING));
    theme::SetProgress(ui.progress, -1, L"");
    LogInfo(i18n::T(S_LOG_GETTING));
    HANDLE t = CreateThread(nullptr, 0, GetYtDlpThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
    else { SetBusy(false); LogErr(i18n::T(S_LOG_NOSTART)); }
}

// ---------------------------------------------------------------- 文件对话框

std::wstring PickFolder(HWND owner, const std::wstring& initial) {
    std::wstring result;
    IFileDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dlg))))
        return result;

    DWORD opts = 0;
    dlg->GetOptions(&opts);
    dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    if (!initial.empty()) {
        IShellItem* si = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(initial.c_str(), nullptr,
                                                  IID_PPV_ARGS(&si)))) {
            dlg->SetFolder(si);
            si->Release();
        }
    }
    if (SUCCEEDED(dlg->Show(owner))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                result = path;
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dlg->Release();
    return result;
}

std::wstring PickFile(HWND owner, const wchar_t* filter, bool save,
                      const wchar_t* defExt = nullptr) {
    wchar_t buf[MAX_PATH * 2] = {};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = (DWORD)std::size(buf);
    ofn.lpstrDefExt = defExt;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR |
                (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    return ok ? std::wstring(buf) : std::wstring();
}

void CopyLogToClipboard() {
    std::wstring text = GetText(ui.log);
    if (text.empty()) return;
    if (!OpenClipboard(g_main)) return;
    EmptyClipboard();
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (mem) {
        void* p = GlobalLock(mem);
        memcpy(p, text.c_str(), bytes);
        GlobalUnlock(mem);
        SetClipboardData(CF_UNICODETEXT, mem);
    }
    CloseClipboard();
}

void SaveLogToFile() {
    std::wstring filter = MakeFilter(S_FILTER_TXT, L"*.txt");
    std::wstring path = PickFile(g_main, filter.c_str(), true, L"txt");
    if (path.empty()) return;
    std::string utf8 = util::WToUtf8(GetText(ui.log));
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        MessageBoxW(g_main, i18n::T(S_MSG_WRITEFAIL), AppTitle(), MB_ICONERROR);
        return;
    }
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    DWORD w = 0;
    WriteFile(h, bom, 3, &w, nullptr);
    WriteFile(h, utf8.data(), (DWORD)utf8.size(), &w, nullptr);
    CloseHandle(h);
    LogOk(i18n::T(S_LOG_LOGSAVED) + path);
}

}  // namespace

// ---------------------------------------------------------------- 控件创建

namespace {

HWND MkStatic(HWND p, const wchar_t* text, theme::FontRole f = theme::FontRole::Body,
              DWORD extra = 0) {
    // SS_NOPREFIX：文案里的 "&" 要原样显示，不是快捷键标记
    HWND h = CreateWindowExW(0, L"STATIC", text,
                             WS_CHILD | WS_VISIBLE | SS_NOPREFIX | extra, 0, 0, 10, 10,
                             p, nullptr, g_inst, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)theme::Font(f), TRUE);
    return h;
}

HWND MkEdit(HWND p, int id, DWORD extra = 0) {
    HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | extra,
                             0, 0, 10, 10, p, (HMENU)(INT_PTR)id, g_inst, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Body), TRUE);
    SetWindowTheme(h, L"DarkMode_CFD", nullptr);
    return h;
}

HWND MkCombo(HWND p, int id) {
    HWND h = CreateWindowExW(0, L"COMBOBOX", L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                                 CBS_DROPDOWNLIST | CBS_HASSTRINGS,
                             0, 0, 10, 300, p, (HMENU)(INT_PTR)id, g_inst, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Body), TRUE);
    SetWindowTheme(h, L"DarkMode_CFD", nullptr);
    return h;
}

HWND MkCheck(HWND p, int id, const wchar_t* text) {
    HWND h = CreateWindowExW(0, L"BUTTON", text,
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                             0, 0, 10, 10, p, (HMENU)(INT_PTR)id, g_inst, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Body), TRUE);
    theme::ApplyDarkToWindow(h);
    return h;
}

void CreateControls(HWND p) {
    using theme::BtnStyle;
    using theme::CreateButton;

    // ---- 顶栏：语言选择 ----
    ui.lang = MkCombo(p, IDC_LANG);
    for (int i = 0; i < i18n::kLangCount; ++i)
        ComboBox_AddString(ui.lang, i18n::LangDisplayName(i18n::LangAtDisplayIndex(i)));
    ComboBox_SetCurSel(ui.lang, i18n::DisplayIndexOf(i18n::Current()));

    // ---- 链接 ----
    ui.url = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                                 ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                             0, 0, 10, 10, p, (HMENU)IDC_URL, g_inst, nullptr);
    SendMessageW(ui.url, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Body), TRUE);
    SetWindowTheme(ui.url, L"DarkMode_CFD", nullptr);
    SendMessageW(ui.url, EM_SETCUEBANNER, TRUE,
                 (LPARAM)i18n::T(S_URL_CUE));

    ui.btnPaste = CreateButton(p, IDC_PASTE, i18n::T(S_BTN_PASTE), BtnStyle::Secondary);
    ui.btnClearUrl = CreateButton(p, IDC_CLEARURL, i18n::T(S_BTN_CLEAR), BtnStyle::Ghost);
    ui.btnProbe = CreateButton(p, IDC_PROBE, i18n::T(S_BTN_PROBE), BtnStyle::Secondary);
    ui.lblUrlCount = MkStatic(p, L"", theme::FontRole::Small, SS_RIGHT);

    // ---- 输出 ----
    ui.lblOut = MkStatic(p, i18n::T(S_LBL_SAVETO));
    ui.outDir = MkEdit(p, IDC_OUTDIR);
    ui.btnBrowse = CreateButton(p, IDC_BROWSE, i18n::T(S_BTN_BROWSE), BtnStyle::Secondary);
    ui.btnOpenDir = CreateButton(p, IDC_OPENDIR, i18n::T(S_BTN_OPEN), BtnStyle::Ghost);

    ui.lblTmpl = MkStatic(p, i18n::T(S_LBL_FILENAME));
    ui.tmplPreset = MkCombo(p, IDC_TMPLPRESET);
    ComboFill(ui.tmplPreset, TEMPLATE_PRESETS, TEMPLATE_PRESET_COUNT, 0);
    ui.tmpl = MkEdit(p, IDC_TMPL);

    // ---- 画质 ----
    ui.lblQuality = MkStatic(p, i18n::T(S_LBL_QUALITY));
    ui.quality = MkCombo(p, IDC_QUALITY);
    ComboBox_ResetContent(ui.quality);
    for (int i = 0; i < QUALITY_PRESET_COUNT; ++i)
        ComboBox_AddString(ui.quality, i18n::T(QUALITY_PRESETS[i].label));

    ui.lblResCap = MkStatic(p, i18n::T(S_LBL_MAXRES));
    ui.resCap = MkCombo(p, IDC_RESCAP);
    ComboFill(ui.resCap, RES_CAPS, RES_CAP_COUNT, 0);

    ui.lblContainer = MkStatic(p, i18n::T(S_LBL_CONTAINER));
    ui.container = MkCombo(p, IDC_CONTAINER);
    ComboFill(ui.container, CONTAINERS, CONTAINER_COUNT, 0);
    ui.recode = MkCheck(p, IDC_RECODE, i18n::T(S_CHK_RECODE));

    ui.audioOnly = MkCheck(p, IDC_AUDIOONLY, i18n::T(S_CHK_AUDIOONLY));
    ui.aFormat = MkCombo(p, IDC_AFORMAT);
    ComboFill(ui.aFormat, AUDIO_FORMATS, AUDIO_FORMAT_COUNT, 0);
    ui.aQuality = MkCombo(p, IDC_AQUALITY);
    ComboBox_ResetContent(ui.aQuality);
    for (int i = 0; i <= 10; ++i) {
        std::wstring s = util::Format(i18n::T(S_AQ_FMT), i);
        if (i == 0) s += i18n::T(S_AQ_BEST);
        else if (i == 10) s += i18n::T(S_AQ_WORST);
        ComboBox_AddString(ui.aQuality, s.c_str());
    }
    ComboBox_SetCurSel(ui.aQuality, 0);

    ui.useCustomFmt = MkCheck(p, IDC_USECUSTOMFMT, i18n::T(S_CHK_CUSTOMFMT));
    ui.customFmt = MkEdit(p, IDC_CUSTOMFMT);
    SendMessageW(ui.customFmt, EM_SETCUEBANNER, TRUE,
                 (LPARAM)i18n::T(S_CUSTOMFMT_CUE));

    // ---- 标签页 ----
    ui.tabs = theme::CreateTabStrip(
        p, IDC_TABS, TabLabels());

    // 页 0：Cookies
    ui.ckMode = MkCombo(p, IDC_CKMODE);
    ComboBox_AddString(ui.ckMode, i18n::T(S_CK_NONE));
    ComboBox_AddString(ui.ckMode, i18n::T(S_CK_FILE));
    ComboBox_AddString(ui.ckMode, i18n::T(S_CK_BROWSER));
    ComboBox_SetCurSel(ui.ckMode, 0);
    ui.ckPath = MkEdit(p, IDC_CKPATH);
    ui.ckPick = CreateButton(p, IDC_CKPICK, i18n::T(S_CK_PICKFILE), BtnStyle::Secondary);
    ui.ckBrowser = MkCombo(p, IDC_CKBROWSER);
    ComboFill(ui.ckBrowser, BROWSERS, BROWSER_COUNT, 0);
    ui.ckProfile = MkEdit(p, IDC_CKPROFILE);
    SendMessageW(ui.ckProfile, EM_SETCUEBANNER, TRUE,
                 (LPARAM)i18n::T(S_CK_PROFILE_CUE));
    ui.lblCkHint = MkStatic(
        p,
        i18n::T(S_CK_HINT),
        theme::FontRole::Small);
    g_pages[0] = {ui.ckMode, ui.ckPath, ui.ckPick, ui.ckBrowser, ui.ckProfile, ui.lblCkHint};

    // 页 1：字幕与嵌入
    ui.subWrite = MkCheck(p, IDC_SUBWRITE, i18n::T(S_SUB_WRITE));
    ui.subAuto = MkCheck(p, IDC_SUBAUTO, i18n::T(S_SUB_AUTO));
    ui.subEmbed = MkCheck(p, IDC_SUBEMBED, i18n::T(S_SUB_EMBED));
    ui.subSrt = MkCheck(p, IDC_SUBSRT, i18n::T(S_SUB_SRT));
    ui.lblSubLangs = MkStatic(p, i18n::T(S_SUB_LANGS));
    ui.subLangs = MkEdit(p, IDC_SUBLANGS);
    ui.embThumb = MkCheck(p, IDC_EMBTHUMB, i18n::T(S_EMB_THUMB));
    ui.embMeta = MkCheck(p, IDC_EMBMETA, i18n::T(S_EMB_META));
    ui.embChap = MkCheck(p, IDC_EMBCHAP, i18n::T(S_EMB_CHAP));
    ui.sponsor = MkCheck(p, IDC_SPONSOR, i18n::T(S_SPONSOR));
    ui.sponsorCats = MkEdit(p, IDC_SPONSORCATS);
    g_pages[1] = {ui.subWrite, ui.subAuto,  ui.subEmbed, ui.subSrt,  ui.lblSubLangs,
                  ui.subLangs, ui.embThumb, ui.embMeta,  ui.embChap, ui.sponsor,
                  ui.sponsorCats};

    // 页 2：网络
    ui.lblProxy = MkStatic(p, i18n::T(S_NET_PROXY));
    ui.proxy = MkEdit(p, IDC_PROXY);
    SendMessageW(ui.proxy, EM_SETCUEBANNER, TRUE,
                 (LPARAM)i18n::T(S_NET_PROXY_CUE));
    ui.lblRate = MkStatic(p, i18n::T(S_NET_RATE));
    ui.rate = MkEdit(p, IDC_RATE);
    SendMessageW(ui.rate, EM_SETCUEBANNER, TRUE, (LPARAM)i18n::T(S_NET_RATE_CUE));
    ui.lblFrags = MkStatic(p, i18n::T(S_NET_FRAGS));
    ui.frags = MkEdit(p, IDC_FRAGS, ES_NUMBER);
    ui.lblRetries = MkStatic(p, i18n::T(S_NET_RETRIES));
    ui.retries = MkEdit(p, IDC_RETRIES, ES_NUMBER);
    ui.polite = MkCheck(p, IDC_POLITE, i18n::T(S_NET_POLITE));
    g_pages[2] = {ui.lblProxy,   ui.proxy,      ui.lblRate, ui.rate,
                  ui.lblFrags,   ui.frags,      ui.lblRetries, ui.retries,
                  ui.polite};

    // 页 3：播放列表
    ui.lblPlItems = MkStatic(p, i18n::T(S_PL_RANGE));
    ui.plItems = MkEdit(p, IDC_PLITEMS);
    SendMessageW(ui.plItems, EM_SETCUEBANNER, TRUE,
                 (LPARAM)i18n::T(S_PL_RANGE_CUE));
    ui.plFolder = MkCheck(p, IDC_PLFOLDER, i18n::T(S_PL_FOLDER));
    ui.archive = MkCheck(p, IDC_ARCHIVE, i18n::T(S_PL_ARCHIVE));
    ui.ignErr = MkCheck(p, IDC_IGNERR, i18n::T(S_PL_IGNERR));
    ui.noOver = MkCheck(p, IDC_NOOVER, i18n::T(S_PL_NOOVER));
    g_pages[3] = {ui.lblPlItems, ui.plItems, ui.plFolder, ui.archive, ui.ignErr, ui.noOver};

    // 页 4：高级
    ui.lblExtra = MkStatic(p, i18n::T(S_ADV_EXTRA));
    ui.extra = MkEdit(p, IDC_EXTRA);
    SendMessageW(ui.extra, EM_SETCUEBANNER, TRUE,
                 (LPARAM)i18n::T(S_ADV_EXTRA_CUE));
    ui.lblYtDlp = MkStatic(p, L"yt-dlp", theme::FontRole::Small);
    ui.ytDlpPath = MkStatic(p, i18n::T(S_ADV_DETECTING), theme::FontRole::Small);
    ui.btnGetYtDlp = CreateButton(p, IDC_GETYTDLP, i18n::T(S_ADV_GET), BtnStyle::Secondary);
    ui.btnUpdateYtDlp = CreateButton(p, IDC_UPDATEYTDLP, i18n::T(S_ADV_UPDATE), BtnStyle::Ghost);
    ui.btnShowCmd = CreateButton(p, IDC_SHOWCMD, i18n::T(S_ADV_SHOWCMD), BtnStyle::Ghost);
    g_pages[4] = {ui.lblExtra,    ui.extra,          ui.lblYtDlp,   ui.ytDlpPath,
                  ui.btnGetYtDlp, ui.btnUpdateYtDlp, ui.btnShowCmd};

    // ---- 运行 ----
    // 这两个直接坐在窗口底色上，不在卡片里
    ui.btnStart = CreateButton(p, IDC_START, i18n::T(S_BTN_START), BtnStyle::Primary, true);
    ui.btnStop = CreateButton(p, IDC_STOP, i18n::T(S_BTN_STOP), BtnStyle::Danger, true);
    EnableWindow(ui.btnStop, FALSE);
    ui.progress = theme::CreateProgress(p, IDC_PROGRESS);
    ui.lblStatus = MkStatic(p, L"", theme::FontRole::Small, SS_PATHELLIPSIS);

    // ---- 日志 ----
    ui.log = CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
                             WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE |
                                 ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
                             0, 0, 10, 10, p, (HMENU)IDC_LOG, g_inst, nullptr);
    SendMessageW(ui.log, EM_SETBKGNDCOLOR, 0, (LPARAM)theme::P.input);
    SendMessageW(ui.log, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Mono), TRUE);
    SendMessageW(ui.log, EM_EXLIMITTEXT, 0, (LPARAM)(kLogLimit * 2));
    SendMessageW(ui.log, EM_SETOPTIONS, ECOOP_OR, ECO_SELECTIONBAR);
    SetWindowTheme(ui.log, L"DarkMode_Explorer", nullptr);

    ui.autoScroll = MkCheck(p, IDC_AUTOSCROLL, i18n::T(S_CHK_AUTOSCROLL));
    ui.btnCopyLog = CreateButton(p, IDC_COPYLOG, i18n::T(S_BTN_COPY), BtnStyle::Ghost);
    ui.btnSaveLog = CreateButton(p, IDC_SAVELOG, i18n::T(S_BTN_SAVE), BtnStyle::Ghost);
    ui.btnClearLog = CreateButton(p, IDC_CLEARLOG, i18n::T(S_BTN_CLEAR), BtnStyle::Ghost);
}

// Cookies 页里「文件」和「浏览器」两组控件占同一行，必须按模式互斥显示，
// 否则两组会叠在一起。
void SyncCookieRow() {
    bool onPage = theme::TabStripSel(ui.tabs) == 0;
    int mode = ComboBox_GetCurSel(ui.ckMode);
    int fileShow = (onPage && mode == 1) ? SW_SHOW : SW_HIDE;
    int browserShow = (onPage && mode == 2) ? SW_SHOW : SW_HIDE;
    ShowWindow(ui.ckPath, fileShow);
    ShowWindow(ui.ckPick, fileShow);
    ShowWindow(ui.ckBrowser, browserShow);
    ShowWindow(ui.ckProfile, browserShow);
}

void ShowPage(int idx) {
    for (int i = 0; i < 5; ++i)
        for (HWND h : g_pages[i]) ShowWindow(h, i == idx ? SW_SHOW : SW_HIDE);
    SyncCookieRow();
}

// ---------------------------------------------------------------- 布局

// 五种语言的文字长度差得很远（"Re-encode" 比 "重编码" 宽一倍还多），
// 所以控件宽度一律按实际文字测量，不写死像素。

int MeasureText(HWND ref, const std::wstring& text, HFONT font) {
    if (text.empty()) return 0;
    HDC dc = GetDC(ref);
    HFONT old = (HFONT)SelectObject(dc, font);
    SIZE sz{};
    GetTextExtentPoint32W(dc, text.c_str(), (int)text.size(), &sz);
    SelectObject(dc, old);
    ReleaseDC(ref, dc);
    return sz.cx;
}

// 按钮：自绘时用的是 Bold，测量也得用 Bold
int BtnW(HWND b, int minPx) {
    int w = MeasureText(b, GetText(b), theme::Font(theme::FontRole::Bold));
    return (std::max)(theme::S(minPx), w + theme::S(26));
}

int LblW(HWND h) {
    return MeasureText(h, GetText(h), theme::Font(theme::FontRole::Body)) + theme::S(6);
}

// 复选框 = 方框 + 间距 + 文字
int ChkW(HWND h) {
    int w = MeasureText(h, GetText(h), theme::Font(theme::FontRole::Body));
    return w + theme::S(26);
}

// 下拉框按最宽的一项算，但夹在 [minPx, maxPx] 之间，免得被超长选项撑爆
int CboW(HWND c, int minPx, int maxPx) {
    HFONT f = theme::Font(theme::FontRole::Body);
    int w = 0;
    int n = ComboBox_GetCount(c);
    for (int i = 0; i < n; ++i) {
        int len = ComboBox_GetLBTextLen(c, i);
        if (len <= 0) continue;
        std::wstring s((size_t)len, L'\0');
        ComboBox_GetLBText(c, i, s.data());
        w = (std::max)(w, MeasureText(c, s, f));
    }
    return (std::max)(theme::S(minPx), (std::min)(theme::S(maxPx), w + theme::S(34)));
}

void Layout(HWND p) {
    RECT rc;
    GetClientRect(p, &rc);
    using theme::S;

    const int pad = S(16);
    const int cp = S(14);          // 卡片内边距
    const int gap = S(8);
    const int rh = S(28);          // 一行控件的高度
    const int rowGap = S(8);
    const int titleH = S(24);
    const int W = rc.right;
    int y = pad;

    // ---- 顶栏 ----
    g_headerRect = {pad, y, W - pad, y + S(44)};
    {
        int lw = CboW(ui.lang, 100, 200);
        MoveWindow(ui.lang, W - pad - lw, y + S(8), lw, S(260), TRUE);
        g_langBoxW = lw;
    }
    y += S(44) + gap;

    auto label = [&](HWND h, int x, int yy, int w) {
        MoveWindow(h, x, yy + S(5), w, rh - S(6), TRUE);
    };
    auto check = [&](HWND h, int x, int yy, int w) {
        MoveWindow(h, x, yy + S(4), w, rh - S(6), TRUE);
    };

    // ---- 卡片1：链接 ----
    int urlH = S(54);
    int card1H = titleH + urlH + rowGap + rh + cp;
    g_cardRects[0] = {pad, y, W - pad, y + card1H};
    {
        int cx = pad + cp, cw = W - pad * 2 - cp * 2;
        int cy = y + titleH;
        MoveWindow(ui.url, cx, cy, cw, urlH, TRUE);
        cy += urlH + rowGap;

        int x = cx;
        for (HWND b : {ui.btnPaste, ui.btnClearUrl}) {
            int w = BtnW(b, 70);
            MoveWindow(b, x, cy, w, rh, TRUE);
            x += w + gap;
        }
        int wp = BtnW(ui.btnProbe, 120);
        MoveWindow(ui.btnProbe, x, cy, wp, rh, TRUE);
        label(ui.lblUrlCount, cx + cw - S(140), cy, S(140));
    }
    y += card1H + gap;

    // ---- 卡片2：输出与画质 ----
    int card2H = titleH + rh * 4 + rowGap * 3 + cp;
    g_cardRects[1] = {pad, y, W - pad, y + card2H};
    {
        int cx = pad + cp, cw = W - pad * 2 - cp * 2;
        int cy = y + titleH;
        // 左列标签对齐，宽度取三行里最宽的
        const int lw = (std::max)({LblW(ui.lblOut), LblW(ui.lblTmpl), LblW(ui.lblQuality)});
        int fx = cx + lw + gap;
        int fw = cw - lw - gap;

        // 行1：保存目录
        label(ui.lblOut, cx, cy, lw);
        int bwB = BtnW(ui.btnBrowse, 70), bwO = BtnW(ui.btnOpenDir, 56);
        MoveWindow(ui.outDir, fx, cy, (std::max)(S(200), fw - bwB - bwO - gap * 2), rh, TRUE);
        MoveWindow(ui.btnBrowse, cx + cw - bwB - bwO - gap, cy, bwB, rh, TRUE);
        MoveWindow(ui.btnOpenDir, cx + cw - bwO, cy, bwO, rh, TRUE);
        cy += rh + rowGap;

        // 行2：命名模板
        label(ui.lblTmpl, cx, cy, lw);
        int pw = CboW(ui.tmplPreset, 150, 260);
        MoveWindow(ui.tmplPreset, fx, cy, pw, S(260), TRUE);
        MoveWindow(ui.tmpl, fx + pw + gap, cy, (std::max)(S(160), fw - pw - gap), rh, TRUE);
        cy += rh + rowGap;

        // 行3：画质 / 上限 / 容器 / 重编码
        // 右侧字段按文字测量，画质下拉吃掉剩下的宽度
        label(ui.lblQuality, cx, cy, lw);
        int capLbl = LblW(ui.lblResCap), capW = CboW(ui.resCap, 90, 160);
        int ctLbl = LblW(ui.lblContainer), ctW = CboW(ui.container, 110, 220);
        int recodeW = ChkW(ui.recode);
        int fixedRight = gap + capLbl + capW + gap + ctLbl + ctW + gap + recodeW;
        int qw = (std::max)(S(180), fw - fixedRight);
        MoveWindow(ui.quality, fx, cy, qw, S(260), TRUE);
        int x2 = fx + qw + gap;
        label(ui.lblResCap, x2, cy, capLbl);
        MoveWindow(ui.resCap, x2 + capLbl, cy, capW, S(260), TRUE);
        x2 += capLbl + capW + gap;
        label(ui.lblContainer, x2, cy, ctLbl);
        MoveWindow(ui.container, x2 + ctLbl, cy, ctW, S(260), TRUE);
        check(ui.recode, x2 + ctLbl + ctW + gap, cy, recodeW);
        cy += rh + rowGap;

        // 行4：仅音频 / 自定义格式
        int aoW = ChkW(ui.audioOnly);
        int afW = CboW(ui.aFormat, 120, 240);
        int aqW = CboW(ui.aQuality, 100, 200);
        int cfW = ChkW(ui.useCustomFmt);
        check(ui.audioOnly, cx, cy, aoW);
        int x3 = cx + aoW + gap;
        MoveWindow(ui.aFormat, x3, cy, afW, S(260), TRUE);
        x3 += afW + gap;
        MoveWindow(ui.aQuality, x3, cy, aqW, S(260), TRUE);
        x3 += aqW + S(20);
        check(ui.useCustomFmt, x3, cy, cfW);
        x3 += cfW + gap;
        MoveWindow(ui.customFmt, x3, cy, (std::max)(S(120), cx + cw - x3), rh, TRUE);
    }
    y += card2H + gap;

    // ---- 卡片3：标签页 ----
    int tabH = S(34);
    int pageH = rh * 2 + rowGap + S(6);
    int card3H = cp / 2 + tabH + gap + pageH + cp;
    g_cardRects[2] = {pad, y, W - pad, y + card3H};
    {
        int cx = pad + cp, cw = W - pad * 2 - cp * 2;
        int cy = y + cp / 2;
        MoveWindow(ui.tabs, cx, cy, cw, tabH, TRUE);
        cy += tabH + gap;
        int cy2 = cy + rh + rowGap;

        // 页0 Cookies
        {
            int mw = CboW(ui.ckMode, 160, 320);
            MoveWindow(ui.ckMode, cx, cy, mw, S(260), TRUE);
            int px = cx + mw + gap;
            int pickW = BtnW(ui.ckPick, 90);
            MoveWindow(ui.ckPath, px, cy, (std::max)(S(120), cw - (px - cx) - pickW - gap), rh,
                       TRUE);
            MoveWindow(ui.ckPick, cx + cw - pickW, cy, pickW, rh, TRUE);
            int bw = CboW(ui.ckBrowser, 150, 300);
            MoveWindow(ui.ckBrowser, px, cy, bw, S(260), TRUE);
            MoveWindow(ui.ckProfile, px + bw + gap, cy,
                       (std::max)(S(120), cw - (px - cx) - bw - gap), rh, TRUE);
            MoveWindow(ui.lblCkHint, cx, cy2, cw, rh + S(6), TRUE);
        }

        // 页1 字幕与嵌入：两行，每行按测量宽度依次摆
        {
            int x = cx;
            for (HWND h : {ui.subWrite, ui.subAuto, ui.subEmbed, ui.subSrt}) {
                int w = ChkW(h);
                check(h, x, cy, w);
                x += w + S(14);
            }
            int ll = LblW(ui.lblSubLangs);
            label(ui.lblSubLangs, x, cy, ll);
            x += ll + gap;
            MoveWindow(ui.subLangs, x, cy, (std::max)(S(120), cx + cw - x), rh, TRUE);

            x = cx;
            for (HWND h : {ui.embThumb, ui.embMeta, ui.embChap, ui.sponsor}) {
                int w = ChkW(h);
                check(h, x, cy2, w);
                x += w + S(14);
            }
            MoveWindow(ui.sponsorCats, x, cy2, (std::max)(S(120), cx + cw - x), rh, TRUE);
        }

        // 页2 网络
        {
            int x = cx;
            int l1 = LblW(ui.lblProxy);
            label(ui.lblProxy, x, cy, l1);
            x += l1 + gap;
            MoveWindow(ui.proxy, x, cy, S(240), rh, TRUE);
            x += S(240) + S(18);
            int l2 = LblW(ui.lblRate);
            label(ui.lblRate, x, cy, l2);
            x += l2 + gap;
            MoveWindow(ui.rate, x, cy, S(90), rh, TRUE);
            x += S(90) + S(18);
            int l3 = LblW(ui.lblFrags);
            label(ui.lblFrags, x, cy, l3);
            x += l3 + gap;
            MoveWindow(ui.frags, x, cy, S(56), rh, TRUE);
            x += S(56) + S(18);
            int l4 = LblW(ui.lblRetries);
            label(ui.lblRetries, x, cy, l4);
            x += l4 + gap;
            MoveWindow(ui.retries, x, cy, S(56), rh, TRUE);
            check(ui.polite, cx, cy2, cw);
        }

        // 页3 播放列表
        {
            int ll = LblW(ui.lblPlItems);
            label(ui.lblPlItems, cx, cy, ll);
            MoveWindow(ui.plItems, cx + ll + gap, cy,
                       (std::max)(S(200), cw - ll - gap), rh, TRUE);
            int x = cx;
            for (HWND h : {ui.plFolder, ui.archive, ui.ignErr, ui.noOver}) {
                int w = ChkW(h);
                check(h, x, cy2, w);
                x += w + S(14);
            }
        }

        // 页4 高级
        {
            int ll = LblW(ui.lblExtra);
            label(ui.lblExtra, cx, cy, ll);
            MoveWindow(ui.extra, cx + ll + gap, cy, (std::max)(S(200), cw - ll - gap), rh, TRUE);

            int wGet = BtnW(ui.btnGetYtDlp, 130);
            int wUpd = BtnW(ui.btnUpdateYtDlp, 64);
            int wCmd = BtnW(ui.btnShowCmd, 110);
            int right = wGet + gap + wUpd + gap + wCmd;
            int l2 = LblW(ui.lblYtDlp);
            label(ui.lblYtDlp, cx, cy2, l2);
            label(ui.ytDlpPath, cx + l2 + gap, cy2,
                  (std::max)(S(100), cw - l2 - gap - right - S(16)));
            int bx = cx + cw - right;
            MoveWindow(ui.btnGetYtDlp, bx, cy2, wGet, rh, TRUE);
            MoveWindow(ui.btnUpdateYtDlp, bx + wGet + gap, cy2, wUpd, rh, TRUE);
            MoveWindow(ui.btnShowCmd, bx + wGet + gap + wUpd + gap, cy2, wCmd, rh, TRUE);
        }
    }
    y += card3H + gap;

    // ---- 运行行 ----
    int runH = S(36);
    {
        int cx = pad;
        int wStart = BtnW(ui.btnStart, 110);
        int wStop = BtnW(ui.btnStop, 76);
        MoveWindow(ui.btnStart, cx, y, wStart, runH, TRUE);
        MoveWindow(ui.btnStop, cx + wStart + gap, y, wStop, runH, TRUE);
        int px = cx + wStart + gap + wStop + S(14);
        MoveWindow(ui.progress, px, y + S(6), (std::max)(S(100), W - pad - px), runH - S(12),
                   TRUE);
    }
    y += runH + S(4);
    MoveWindow(ui.lblStatus, pad, y, W - pad * 2, S(18), TRUE);
    y += S(18) + gap;

    // ---- 卡片4：日志 ----
    int logBottom = rc.bottom - pad;
    int card4H = (std::max)(S(120), logBottom - y);
    g_cardRects[3] = {pad, y, W - pad, y + card4H};
    {
        int cx = pad + cp, cw = W - pad * 2 - cp * 2;
        int cy = y + titleH;
        int barH = rh;
        int logH = (std::max)(S(60), card4H - titleH - barH - rowGap - cp);
        MoveWindow(ui.log, cx, cy, cw, logH, TRUE);
        int by = cy + logH + rowGap;
        check(ui.autoScroll, cx, by, ChkW(ui.autoScroll));

        int wClear = BtnW(ui.btnClearLog, 64);
        int wSave = BtnW(ui.btnSaveLog, 64);
        int wCopy = BtnW(ui.btnCopyLog, 64);
        int bx = cx + cw - wClear;
        MoveWindow(ui.btnClearLog, bx, by, wClear, rh, TRUE);
        bx -= wSave + gap;
        MoveWindow(ui.btnSaveLog, bx, by, wSave, rh, TRUE);
        bx -= wCopy + gap;
        MoveWindow(ui.btnCopyLog, bx, by, wCopy, rh, TRUE);
    }

    ShowPage(theme::TabStripSel(ui.tabs));
    InvalidateRect(p, nullptr, FALSE);
}

// ---------------------------------------------------------------- 语言切换

// 字体在 DPI 变化和语言切换时都要重来一遍
void ApplyFonts() {
    EnumChildWindows(g_main, [](HWND c, LPARAM) -> BOOL {
        SendMessageW(c, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Body), TRUE);
        return TRUE;
    }, 0);
    // 上面一刀切成正文字体，这里把该用小号和等宽的挑回来
    SendMessageW(ui.log, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Mono), TRUE);
    for (HWND h : {ui.lblUrlCount, ui.lblStatus, ui.lblCkHint, ui.lblYtDlp, ui.ytDlpPath})
        SendMessageW(h, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Small), TRUE);
    // 按钮是自绘的，DrawButton 里自己选字体，不需要在这里处理
}

// 把界面上所有文字换成当前语言。下拉框要保住原有选中项。
void ApplyLanguage() {
    theme::SetFontFace(i18n::UiFontFace());
    theme::RebuildFonts();
    ApplyFonts();

    SetWindowTextW(g_main, AppTitle());
    g_cardTitles[0] = i18n::T(S_CARD_LINKS);
    g_cardTitles[1] = i18n::T(S_CARD_OUTPUT);
    g_cardTitles[2] = i18n::T(S_CARD_OPTIONS);
    g_cardTitles[3] = i18n::T(S_CARD_LOG);

    struct { HWND h; StrId s; } texts[] = {
        {ui.btnPaste, S_BTN_PASTE},        {ui.btnClearUrl, S_BTN_CLEAR},
        {ui.btnProbe, S_BTN_PROBE},        {ui.lblOut, S_LBL_SAVETO},
        {ui.btnBrowse, S_BTN_BROWSE},      {ui.btnOpenDir, S_BTN_OPEN},
        {ui.lblTmpl, S_LBL_FILENAME},      {ui.lblQuality, S_LBL_QUALITY},
        {ui.lblResCap, S_LBL_MAXRES},      {ui.lblContainer, S_LBL_CONTAINER},
        {ui.recode, S_CHK_RECODE},         {ui.audioOnly, S_CHK_AUDIOONLY},
        {ui.useCustomFmt, S_CHK_CUSTOMFMT},{ui.ckPick, S_CK_PICKFILE},
        {ui.lblCkHint, S_CK_HINT},         {ui.subWrite, S_SUB_WRITE},
        {ui.subAuto, S_SUB_AUTO},          {ui.subEmbed, S_SUB_EMBED},
        {ui.subSrt, S_SUB_SRT},            {ui.lblSubLangs, S_SUB_LANGS},
        {ui.embThumb, S_EMB_THUMB},        {ui.embMeta, S_EMB_META},
        {ui.embChap, S_EMB_CHAP},          {ui.sponsor, S_SPONSOR},
        {ui.lblProxy, S_NET_PROXY},        {ui.lblRate, S_NET_RATE},
        {ui.lblFrags, S_NET_FRAGS},        {ui.lblRetries, S_NET_RETRIES},
        {ui.polite, S_NET_POLITE},         {ui.lblPlItems, S_PL_RANGE},
        {ui.plFolder, S_PL_FOLDER},        {ui.archive, S_PL_ARCHIVE},
        {ui.ignErr, S_PL_IGNERR},          {ui.noOver, S_PL_NOOVER},
        {ui.lblExtra, S_ADV_EXTRA},        {ui.btnGetYtDlp, S_ADV_GET},
        {ui.btnUpdateYtDlp, S_ADV_UPDATE}, {ui.btnShowCmd, S_ADV_SHOWCMD},
        {ui.btnStart, S_BTN_START},        {ui.btnStop, S_BTN_STOP},
        {ui.autoScroll, S_CHK_AUTOSCROLL}, {ui.btnCopyLog, S_BTN_COPY},
        {ui.btnSaveLog, S_BTN_SAVE},       {ui.btnClearLog, S_BTN_CLEAR},
    };
    for (auto& t : texts) SetWindowTextW(t.h, i18n::T(t.s));

    struct { HWND h; StrId s; } cues[] = {
        {ui.url, S_URL_CUE},          {ui.customFmt, S_CUSTOMFMT_CUE},
        {ui.ckProfile, S_CK_PROFILE_CUE}, {ui.proxy, S_NET_PROXY_CUE},
        {ui.rate, S_NET_RATE_CUE},    {ui.plItems, S_PL_RANGE_CUE},
        {ui.extra, S_ADV_EXTRA_CUE},
    };
    for (auto& c : cues)
        SendMessageW(c.h, EM_SETCUEBANNER, TRUE, (LPARAM)i18n::T(c.s));

    auto refill = [](HWND c, const NamedValue* items, int n) {
        int sel = ComboBox_GetCurSel(c);
        ComboBox_ResetContent(c);
        for (int i = 0; i < n; ++i) ComboBox_AddString(c, NVLabel(items[i]));
        ComboBox_SetCurSel(c, sel);
    };
    refill(ui.resCap, RES_CAPS, RES_CAP_COUNT);
    refill(ui.container, CONTAINERS, CONTAINER_COUNT);
    refill(ui.aFormat, AUDIO_FORMATS, AUDIO_FORMAT_COUNT);
    refill(ui.ckBrowser, BROWSERS, BROWSER_COUNT);
    refill(ui.tmplPreset, TEMPLATE_PRESETS, TEMPLATE_PRESET_COUNT);

    int sel = ComboBox_GetCurSel(ui.quality);
    ComboBox_ResetContent(ui.quality);
    for (int i = 0; i < QUALITY_PRESET_COUNT; ++i)
        ComboBox_AddString(ui.quality, i18n::T(QUALITY_PRESETS[i].label));
    ComboBox_SetCurSel(ui.quality, sel);

    sel = ComboBox_GetCurSel(ui.ckMode);
    ComboBox_ResetContent(ui.ckMode);
    ComboBox_AddString(ui.ckMode, i18n::T(S_CK_NONE));
    ComboBox_AddString(ui.ckMode, i18n::T(S_CK_FILE));
    ComboBox_AddString(ui.ckMode, i18n::T(S_CK_BROWSER));
    ComboBox_SetCurSel(ui.ckMode, sel);

    sel = ComboBox_GetCurSel(ui.aQuality);
    ComboBox_ResetContent(ui.aQuality);
    for (int i = 0; i <= 10; ++i) {
        std::wstring s = util::Format(i18n::T(S_AQ_FMT), i);
        if (i == 0) s += i18n::T(S_AQ_BEST);
        else if (i == 10) s += i18n::T(S_AQ_WORST);
        ComboBox_AddString(ui.aQuality, s.c_str());
    }
    ComboBox_SetCurSel(ui.aQuality, sel);

    theme::TabStripSetLabels(ui.tabs, TabLabels());

    if (!g_ytDlpPath.empty())
        SetWindowTextW(ui.ytDlpPath, (g_ytDlpVersion + L"   " + g_ytDlpPath).c_str());
    else
        SetWindowTextW(ui.ytDlpPath, i18n::T(S_ADV_NOTFOUND));

    UpdateUrlCount();
    Layout(g_main);
    InvalidateRect(g_main, nullptr, TRUE);
}

// ---------------------------------------------------------------- 绘制

void PaintMain(HWND p, HDC dc) {
    RECT rc;
    GetClientRect(p, &rc);
    theme::FillRect(dc, rc, theme::P.bg);

    // 顶栏
    SetBkMode(dc, TRANSPARENT);
    RECT hr = g_headerRect;
    HFONT old = (HFONT)SelectObject(dc, theme::Font(theme::FontRole::Title));
    SetTextColor(dc, theme::P.text);
    RECT tr = hr;
    tr.left += theme::S(2);
    // DT_NOPREFIX 不能少：英文标题里有 "&"，否则会被当成快捷键前缀吃掉
    DrawTextW(dc, AppTitle(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // 右侧状态徽章：yt-dlp / Deno / ffmpeg
    SelectObject(dc, theme::Font(theme::FontRole::Small));
    std::wstring badge;
    if (g_ytDlpVersion.empty()) badge = i18n::T(S_HDR_YTDLP_NOTREADY);
    else badge = L"yt-dlp " + g_ytDlpVersion;
    badge += L"   ·   ";
    badge += i18n::T(g_tools.hasDeno ? S_HDR_DENO_OK : S_HDR_DENO_MISSING);
    badge += L"   ·   ";
    badge += i18n::T(g_tools.hasFfmpeg ? S_HDR_FFMPEG_OK : S_HDR_FFMPEG_MISSING);

    bool allGood = !g_ytDlpVersion.empty() && g_tools.hasDeno && g_tools.hasFfmpeg;
    SetTextColor(dc, allGood ? theme::P.textDim : theme::P.warn);
    RECT br = hr;
    br.right -= g_langBoxW + theme::S(14);  // 给右边的语言下拉框让位
    DrawTextW(dc, badge.c_str(), -1, &br,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // 卡片
    for (int i = 0; i < 4; ++i) {
        theme::FillRoundBorder(dc, g_cardRects[i], theme::S(10), theme::P.card,
                               theme::P.border, 1.0f);
        SelectObject(dc, theme::Font(theme::FontRole::Bold));
        SetTextColor(dc, theme::P.textDim);
        RECT t = g_cardRects[i];
        t.left += theme::S(14);
        t.top += theme::S(4);
        t.bottom = t.top + theme::S(20);
        DrawTextW(dc, g_cardTitles[i].c_str(), -1, &t,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    SelectObject(dc, old);
}

// ---------------------------------------------------------------- 消息

void OnLines(std::vector<std::wstring>* batch) {
    std::unique_ptr<std::vector<std::wstring>> guard(batch);
    // 批量插入时先关重绘，几百行也不会闪
    SendMessageW(ui.log, WM_SETREDRAW, FALSE, 0);
    for (auto& line : *batch) {
        if (line.empty()) continue;
        bool progressOnly = HandleLine(line);
        if (progressOnly) continue;
        LogRaw(line, ClassifyLine(line));
    }
    SendMessageW(ui.log, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(ui.log, nullptr, FALSE);
    if (Checked(ui.autoScroll)) SendMessageW(ui.log, WM_VSCROLL, SB_BOTTOM, 0);
}

void OnDone(DWORD code) {
    SetBusy(false);
    bool killed = g_runner.KillRequested();
    if (killed) {
        LogWarn(i18n::T(S_LOG_STOPPED));
        SetWindowTextW(ui.lblStatus, i18n::T(S_STATUS_STOPPED));
        theme::SetProgress(ui.progress, 0, L"");
    } else if (code == 0) {
        LogOk(i18n::T(S_LOG_ALLDONE));
        SetWindowTextW(ui.lblStatus, i18n::T(S_STATUS_DONE));
        theme::SetProgress(ui.progress, 100, L"100%");
    } else {
        LogErr(util::Format(i18n::T(S_LOG_EXITCODE), code));
        SetWindowTextW(ui.lblStatus,
                       util::Format(i18n::T(S_STATUS_ERRORS), g_run.errors).c_str());
        theme::SetProgress(ui.progress, 0, L"");
    }
    FlashWindow(g_main, TRUE);
}

void OnProbeDone(ProbeResult* raw) {
    std::unique_ptr<ProbeResult> pr(raw);
    g_probing = false;
    EnableWindow(ui.btnProbe, TRUE);
    theme::SetProgress(ui.progress, 0, L"");
    SetWindowTextW(ui.lblStatus, L"");

    if (!pr->ok) {
        LogErr(i18n::T(S_LOG_PROBE_FAILED) + pr->error);
        MessageBoxW(g_main, (i18n::T(S_MSG_PROBEFAIL) + pr->error).c_str(), AppTitle(),
                    MB_ICONWARNING);
        return;
    }
    LogOk(util::Format(i18n::T(S_LOG_PROBE_OK), pr->formats.size(),
                       pr->title.c_str()));

    std::wstring chosen = ShowFormatPicker(g_main, *pr);
    if (chosen.empty()) return;
    SetWindowTextW(ui.customFmt, chosen.c_str());
    SetCheck(ui.useCustomFmt, true);
    SyncEnabled();
    LogOk(i18n::T(S_LOG_FMT_CHOSEN) + chosen);
}

void OnToolsDetected() {
    if (g_ytDlpPath.empty()) {
        SetWindowTextW(ui.ytDlpPath, i18n::T(S_ADV_NOTFOUND));
        LogErr(i18n::T(S_LOG_NO_YTDLP));
        LogPlain(i18n::T(S_LOG_NO_YTDLP_HINT));
    } else {
        SetWindowTextW(ui.ytDlpPath, (g_ytDlpVersion + L"   " + g_ytDlpPath).c_str());
        LogOk(L"yt-dlp " + g_ytDlpVersion + L"  —  " + g_ytDlpPath);
    }
    if (!g_tools.hasDeno) {
        LogWarn(i18n::T(S_LOG_DENO_WARN1));
        LogWarn(i18n::T(S_LOG_DENO_WARN2));
        LogPlain(i18n::T(S_LOG_DENO_INSTALL));
    } else {
        LogOk(std::wstring(i18n::T(S_HDR_DENO_OK)) + L"  —  " + g_tools.denoPath);
    }
    if (!g_tools.hasFfmpeg) {
        LogWarn(i18n::T(S_LOG_FFMPEG_WARN));
    } else {
        LogOk(std::wstring(i18n::T(S_HDR_FFMPEG_OK)) + L"  —  " + g_tools.ffmpegPath);
    }
    InvalidateRect(g_main, &g_headerRect, FALSE);
}

LRESULT CALLBACK MainProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(h, &ps);
            // 整窗双缓冲，卡片圆角重绘时才不会闪
            RECT rc;
            GetClientRect(h, &rc);
            HDC mem = CreateCompatibleDC(dc);
            HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
            HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);
            PaintMain(h, mem);
            BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
            SelectObject(mem, oldBmp);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(h, &ps);
            return 0;
        }

        case WM_SIZE:
            if (wp != SIZE_MINIMIZED) Layout(h);
            return 0;

        case WM_GETMINMAXINFO: {
            auto* mmi = (MINMAXINFO*)lp;
            mmi->ptMinTrackSize.x = theme::S(960);
            mmi->ptMinTrackSize.y = theme::S(740);
            return 0;
        }

        case WM_DPICHANGED: {
            theme::SetDpi(HIWORD(wp));
            theme::RebuildFonts();
            ApplyFonts();
            RECT* nr = (RECT*)lp;
            SetWindowPos(h, nullptr, nr->left, nr->top, nr->right - nr->left,
                         nr->bottom - nr->top, SWP_NOZORDER | SWP_NOACTIVATE);
            Layout(h);
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wp;
            HWND ctl = (HWND)lp;
            bool dim = ctl == ui.lblCkHint || ctl == ui.lblStatus ||
                       ctl == ui.lblUrlCount || ctl == ui.ytDlpPath ||
                       ctl == ui.lblYtDlp;
            // 状态行不在任何卡片里，底色得跟窗口背景一致
            COLORREF back = (ctl == ui.lblStatus) ? theme::P.bg : theme::P.card;
            SetTextColor(dc, dim ? theme::P.textDim : theme::P.text);
            SetBkColor(dc, back);
            SetDCBrushColor(dc, back);
            return (LRESULT)GetStockObject(DC_BRUSH);
        }

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC dc = (HDC)wp;
            SetTextColor(dc, theme::P.text);
            SetBkColor(dc, theme::P.input);
            SetDCBrushColor(dc, theme::P.input);
            return (LRESULT)GetStockObject(DC_BRUSH);
        }

        case WM_DRAWITEM:
            theme::DrawButton((LPDRAWITEMSTRUCT)lp);
            return TRUE;

        case WM_APP_LINES:
            OnLines((std::vector<std::wstring>*)lp);
            return 0;

        case WM_APP_DONE:
            OnDone((DWORD)wp);
            return 0;

        case WM_APP_PROBEDONE:
            OnProbeDone((ProbeResult*)lp);
            return 0;

        case WM_APP_TOOLS:
            OnToolsDetected();
            return 0;

        case WM_APP_GETDONE:
            SetBusy(false);
            theme::SetProgress(ui.progress, 0, L"");
            if (wp) {
                LogOk(i18n::T(S_LOG_GETOK));
                SetWindowTextW(ui.lblStatus, L"");
                HANDLE t = CreateThread(nullptr, 0, DetectThread, nullptr, 0, nullptr);
                if (t) CloseHandle(t);
            } else {
                LogErr(i18n::T(S_LOG_GETFAIL));
                SetWindowTextW(ui.lblStatus, i18n::T(S_STATUS_GETFAIL));
            }
            return 0;

        case WM_DROPFILES: {
            auto drop = (HDROP)wp;
            UINT n = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < n; ++i) {
                wchar_t path[MAX_PATH * 2] = {};
                DragQueryFileW(drop, i, path, (UINT)std::size(path));
                std::wstring p = path;
                if (util::ContainsNoCase(p, L".txt")) {
                    // 丢进来一个文本文件，就把里面的链接读出来
                    HANDLE f = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                           OPEN_EXISTING, 0, nullptr);
                    if (f != INVALID_HANDLE_VALUE) {
                        std::string raw;
                        char buf[4096];
                        DWORD got = 0;
                        while (ReadFile(f, buf, sizeof(buf), &got, nullptr) && got)
                            raw.append(buf, got);
                        CloseHandle(f);
                        std::wstring cur = GetText(ui.url);
                        if (!cur.empty() && cur.back() != L'\n') cur += L"\r\n";
                        SetWindowTextW(ui.url, (cur + util::DecodeConsole(raw)).c_str());
                    }
                } else if (util::ContainsNoCase(p, L"cookies")) {
                    SetWindowTextW(ui.ckPath, p.c_str());
                    ComboBox_SetCurSel(ui.ckMode, 1);
                    SyncEnabled();
                }
            }
            DragFinish(drop);
            UpdateUrlCount();
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wp);
            int code = HIWORD(wp);

            if (id == IDC_TABS && code == theme::TABSTRIP_SELCHANGE) {
                ShowPage(theme::TabStripSel(ui.tabs));
                return 0;
            }
            if (id == IDC_URL && code == EN_CHANGE) {
                UpdateUrlCount();
                return 0;
            }
            if (code == CBN_SELCHANGE) {
                if (id == IDC_TMPLPRESET) {
                    int i = ComboBox_GetCurSel(ui.tmplPreset);
                    if (i >= 0 && i < TEMPLATE_PRESET_COUNT)
                        SetWindowTextW(ui.tmpl, TEMPLATE_PRESETS[i].value);
                    return 0;
                }
                if (id == IDC_CKMODE) {
                    SyncEnabled();
                    SyncCookieRow();
                    return 0;
                }
                if (id == IDC_LANG) {
                    // 下拉框位置 != 语言下标，要先换算
                    int pos = ComboBox_GetCurSel(ui.lang);
                    int sel = (int)i18n::LangAtDisplayIndex(pos);
                    if (pos >= 0 && sel != i18n::CurrentIndex()) {
                        // 先把当前界面状态收回结构体，再换语言重建界面文字，
                        // 否则用户已改但没保存的输入会被覆盖掉
                        SettingsFromUi();
                        // 字幕语言如果还是上一个界面语言的默认值（用户没动过），
                        // 就跟着一起换；动过的话尊重用户的设置
                        bool subsAtDefault =
                            (g_cfg.subLangs == i18n::T(S_DEFAULT_SUBLANGS));
                        g_cfg.lang = sel;
                        i18n::SetLangIndex(sel);
                        if (subsAtDefault) {
                            g_cfg.subLangs = i18n::T(S_DEFAULT_SUBLANGS);
                            SetWindowTextW(ui.subLangs, g_cfg.subLangs.c_str());
                        }
                        ApplyLanguage();
                        g_cfg.Save(g_configPath);
                    }
                    return 0;
                }
            }
            if (code == BN_CLICKED) {
                switch (id) {
                    case IDC_AUDIOONLY:
                    case IDC_USECUSTOMFMT:
                    case IDC_SUBWRITE:
                    case IDC_SUBAUTO:
                    case IDC_SPONSOR:
                        SyncEnabled();
                        return 0;

                    case IDC_PASTE: {
                        if (!OpenClipboard(h)) return 0;
                        HANDLE d = GetClipboardData(CF_UNICODETEXT);
                        if (d) {
                            auto* txt = (const wchar_t*)GlobalLock(d);
                            if (txt) {
                                std::wstring cur = util::Trim(GetText(ui.url));
                                std::wstring add = util::Trim(txt);
                                if (!add.empty())
                                    SetWindowTextW(ui.url, (cur.empty() ? add
                                                                        : cur + L"\r\n" + add)
                                                               .c_str());
                                GlobalUnlock(d);
                            }
                        }
                        CloseClipboard();
                        UpdateUrlCount();
                        return 0;
                    }
                    case IDC_CLEARURL:
                        SetWindowTextW(ui.url, L"");
                        UpdateUrlCount();
                        return 0;

                    case IDC_PROBE:
                        StartProbe();
                        return 0;

                    case IDC_BROWSE: {
                        std::wstring d = PickFolder(h, GetText(ui.outDir));
                        if (!d.empty()) SetWindowTextW(ui.outDir, d.c_str());
                        return 0;
                    }
                    case IDC_OPENDIR: {
                        std::wstring d = util::Trim(GetText(ui.outDir));
                        if (d.empty()) return 0;
                        util::EnsureDir(d);
                        ShellExecuteW(h, L"open", d.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                        return 0;
                    }
                    case IDC_CKPICK: {
                        std::wstring filter = MakeFilter(S_FILTER_COOKIES, L"*.txt");
                        std::wstring f = PickFile(h, filter.c_str(), false);
                        if (!f.empty()) {
                            SetWindowTextW(ui.ckPath, f.c_str());
                            ComboBox_SetCurSel(ui.ckMode, 1);
                            SyncEnabled();
                        }
                        return 0;
                    }
                    case IDC_GETYTDLP:
                        StartGetYtDlp();
                        return 0;

                    case IDC_UPDATEYTDLP: {
                        if (!EnsureReady() || g_busy) return 0;
                        LogInfo(i18n::T(S_LOG_UPDATING));
                        SetBusy(true);
                        theme::SetProgress(ui.progress, -1, L"");
                        if (!g_runner.Start({g_ytDlpPath, L"-U"}, util::ExeDir(), h,
                                            WM_APP_LINES, WM_APP_DONE)) {
                            LogErr(i18n::T(S_LOG_NOUPDATE));
                            SetBusy(false);
                        }
                        return 0;
                    }
                    case IDC_SHOWCMD: {
                        SettingsFromUi();
                        std::wstring exe = g_ytDlpPath.empty() ? L"yt-dlp.exe" : g_ytDlpPath;
                        auto args = g_cfg.BuildArgs(exe);
                        for (auto& u : UrlList()) args.push_back(u);
                        LogInfo(L"> " + util::JoinCommandLine(args));
                        return 0;
                    }

                    case IDC_START:
                        StartDownload();
                        return 0;

                    case IDC_STOP:
                        if (g_busy) {
                            SetWindowTextW(ui.lblStatus, i18n::T(S_STATUS_STOPPING));
                            g_runner.Kill();
                        }
                        return 0;

                    case IDC_COPYLOG:
                        CopyLogToClipboard();
                        return 0;
                    case IDC_SAVELOG:
                        SaveLogToFile();
                        return 0;
                    case IDC_CLEARLOG:
                        SetWindowTextW(ui.log, L"");
                        return 0;
                }
            }
            return 0;
        }

        case WM_CLOSE:
            if (g_busy) {
                if (MessageBoxW(h, i18n::T(S_MSG_QUIT), AppTitle(),
                                MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
                    return 0;
                g_runner.Kill();
            }
            SettingsFromUi();
            {
                RECT wr;
                GetWindowRect(h, &wr);
                g_cfg.winW = wr.right - wr.left;
                g_cfg.winH = wr.bottom - wr.top;
            }
            g_cfg.Save(g_configPath);
            DestroyWindow(h);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

}  // namespace

// ---------------------------------------------------------------- 入口

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int show) {
    g_inst = inst;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES |
                                              ICC_PROGRESS_CLASS | ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);
    LoadLibraryW(L"msftedit.dll");  // RichEdit 4.1，日志要靠它上色

    theme::Init();
    theme::RegisterProgressClass(inst);
    theme::RegisterTabStripClass(inst);
    RegisterFormatPickerClass(inst);

    g_configPath = util::PathJoin(util::ExeDir(), L"settings.ini");
    g_cfg.outDir = util::PathJoin(util::KnownFolderDownloads(), L"YouTube");
    g_cfg.outputTemplate = TEMPLATE_PRESETS[0].value;
    // 首次运行一律用英语（本程序主要面向国际用户）。
    // 想改成跟随 Windows 显示语言的话，把下面这行换成：
    //     g_cfg.lang = (int)i18n::DetectSystem();
    // settings.ini 里存过语言的话，下面的 Load 会覆盖掉这个默认值。
    g_cfg.lang = (int)i18n::Lang::En;
    g_cfg.Load(g_configPath);
    i18n::SetLangIndex(g_cfg.lang);
    theme::SetFontFace(i18n::UiFontFace());

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = MainProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kMainClass;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);

    UINT dpi = GetDpiForSystem();
    theme::SetDpi(dpi);
    theme::RebuildFonts();

    int w = g_cfg.winW > 0 ? g_cfg.winW : theme::S(1120);
    int ht = g_cfg.winH > 0 ? g_cfg.winH : theme::S(840);

    // 别让默认尺寸超出可用桌面，否则底部的日志和按钮会掉到任务栏后面
    RECT work{};
    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0)) {
        int maxW = work.right - work.left;
        int maxH = work.bottom - work.top;
        w = (std::min)(w, maxW);
        ht = (std::min)(ht, maxH);
    }
    int x = work.left + ((work.right - work.left) - w) / 2;
    int y = work.top + ((work.bottom - work.top) - ht) / 2;

    g_main = CreateWindowExW(WS_EX_ACCEPTFILES, kMainClass, AppTitle(),
                             // WS_CLIPCHILDREN 必须有：父窗口自绘时会重画整个客户区，
                             // 没有它就会把子控件全盖掉，只剩刚好之后重绘过的那几个。
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             x, y, w, ht,
                             nullptr, nullptr, inst, nullptr);
    if (!g_main) return 1;

    theme::SetDpi(GetDpiForWindow(g_main));
    theme::RebuildFonts();
    theme::ApplyDarkTitleBar(g_main);

    CreateControls(g_main);
    theme::ApplyDarkToWindow(g_main);
    UiFromSettings();
    SyncEnabled();
    ApplyLanguage();   // 顺带填好卡片标题、字体和布局

    ShowWindow(g_main, show);
    UpdateWindow(g_main);

    LogPlain(i18n::T(S_LOG_STARTUP));
    HANDLE t = CreateThread(nullptr, 0, DetectThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(g_main, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    theme::Shutdown();
    CoUninitialize();
    return 0;
}
