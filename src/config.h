// config.h —— 全部可调项集中在一个结构里，存成 UTF-8 的 key=value 文件
#pragma once

#include "i18n.h"

#include <string>
#include <vector>

struct Settings {
    // 界面语言（i18n::Lang 的下标）
    int  lang = 1;   // 默认 English，启动时会按系统语言覆盖

    // 输出
    std::wstring outDir;
    std::wstring outputTemplate;
    bool playlistFolder = true;

    // 画质
    int  qualityPreset   = 0;      // QUALITY_PRESETS 下标
    int  resCap          = 0;      // RES_CAPS 下标，0 = 不限
    int  container       = 0;      // CONTAINERS 下标
    bool recode          = false;  // true=重编码(慢，画质有损)  false=仅重封装
    bool useCustomFormat = false;
    std::wstring customFormat;

    // 音频
    bool audioOnly     = false;
    int  audioFormat   = 0;        // AUDIO_FORMATS 下标
    int  audioQuality  = 0;        // 0 最好 .. 10 最差

    // Cookies
    int  cookieMode = 0;           // 0=不用 1=文件 2=浏览器
    std::wstring cookieFile;
    int  browserIndex = 0;         // BROWSERS 下标
    std::wstring browserProfile;

    // 字幕
    bool writeSubs  = false;
    bool autoSubs   = false;
    bool embedSubs  = true;
    bool convertSrt = true;
    std::wstring subLangs;         // 空则按界面语言取默认值

    // 嵌入
    bool embedThumb    = true;
    bool embedMeta     = true;
    bool embedChapters = true;

    // SponsorBlock
    bool sponsorBlock = false;
    std::wstring sponsorCats = L"sponsor,selfpromo,interaction";

    // 播放列表 / 行为
    std::wstring playlistItems;
    bool useArchive   = true;
    bool ignoreErrors = true;
    bool politeMode   = true;
    bool noOverwrites = true;

    // 网络
    std::wstring proxy;
    std::wstring rateLimit;
    int concurrentFrags = 4;
    int retries         = 10;

    // 杂项
    std::wstring extraArgs;
    bool autoScroll = true;

    // 窗口
    int winW = 0, winH = 0;

    // 拼装好的 yt-dlp 参数里不含 URL；URL 由调用方追加。
    std::vector<std::wstring> BuildArgs(const std::wstring& ytDlpPath) const;

    void Load(const std::wstring& path);
    void Save(const std::wstring& path) const;
};

// ---- 下拉框选项表（UI 和参数拼装共用同一份，避免两边对不上）----
//
// 标签分两类：需要翻译的用 StrId，技术性的（MP4、4K 2160p）用 literal。
// label == S_COUNT 表示这一项用 literal。

struct QualityPreset {
    StrId label;
    const wchar_t* sortOrder;   // 传给 -S
};

struct NamedValue {
    StrId label;
    const wchar_t* literal;
    const wchar_t* value;
};

// 取该项当前语言下应显示的文字
const wchar_t* NVLabel(const NamedValue& n);

extern const QualityPreset QUALITY_PRESETS[];
extern const int QUALITY_PRESET_COUNT;
extern const NamedValue RES_CAPS[];
extern const int RES_CAP_COUNT;
extern const NamedValue CONTAINERS[];
extern const int CONTAINER_COUNT;
extern const NamedValue AUDIO_FORMATS[];
extern const int AUDIO_FORMAT_COUNT;
extern const NamedValue BROWSERS[];
extern const int BROWSER_COUNT;
extern const NamedValue TEMPLATE_PRESETS[];
extern const int TEMPLATE_PRESET_COUNT;
