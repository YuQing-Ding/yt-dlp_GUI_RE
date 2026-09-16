#include "i18n.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace i18n {

namespace {

// 行 = 词条，列 = 语言。列顺序必须和 enum Lang 一致。
const wchar_t* const kTable[S_COUNT][kLangCount] = {
#define X(id, zh, en, tw, ja, ko) {zh, en, tw, ja, ko},
    STRING_TABLE(X)
#undef X
};

Lang g_lang = Lang::En;

// 每种语言用各自系统自带的界面字体。
// 用简中字体渲染日文会出现字形不对（比如「直」「骨」），渲染韩文则直接缺字。
const wchar_t* const kFonts[kLangCount] = {
    L"Microsoft YaHei UI",    // 简体中文
    L"Segoe UI",              // English
    L"Microsoft JhengHei UI", // 繁體中文
    L"Yu Gothic UI",          // 日本語
    L"Malgun Gothic",         // 한국어
};

// 语言名用各自的母语写法，这样任何人都能在列表里认出自己的语言。
// 下标对应 enum Lang，不是下拉框顺序。
const wchar_t* const kLangNames[kLangCount] = {
    L"简体中文", L"English", L"繁體中文", L"日本語", L"한국어",
};

// 下拉框的显示顺序：English 在最前，其余按原顺序。
// 和 enum Lang 分开，是为了让 settings.ini 里存的下标保持稳定。
const Lang kDisplayOrder[kLangCount] = {
    Lang::En, Lang::ZhCN, Lang::ZhTW, Lang::Ja, Lang::Ko,
};

}  // namespace

Lang DetectSystem() {
    LANGID id = GetUserDefaultUILanguage();
    WORD primary = PRIMARYLANGID(id);
    WORD sub = SUBLANGID(id);

    if (primary == LANG_CHINESE) {
        // 简体：中国大陆、新加坡；繁体：台湾、香港、澳门
        if (sub == SUBLANG_CHINESE_SIMPLIFIED || sub == SUBLANG_CHINESE_SINGAPORE)
            return Lang::ZhCN;
        return Lang::ZhTW;
    }
    if (primary == LANG_JAPANESE) return Lang::Ja;
    if (primary == LANG_KOREAN) return Lang::Ko;
    return Lang::En;
}

Lang Current() { return g_lang; }

void SetLang(Lang l) {
    if ((int)l >= 0 && (int)l < kLangCount) g_lang = l;
}

void SetLangIndex(int i) { SetLang((Lang)i); }

int CurrentIndex() { return (int)g_lang; }

const wchar_t* T(StrId id) { return T(id, g_lang); }

const wchar_t* T(StrId id, Lang l) {
    if (id < 0 || id >= S_COUNT) return L"";
    int li = (int)l;
    if (li < 0 || li >= kLangCount) li = (int)Lang::En;
    return kTable[id][li];
}

const wchar_t* UiFontFace() { return kFonts[(int)g_lang]; }

Lang LangAtDisplayIndex(int i) {
    if (i < 0 || i >= kLangCount) return Lang::En;
    return kDisplayOrder[i];
}

int DisplayIndexOf(Lang l) {
    for (int i = 0; i < kLangCount; ++i)
        if (kDisplayOrder[i] == l) return i;
    return 0;
}

const wchar_t* LangDisplayName(Lang l) {
    int i = (int)l;
    if (i < 0 || i >= kLangCount) return L"";
    return kLangNames[i];
}

}  // namespace i18n
