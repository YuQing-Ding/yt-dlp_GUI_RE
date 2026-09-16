#include "config.h"

#include "util.h"  // 必须排在 shellapi.h 前面，它负责拉进 windows.h

#include <shellapi.h>

#include <cstdio>
#include <map>

// ---------------------------------------------------------------- 选项表

const QualityPreset QUALITY_PRESETS[] = {
    {S_Q_BEST,     L"res,fps,br"},
    {S_Q_BITRATE,  L"br,res,fps"},
    {S_Q_FPS,      L"fps,res,br"},
    {S_Q_HDR,      L"hdr:12,res,fps,br"},
    {S_Q_DEFAULT,  L""},
    {S_Q_SMALLEST, L"+size,+br,res"},
};
const int QUALITY_PRESET_COUNT = (int)std::size(QUALITY_PRESETS);

const wchar_t* NVLabel(const NamedValue& n) {
    return n.label == S_COUNT ? n.literal : i18n::T(n.label);
}

// 下拉框宽度有限，标签要短。完整解释放在 README 和界面提示里。
// 纯技术性的标签（分辨率、编码名）不翻译，各语言都一样。
const NamedValue RES_CAPS[] = {
    {S_RES_NONE, nullptr,       L""},
    {S_COUNT,    L"4K 2160p",   L"2160"},
    {S_COUNT,    L"2K 1440p",   L"1440"},
    {S_COUNT,    L"FHD 1080p",  L"1080"},
    {S_COUNT,    L"HD 720p",    L"720"},
    {S_COUNT,    L"SD 480p",    L"480"},
};
const int RES_CAP_COUNT = (int)std::size(RES_CAPS);

const NamedValue CONTAINERS[] = {
    {S_CT_AUTO, nullptr,             L""},
    {S_COUNT,   L"MP4 · H.264+AAC",  L"mp4"},
    {S_CT_MKV,  nullptr,             L"mkv"},
    {S_COUNT,   L"WebM",             L"webm"},
};
const int CONTAINER_COUNT = (int)std::size(CONTAINERS);

const NamedValue AUDIO_FORMATS[] = {
    {S_AF_BEST, nullptr,  L""},
    {S_COUNT,   L"MP3",   L"mp3"},
    {S_COUNT,   L"M4A",   L"m4a"},
    {S_COUNT,   L"Opus",  L"opus"},
    {S_COUNT,   L"FLAC",  L"flac"},
    {S_COUNT,   L"WAV",   L"wav"},
};
const int AUDIO_FORMAT_COUNT = (int)std::size(AUDIO_FORMATS);

const NamedValue BROWSERS[] = {
    {S_CK_FIREFOX_NOTE, nullptr,      L"firefox"},
    {S_COUNT,           L"Chrome",    L"chrome"},
    {S_COUNT,           L"Edge",      L"edge"},
    {S_COUNT,           L"Brave",     L"brave"},
    {S_COUNT,           L"Chromium",  L"chromium"},
    {S_COUNT,           L"Opera",     L"opera"},
    {S_COUNT,           L"Vivaldi",   L"vivaldi"},
    {S_COUNT,           L"Whale",     L"whale"},
};
const int BROWSER_COUNT = (int)std::size(BROWSERS);

const NamedValue TEMPLATE_PRESETS[] = {
    {S_TM_TITLE_ID, nullptr, L"%(title).150B [%(id)s].%(ext)s"},
    {S_TM_TITLE,    nullptr, L"%(title).180B.%(ext)s"},
    {S_TM_UPLOADER, nullptr, L"%(uploader).60B - %(title).120B [%(id)s].%(ext)s"},
    {S_TM_DATE,     nullptr, L"%(upload_date>%Y-%m-%d)s - %(title).120B [%(id)s].%(ext)s"},
    {S_TM_INDEX,    nullptr, L"%(playlist_index&{} - |)s%(title).130B [%(id)s].%(ext)s"},
};
const int TEMPLATE_PRESET_COUNT = (int)std::size(TEMPLATE_PRESETS);

// ---------------------------------------------------------------- 参数拼装

