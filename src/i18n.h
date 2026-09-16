// i18n.h —— 多语言支持：简体中文 / English / 繁體中文 / 日本語 / 한국어
#pragma once

#include "strings.inc"

// 词条 ID。和 i18n.cpp 里的查表数组同出于 strings.inc，不会错位。
enum StrId {
#define X(id, zh, en, tw, ja, ko) id,
    STRING_TABLE(X)
#undef X
    S_COUNT
};

namespace i18n {

// 这个顺序 = settings.ini 里存的下标 = strings.inc 的列顺序。
// 不要重排，否则老配置文件会串语言、翻译表会整体错位。
// 下拉框里给用户看的顺序另有一份（见下面的 LangAtDisplayIndex）。
enum class Lang { ZhCN = 0, En, ZhTW, Ja, Ko, COUNT };
constexpr int kLangCount = (int)Lang::COUNT;

// 按系统界面语言挑一个默认值（认不出来就用英文）
Lang DetectSystem();

Lang Current();
void SetLang(Lang l);
void SetLangIndex(int i);
int  CurrentIndex();

// 取当前语言的文案
const wchar_t* T(StrId id);
// 取指定语言的文案（语言下拉框自身要用）
const wchar_t* T(StrId id, Lang l);

// 当前语言适合的界面字体。中日韩各有各的字形习惯，混用会很难看。
const wchar_t* UiFontFace();

// ---- 语言下拉框 ----
// 显示顺序和存储下标是两回事：English 排在最前，但它的 Lang 值仍是 1。
Lang LangAtDisplayIndex(int i);   // 下拉框第 i 项 -> 语言
int  DisplayIndexOf(Lang l);      // 语言 -> 下拉框第几项
// 下拉框里显示的语言名，各自用母语写法
const wchar_t* LangDisplayName(Lang l);

}  // namespace i18n
