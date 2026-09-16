// theme.h —— 暗色配色、DPI 缩放、自绘按钮和进度条
#pragma once

#include "util.h"

#include <commctrl.h>

namespace theme {

// ---- 配色 ----
struct Palette {
    COLORREF bg        = RGB(0x16, 0x18, 0x1D);  // 窗口底
    COLORREF card      = RGB(0x1E, 0x21, 0x28);  // 分组卡片
    COLORREF cardHi    = RGB(0x26, 0x2A, 0x33);  // 卡片上的次级块
    COLORREF border    = RGB(0x2F, 0x34, 0x3E);
    COLORREF input     = RGB(0x12, 0x14, 0x19);  // 输入框底
    COLORREF text      = RGB(0xE6, 0xE8, 0xEC);
    COLORREF textDim   = RGB(0x97, 0x9E, 0xAA);
    COLORREF accent    = RGB(0xE8, 0x45, 0x3C);  // 主色
    COLORREF accentHi  = RGB(0xF2, 0x5C, 0x53);
    COLORREF accentDn  = RGB(0xC9, 0x36, 0x2E);
    COLORREF ok        = RGB(0x5C, 0xCF, 0xA8);
    COLORREF warn      = RGB(0xE2, 0xC9, 0x7A);
    COLORREF err       = RGB(0xF3, 0x7B, 0x72);
    COLORREF info      = RGB(0x6C, 0xB6, 0xFF);
};
extern Palette P;

// ---- 生命周期 ----
void Init();      // GDI+ 启动 + 让系统按暗色渲染通用控件
void Shutdown();

// ---- DPI ----
void SetDpi(UINT dpi);
UINT Dpi();
int  S(int px);   // 96dpi 下的像素值 -> 当前 DPI

// ---- 字体 ----
enum class FontRole { Body, Bold, Small, Title, Mono };
HFONT Font(FontRole r);
void  RebuildFonts();   // DPI 或界面语言变化后调用
// 界面字体名。中日韩各有各的字形习惯，切换语言时要跟着换。
void  SetFontFace(const wchar_t* face);

// ---- 让通用控件走暗色 ----
void ApplyDarkToWindow(HWND h);       // 单个控件
void ApplyDarkTitleBar(HWND h);       // 标题栏
void ApplyDarkToTree(HWND root);      // 自己 + 所有子控件

// ---- GDI+ 绘图助手 ----
void FillRound(HDC dc, const RECT& r, int radius, COLORREF fill);
void FillRoundBorder(HDC dc, const RECT& r, int radius, COLORREF fill, COLORREF border,
                     float borderWidth = 1.0f);
void FillRect(HDC dc, const RECT& r, COLORREF c);

// ---- 自绘按钮 ----
// 样式存在按钮的 GWLP_USERDATA 里；父窗口把 WM_DRAWITEM 转给 DrawButton。
enum class BtnStyle { Primary, Secondary, Ghost, Danger };
// onBackground: 按钮直接放在窗口底色上（不在卡片里），圆角外要涂 bg 而不是 card
HWND CreateButton(HWND parent, int id, const wchar_t* text, BtnStyle style,
                  bool onBackground = false);
void DrawButton(LPDRAWITEMSTRUCT di);

// ---- 自绘进度条 ----
extern const wchar_t* kProgressClass;
void RegisterProgressClass(HINSTANCE inst);
HWND CreateProgress(HWND parent, int id);
// pct<0 表示不确定状态（走马灯）
void SetProgress(HWND h, double pct, const std::wstring& centerText);

// ---- 自绘分段标签页 ----
extern const wchar_t* kTabStripClass;
void RegisterTabStripClass(HINSTANCE inst);
HWND CreateTabStrip(HWND parent, int id, const std::vector<std::wstring>& labels);
int  TabStripSel(HWND h);
void TabStripSetSel(HWND h, int i);
void TabStripSetLabels(HWND h, const std::vector<std::wstring>& labels);  // 切换语言时用
// 选中项变化时给父窗口发 WM_COMMAND，通知码用这个
constexpr WORD TABSTRIP_SELCHANGE = 0x9001;

}  // namespace theme