namespace {

// 把用户在「额外参数」里敲的一行拆成 argv。前面垫一个假程序名，
// 因为 CommandLineToArgvW 对第 0 个参数的引号规则和后面不一样。
std::vector<std::wstring> SplitArgs(const std::wstring& line) {
    std::vector<std::wstring> out;
    std::wstring trimmed = util::Trim(line);
    if (trimmed.empty()) return out;

    std::wstring full = L"x " + trimmed;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(full.c_str(), &argc);
    if (!argv) return out;
    for (int i = 1; i < argc; ++i) out.push_back(argv[i]);
    LocalFree(argv);
    return out;
}

}  // namespace

std::vector<std::wstring> Settings::BuildArgs(const std::wstring& ytDlpPath) const {
    std::vector<std::wstring> a;
    auto add  = [&](const std::wstring& s) { a.push_back(s); };
    auto add2 = [&](const std::wstring& k, const std::wstring& v) { a.push_back(k); a.push_back(v); };

    add(ytDlpPath);

    // 进度按行输出，否则全是 \r 刷同一行，管道这边没法解析
    add(L"--newline");
    add(L"--no-mtime");
    add(L"--progress");

    add2(L"-P", outDir);

    std::wstring tmpl = outputTemplate.empty()
                            ? std::wstring(TEMPLATE_PRESETS[0].value)
                            : outputTemplate;
    if (playlistFolder) {
        // 缺省值必须是 "."：留空会生成 "\文件名"，在 Windows 上指向盘符根目录
        tmpl = L"%(playlist_title|.)s/" + tmpl;
    }
    add2(L"-o", tmpl);

    // ---- 格式选择 ----
    if (useCustomFormat && !util::Trim(customFormat).empty()) {
        add2(L"-f", util::Trim(customFormat));
    } else if (audioOnly) {
        add2(L"-f", L"ba/b");
        add2(L"-S", L"abr,asr");
        add(L"-x");
        const wchar_t* af = AUDIO_FORMATS[audioFormat].value;
        if (af && *af) add2(L"--audio-format", af);
        add2(L"--audio-quality", std::to_wstring(audioQuality));
    } else {
        const wchar_t* sort = QUALITY_PRESETS[qualityPreset].sortOrder;
        if (sort && *sort) add2(L"-S", sort);

        std::wstring vfilter, afilter;
        const wchar_t* cap = RES_CAPS[resCap].value;
        if (cap && *cap) vfilter += L"[height<=" + std::wstring(cap) + L"]";

        const wchar_t* ext = CONTAINERS[container].value;
        bool wantMp4Native = (ext && wcscmp(ext, L"mp4") == 0 && !recode);
        if (wantMp4Native) {
            // 直接挑 H.264 + AAC 的原生流，省掉重编码
            vfilter += L"[vcodec^=avc1]";
            afilter += L"[acodec^=mp4a]";
        }

        if (!vfilter.empty() || !afilter.empty()) {
            std::wstring f = L"bv*" + vfilter + L"+ba" + afilter;
            // 逐级放宽：严格匹配 → 只保留分辨率上限 → 随便给一个
            std::wstring capOnly = (cap && *cap) ? L"[height<=" + std::wstring(cap) + L"]" : L"";
            f += L"/bv*" + capOnly + L"+ba";
            f += L"/b" + capOnly;
            f += L"/bv*+ba/b";
            add2(L"-f", f);
        }

        if (ext && *ext) {
            if (recode) add2(L"--recode-video", ext);
            else        add2(L"--merge-output-format", ext);
        }
    }

    // ---- Cookies ----
    if (cookieMode == 1 && !util::Trim(cookieFile).empty()) {
        add2(L"--cookies", cookieFile);
    } else if (cookieMode == 2) {
        std::wstring spec = BROWSERS[browserIndex].value;
        std::wstring prof = util::Trim(browserProfile);
        if (!prof.empty()) spec += L":" + prof;
        add2(L"--cookies-from-browser", spec);
    }

    // ---- 字幕 ----
    if (writeSubs || autoSubs) {
        if (writeSubs) add(L"--write-subs");
        if (autoSubs)  add(L"--write-auto-subs");
        add2(L"--sub-langs",
             subLangs.empty() ? std::wstring(i18n::T(S_DEFAULT_SUBLANGS)) : subLangs);
        if (convertSrt) add2(L"--convert-subs", L"srt");
        if (embedSubs)  add(L"--embed-subs");
    }

    // ---- 嵌入 ----
    if (embedThumb)    add(L"--embed-thumbnail");
    if (embedMeta)     add(L"--embed-metadata");
    if (embedChapters) add(L"--embed-chapters");

    if (sponsorBlock && !util::Trim(sponsorCats).empty())
        add2(L"--sponsorblock-remove", sponsorCats);

    // ---- 播放列表 / 行为 ----
    if (!util::Trim(playlistItems).empty())
        add2(L"--playlist-items", util::Trim(playlistItems));
    if (useArchive) {
        // v1.0 用的是中文名。已经有那个文件就继续用，免得老用户的记录失效；
        // 否则用语言无关的新名字。
        std::wstring legacy = util::PathJoin(outDir, L"已下载记录.txt");
        add2(L"--download-archive",
             util::FileExists(legacy) ? legacy : util::PathJoin(outDir, L"downloaded.txt"));
    }
    if (ignoreErrors)  add(L"--ignore-errors");
    if (noOverwrites)  add(L"--no-overwrites");

    if (politeMode) {
        // 批量下播放列表时请求过密是触发 "Sign in to confirm you're not a bot" 的主因
        add2(L"--sleep-requests", L"1.5");
        add2(L"--min-sleep-interval", L"3");
        add2(L"--max-sleep-interval", L"12");
    }

    // ---- 网络 ----
    if (!util::Trim(proxy).empty())     add2(L"--proxy", util::Trim(proxy));
    if (!util::Trim(rateLimit).empty()) add2(L"-r", util::Trim(rateLimit));
    if (concurrentFrags > 0) add2(L"--concurrent-fragments", std::to_wstring(concurrentFrags));
    if (retries > 0) {
        add2(L"--retries", std::to_wstring(retries));
        add2(L"--fragment-retries", std::to_wstring(retries));
    }

    for (auto& x : SplitArgs(extraArgs)) add(x);
    return a;
}

// ---------------------------------------------------------------- 存取

namespace {

void PutS(std::wstring& buf, const wchar_t* k, const std::wstring& v) {
    buf += k;
    buf += L'=';
    buf += util::Replace(v, L"\n", L" ");
    buf += L'\n';
}
void PutI(std::wstring& buf, const wchar_t* k, int v) { PutS(buf, k, std::to_wstring(v)); }
void PutB(std::wstring& buf, const wchar_t* k, bool v) { PutS(buf, k, v ? L"1" : L"0"); }

}  // namespace

void Settings::Save(const std::wstring& path) const {
    std::wstring b;
    PutI(b, L"lang", lang);
    PutS(b, L"outDir", outDir);
    PutS(b, L"outputTemplate", outputTemplate);
    PutB(b, L"playlistFolder", playlistFolder);
    PutI(b, L"qualityPreset", qualityPreset);
    PutI(b, L"resCap", resCap);
    PutI(b, L"container", container);
    PutB(b, L"recode", recode);
    PutB(b, L"useCustomFormat", useCustomFormat);
    PutS(b, L"customFormat", customFormat);
    PutB(b, L"audioOnly", audioOnly);
    PutI(b, L"audioFormat", audioFormat);
    PutI(b, L"audioQuality", audioQuality);
    PutI(b, L"cookieMode", cookieMode);
    PutS(b, L"cookieFile", cookieFile);
    PutI(b, L"browserIndex", browserIndex);
    PutS(b, L"browserProfile", browserProfile);
    PutB(b, L"writeSubs", writeSubs);
    PutB(b, L"autoSubs", autoSubs);
    PutB(b, L"embedSubs", embedSubs);
    PutB(b, L"convertSrt", convertSrt);
    PutS(b, L"subLangs", subLangs);
    PutB(b, L"embedThumb", embedThumb);
    PutB(b, L"embedMeta", embedMeta);
    PutB(b, L"embedChapters", embedChapters);
    PutB(b, L"sponsorBlock", sponsorBlock);
    PutS(b, L"sponsorCats", sponsorCats);
    PutS(b, L"playlistItems", playlistItems);
    PutB(b, L"useArchive", useArchive);
    PutB(b, L"ignoreErrors", ignoreErrors);
    PutB(b, L"politeMode", politeMode);
    PutB(b, L"noOverwrites", noOverwrites);
    PutS(b, L"proxy", proxy);
    PutS(b, L"rateLimit", rateLimit);
    PutI(b, L"concurrentFrags", concurrentFrags);
    PutI(b, L"retries", retries);
    PutS(b, L"extraArgs", extraArgs);
    PutB(b, L"autoScroll", autoScroll);
    PutI(b, L"winW", winW);
    PutI(b, L"winH", winH);

    std::string utf8 = util::WToUtf8(b);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(h, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
    CloseHandle(h);
}

void Settings::Load(const std::wstring& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;

    std::string raw;
    char buf[8192];
    DWORD got = 0;
    while (ReadFile(h, buf, sizeof(buf), &got, nullptr) && got > 0)
        raw.append(buf, got);
    CloseHandle(h);

    std::map<std::wstring, std::wstring> kv;
    for (auto& line : util::SplitLines(util::Utf8ToW(raw))) {
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;
        kv[line.substr(0, eq)] = line.substr(eq + 1);
    }

    auto S = [&](const wchar_t* k, std::wstring& dst) {
        auto it = kv.find(k);
        if (it != kv.end()) dst = it->second;
    };
    auto B = [&](const wchar_t* k, bool& dst) {
        auto it = kv.find(k);
        if (it != kv.end()) dst = (it->second == L"1");
    };
    // 下标类的值必须夹在有效范围里，否则旧配置会让下拉框越界读到野数据
    auto I = [&](const wchar_t* k, int& dst, int lo, int hi) {
        auto it = kv.find(k);
        if (it == kv.end()) return;
        try {
            int v = std::stoi(it->second);
            if (v >= lo && v <= hi) dst = v;
        } catch (...) {}
    };

    I(L"lang", lang, 0, i18n::kLangCount - 1);
    S(L"outDir", outDir);
    S(L"outputTemplate", outputTemplate);
    B(L"playlistFolder", playlistFolder);
    I(L"qualityPreset", qualityPreset, 0, QUALITY_PRESET_COUNT - 1);
    I(L"resCap", resCap, 0, RES_CAP_COUNT - 1);
    I(L"container", container, 0, CONTAINER_COUNT - 1);
    B(L"recode", recode);
    B(L"useCustomFormat", useCustomFormat);
    S(L"customFormat", customFormat);
    B(L"audioOnly", audioOnly);
    I(L"audioFormat", audioFormat, 0, AUDIO_FORMAT_COUNT - 1);
    I(L"audioQuality", audioQuality, 0, 10);
    I(L"cookieMode", cookieMode, 0, 2);
    S(L"cookieFile", cookieFile);
    I(L"browserIndex", browserIndex, 0, BROWSER_COUNT - 1);
    S(L"browserProfile", browserProfile);
    B(L"writeSubs", writeSubs);
    B(L"autoSubs", autoSubs);
    B(L"embedSubs", embedSubs);
    B(L"convertSrt", convertSrt);
    S(L"subLangs", subLangs);
    B(L"embedThumb", embedThumb);
    B(L"embedMeta", embedMeta);
    B(L"embedChapters", embedChapters);
    B(L"sponsorBlock", sponsorBlock);
    S(L"sponsorCats", sponsorCats);
    S(L"playlistItems", playlistItems);
    B(L"useArchive", useArchive);
    B(L"ignoreErrors", ignoreErrors);
    B(L"politeMode", politeMode);
    B(L"noOverwrites", noOverwrites);
    S(L"proxy", proxy);
    S(L"rateLimit", rateLimit);
    I(L"concurrentFrags", concurrentFrags, 1, 32);
    I(L"retries", retries, 0, 100);
    S(L"extraArgs", extraArgs);
    B(L"autoScroll", autoScroll);
    I(L"winW", winW, 0, 20000);
    I(L"winH", winH, 0, 20000);
}
