#include "theme.h"

#include <dwmapi.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <algorithm>
#include <string>
#include <vector>

// gdiplus.h 内部用了裸的 min/max，而我们全局开了 NOMINMAX，得先把它们补回去
namespace Gdiplus {
using std::max;
using std::min;
}  // namespace Gdiplus
#include <objidl.h>
#include <gdiplus.h>

namespace theme {

Palette P;

namespace {

ULONG_PTR g_gdiplusToken = 0;
UINT g_dpi = 96;
HFONT g_fonts[5] = {};
std::wstring g_face = L"Segoe UI";

Gdiplus::Color ToG(COLORREF c, BYTE a = 255) {
    return Gdiplus::Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
}

Gdiplus::GraphicsPath* RoundPath(const RECT& r, int radius) {
    auto* p = new Gdiplus::GraphicsPath();
    float x = (float)r.left, y = (float)r.top;
    float w = (float)(r.right - r.left), h = (float)(r.bottom - r.top);
    float d = (float)radius * 2.0f;
    if (d > w) d = w;
    if (d > h) d = h;
    if (d <= 0.5f) {
        p->AddRectangle(Gdiplus::RectF(x, y, w, h));
        return p;
    }
    p->AddArc(x, y, d, d, 180.0f, 90.0f);
    p->AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    p->AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    p->AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    p->CloseFigure();
    return p;
}

// ---- 暗色模式：用的是 uxtheme 没导出名字的序号函数 ----
enum PreferredAppMode { Default_, AllowDark, ForceDark, ForceLight, Max_ };
using FnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode);
using FnAllowDarkModeForWindow = BOOL(WINAPI*)(HWND, BOOL);
using FnRefreshImmersiveColorPolicyState = void(WINAPI*)();

FnSetPreferredAppMode g_setPreferredAppMode = nullptr;
FnAllowDarkModeForWindow g_allowDarkModeForWindow = nullptr;

void InitDarkModeApi() {
    HMODULE ux = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!ux) return;
    // 序号 135 在 1903+ 是 SetPreferredAppMode，133 是 AllowDarkModeForWindow，
    // 104 是 RefreshImmersiveColorPolicyState。都没有导出名，只能按序号取。
    g_setPreferredAppMode =
        (FnSetPreferredAppMode)GetProcAddress(ux, MAKEINTRESOURCEA(135));
    g_allowDarkModeForWindow =
        (FnAllowDarkModeForWindow)GetProcAddress(ux, MAKEINTRESOURCEA(133));
    auto refresh =
        (FnRefreshImmersiveColorPolicyState)GetProcAddress(ux, MAKEINTRESOURCEA(104));

    if (g_setPreferredAppMode) g_setPreferredAppMode(ForceDark);
    if (refresh) refresh();
}

// ---- 自绘按钮 ----
struct ButtonState {
    BtnStyle style = BtnStyle::Secondary;
    bool hover = false;
    bool onBackground = false;
};

LRESULT CALLBACK ButtonSubclass(HWND h, UINT msg, WPARAM wp, LPARAM lp,
                                UINT_PTR, DWORD_PTR ref) {
    auto* st = (ButtonState*)ref;
    switch (msg) {
        case WM_MOUSEMOVE:
            if (!st->hover) {
                st->hover = true;
                TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, h, 0};
                TrackMouseEvent(&tme);
                InvalidateRect(h, nullptr, FALSE);
            }
            break;
        case WM_MOUSELEAVE:
            st->hover = false;
            InvalidateRect(h, nullptr, FALSE);
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(h, ButtonSubclass, 1);
            delete st;
            break;
    }
    return DefSubclassProc(h, msg, wp, lp);
}

// ---- 自绘进度条 ----
struct ProgressState {
    double pct = 0;          // <0 表示走马灯
    std::wstring text;
    int marquee = 0;
};

LRESULT CALLBACK ProgressProc(HWND h, UINT msg, WPARAM wp, LPARAM lp);

// ---- 自绘标签页 ----
struct TabState {
    std::vector<std::wstring> labels;
    int sel = 0;
    int hover = -1;
};

LRESULT CALLBACK TabStripProc(HWND h, UINT msg, WPARAM wp, LPARAM lp);

}  // namespace

// ---------------------------------------------------------------- 生命周期

void Init() {
    Gdiplus::GdiplusStartupInput in;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &in, nullptr);
    InitDarkModeApi();
    RebuildFonts();
}

void Shutdown() {
    for (HFONT& f : g_fonts) {
        if (f) { DeleteObject(f); f = nullptr; }
    }
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

// ---------------------------------------------------------------- DPI / 字体

void SetDpi(UINT dpi) { g_dpi = dpi ? dpi : 96; }
UINT Dpi() { return g_dpi; }
int  S(int px) { return MulDiv(px, (int)g_dpi, 96); }

void RebuildFonts() {
    for (HFONT& f : g_fonts) {
        if (f) { DeleteObject(f); f = nullptr; }
    }
    auto mk = [&](int px, int weight, const wchar_t* face) {
        LOGFONTW lf{};
        lf.lfHeight = -S(px);
        lf.lfWeight = weight;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfQuality = CLEARTYPE_QUALITY;
        wcscpy_s(lf.lfFaceName, face);
        return CreateFontIndirectW(&lf);
    };
    const wchar_t* f = g_face.c_str();
    g_fonts[(int)FontRole::Body]  = mk(14, FW_NORMAL,   f);
    g_fonts[(int)FontRole::Bold]  = mk(14, FW_SEMIBOLD, f);
    g_fonts[(int)FontRole::Small] = mk(12, FW_NORMAL,   f);
    g_fonts[(int)FontRole::Title] = mk(15, FW_SEMIBOLD, f);
    // 日志固定等宽；CJK 字符由系统字体链接兜底
    g_fonts[(int)FontRole::Mono]  = mk(13, FW_NORMAL,   L"Consolas");
}

void SetFontFace(const wchar_t* face) {
    if (face && *face) g_face = face;
}

HFONT Font(FontRole r) { return g_fonts[(int)r]; }

// ---------------------------------------------------------------- 暗色

void ApplyDarkToWindow(HWND h) {
    if (g_allowDarkModeForWindow) g_allowDarkModeForWindow(h, TRUE);
    // 让滚动条、下拉箭头这些非客户区也走暗色
    SetWindowTheme(h, L"DarkMode_Explorer", nullptr);
}

void ApplyDarkTitleBar(HWND h) {
    BOOL dark = TRUE;
    // 属性号 20 是 Win10 2004+ 的正式值，19 是早期预览版的
    if (FAILED(DwmSetWindowAttribute(h, 20, &dark, sizeof(dark))))
        DwmSetWindowAttribute(h, 19, &dark, sizeof(dark));
}

void ApplyDarkToTree(HWND root) {
    ApplyDarkToWindow(root);
    EnumChildWindows(root, [](HWND c, LPARAM) -> BOOL {
        ApplyDarkToWindow(c);
        return TRUE;
    }, 0);
}

// ---------------------------------------------------------------- 绘图

void FillRect(HDC dc, const RECT& r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    ::FillRect(dc, &r, b);
    DeleteObject(b);
}

void FillRound(HDC dc, const RECT& r, int radius, COLORREF fill) {
    Gdiplus::Graphics g(dc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Gdiplus::GraphicsPath* p = RoundPath(r, radius);
    Gdiplus::SolidBrush b(ToG(fill));
    g.FillPath(&b, p);
    delete p;
}

void FillRoundBorder(HDC dc, const RECT& r, int radius, COLORREF fill, COLORREF border,
                     float bw) {
    Gdiplus::Graphics g(dc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    // 描边是压着路径中线画的，往里缩半个线宽才不会被裁掉
    RECT rr = r;
    Gdiplus::GraphicsPath* p = RoundPath(rr, radius);
    Gdiplus::SolidBrush b(ToG(fill));
    g.FillPath(&b, p);
    Gdiplus::Pen pen(ToG(border), bw);
    Gdiplus::RectF inset((float)rr.left + bw / 2, (float)rr.top + bw / 2,
                         (float)(rr.right - rr.left) - bw,
                         (float)(rr.bottom - rr.top) - bw);
    RECT ir{(LONG)inset.X, (LONG)inset.Y, (LONG)(inset.X + inset.Width),
            (LONG)(inset.Y + inset.Height)};
    Gdiplus::GraphicsPath* p2 = RoundPath(ir, radius);
    g.DrawPath(&pen, p2);
    delete p2;
    delete p;
}

// ---------------------------------------------------------------- 按钮

HWND CreateButton(HWND parent, int id, const wchar_t* text, BtnStyle style,
                  bool onBackground) {
    HWND h = CreateWindowExW(0, L"BUTTON", text,
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                             0, 0, 10, 10, parent, (HMENU)(INT_PTR)id,
                             (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), nullptr);
    if (!h) return nullptr;
    auto* st = new ButtonState{style, false, onBackground};
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)st);
    SetWindowSubclass(h, ButtonSubclass, 1, (DWORD_PTR)st);
    SendMessageW(h, WM_SETFONT, (WPARAM)Font(FontRole::Bold), TRUE);
    return h;
}

void DrawButton(LPDRAWITEMSTRUCT di) {
    auto* st = (ButtonState*)GetWindowLongPtrW(di->hwndItem, GWLP_USERDATA);
    if (!st) return;

    bool pressed = (di->itemState & ODS_SELECTED) != 0;
    bool disabled = (di->itemState & ODS_DISABLED) != 0;
    bool focused = (di->itemState & ODS_FOCUS) != 0;

    COLORREF fill = P.cardHi, fg = P.text, bd = P.border;
    switch (st->style) {
        case BtnStyle::Primary:
            fill = pressed ? P.accentDn : (st->hover ? P.accentHi : P.accent);
            fg = RGB(255, 255, 255);
            bd = fill;
            break;
        case BtnStyle::Danger:
            fill = pressed ? RGB(0x8C, 0x2F, 0x2F)
                           : (st->hover ? RGB(0x6E, 0x2A, 0x2A) : P.cardHi);
            fg = st->hover ? RGB(255, 255, 255) : P.err;
            bd = st->hover ? RGB(0xA8, 0x3A, 0x3A) : P.border;
            break;
        case BtnStyle::Ghost:
            fill = pressed ? P.cardHi : (st->hover ? P.cardHi : P.card);
            fg = st->hover ? P.text : P.textDim;
            bd = st->hover ? P.border : P.card;
            break;
        case BtnStyle::Secondary:
        default:
            fill = pressed ? P.border : (st->hover ? RGB(0x2E, 0x33, 0x3E) : P.cardHi);
            fg = P.text;
            bd = P.border;
            break;
    }
    if (disabled) {
        fill = P.card;
        fg = RGB(0x5A, 0x60, 0x6B);
        bd = P.border;
    }

    // 先把控件矩形涂成它所在容器的底色，圆角外面才不会留黑边
    theme::FillRect(di->hDC, di->rcItem, st->onBackground ? P.bg : P.card);
    FillRoundBorder(di->hDC, di->rcItem, S(6), fill, bd, 1.0f);

    if (focused && !disabled) {
        RECT fr = di->rcItem;
        InflateRect(&fr, -S(3), -S(3));
        Gdiplus::Graphics g(di->hDC);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        Gdiplus::Pen pen(ToG(fg, 90), 1.0f);
        Gdiplus::GraphicsPath* p = RoundPath(fr, S(4));
        g.DrawPath(&pen, p);
        delete p;
    }

    wchar_t txt[256] = {};
    GetWindowTextW(di->hwndItem, txt, 255);
    SetBkMode(di->hDC, TRANSPARENT);
    SetTextColor(di->hDC, fg);
    HFONT old = (HFONT)SelectObject(di->hDC, Font(FontRole::Bold));
    RECT tr = di->rcItem;
    if (pressed) OffsetRect(&tr, 0, 1);
    DrawTextW(di->hDC, txt, -1, &tr,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(di->hDC, old);
}

// ---------------------------------------------------------------- 进度条

const wchar_t* kProgressClass = L"YtDlpGuiProgress";

namespace {

LRESULT CALLBACK ProgressProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = (ProgressState*)GetWindowLongPtrW(h, GWLP_USERDATA);
    switch (msg) {
        case WM_NCCREATE: {
            auto* s = new ProgressState();
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)s);
            return TRUE;
        }
        case WM_NCDESTROY:
            delete st;
            SetWindowLongPtrW(h, GWLP_USERDATA, 0);
            break;
        case WM_TIMER:
            if (st && st->pct < 0) {
                st->marquee = (st->marquee + S(6)) % 10000;
                InvalidateRect(h, nullptr, FALSE);
            }
            break;
        case WM_ERASEBKGND:
            return 1;  // 全部在 WM_PAINT 里双缓冲画掉
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(h, &ps);
            RECT rc;
            GetClientRect(h, &rc);
            int w = rc.right, ht = rc.bottom;

            HDC mem = CreateCompatibleDC(dc);
            HBITMAP bmp = CreateCompatibleBitmap(dc, w, ht);
            HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

            // 进度条放在窗口背景上（不在卡片里），圆角外的底色要用 bg
            theme::FillRect(mem, rc, P.bg);
            int r = ht / 2;
            FillRound(mem, rc, r, P.input);

            if (st) {
                if (st->pct >= 0) {
                    int fw = (int)(w * (std::min)(1.0, st->pct / 100.0));
                    if (fw > 2) {
                        RECT fr{0, 0, fw, ht};
                        FillRound(mem, fr, r, P.accent);
                    }
                } else {
                    // 走马灯：一段固定宽度的亮块来回跑
                    int seg = (std::max)(S(60), w / 5);
                    int span = w + seg;
                    int x = (st->marquee % span) - seg;
                    RECT fr{(std::max)(0, x), 0, (std::min)(w, x + seg), ht};
                    if (fr.right > fr.left) FillRound(mem, fr, r, P.accent);
                }
                if (!st->text.empty()) {
                    SetBkMode(mem, TRANSPARENT);
                    SetTextColor(mem, P.text);
                    HFONT old = (HFONT)SelectObject(mem, Font(FontRole::Small));
                    DrawTextW(mem, st->text.c_str(), -1, &rc,
                              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(mem, old);
                }
            }

            BitBlt(dc, 0, 0, w, ht, mem, 0, 0, SRCCOPY);
            SelectObject(mem, oldBmp);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(h, &ps);
            return 0;
        }
    }
    return DefWindowProcW(h, msg, wp, lp);
}

}  // namespace

void RegisterProgressClass(HINSTANCE inst) {
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = ProgressProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kProgressClass;
    RegisterClassExW(&wc);
}

HWND CreateProgress(HWND parent, int id) {
    return CreateWindowExW(0, kProgressClass, L"", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10,
                           parent, (HMENU)(INT_PTR)id,
                           (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), nullptr);
}

void SetProgress(HWND h, double pct, const std::wstring& text) {
    auto* st = (ProgressState*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (!st) return;
    bool wasMarquee = st->pct < 0;
    bool nowMarquee = pct < 0;
    st->pct = pct;
    st->text = text;
    if (nowMarquee && !wasMarquee) SetTimer(h, 1, 30, nullptr);
    if (!nowMarquee && wasMarquee) KillTimer(h, 1);
    InvalidateRect(h, nullptr, FALSE);
}

// ---------------------------------------------------------------- 标签页

const wchar_t* kTabStripClass = L"YtDlpGuiTabStrip";

namespace {

int TabHitTest(HWND h, int x) {
    auto* st = (TabState*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (!st || st->labels.empty()) return -1;
    RECT rc;
    GetClientRect(h, &rc);
    int n = (int)st->labels.size();
    int seg = rc.right / n;
    int i = seg > 0 ? x / seg : 0;
    return (i >= 0 && i < n) ? i : -1;
}

LRESULT CALLBACK TabStripProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = (TabState*)GetWindowLongPtrW(h, GWLP_USERDATA);
    switch (msg) {
        case WM_NCCREATE:
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR) new TabState());
            return TRUE;
        case WM_NCDESTROY:
            delete st;
            SetWindowLongPtrW(h, GWLP_USERDATA, 0);
            break;
        case WM_MOUSEMOVE: {
            int i = TabHitTest(h, GET_X_LPARAM(lp));
            if (st && st->hover != i) {
                st->hover = i;
                TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, h, 0};
                TrackMouseEvent(&tme);
                InvalidateRect(h, nullptr, FALSE);
            }
            break;
        }
        case WM_MOUSELEAVE:
            if (st && st->hover != -1) {
                st->hover = -1;
                InvalidateRect(h, nullptr, FALSE);
            }
            break;
        case WM_LBUTTONDOWN: {
            int i = TabHitTest(h, GET_X_LPARAM(lp));
            if (st && i >= 0 && i != st->sel) {
                st->sel = i;
                InvalidateRect(h, nullptr, FALSE);
                SendMessageW(GetParent(h), WM_COMMAND,
                             MAKEWPARAM(GetDlgCtrlID(h), TABSTRIP_SELCHANGE), (LPARAM)h);
            }
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(h, &ps);
            RECT rc;
            GetClientRect(h, &rc);

            HDC mem = CreateCompatibleDC(dc);
            HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
            HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

            theme::FillRect(mem, rc, P.bg);
            FillRound(mem, rc, S(8), P.card);

            if (st && !st->labels.empty()) {
                int n = (int)st->labels.size();
                int seg = rc.right / n;
                SetBkMode(mem, TRANSPARENT);
                for (int i = 0; i < n; ++i) {
                    RECT ir{i * seg, 0, (i == n - 1) ? rc.right : (i + 1) * seg, rc.bottom};
                    COLORREF fg = P.textDim;
                    if (i == st->sel) {
                        RECT pill = ir;
                        InflateRect(&pill, -S(3), -S(3));
                        FillRound(mem, pill, S(6), P.cardHi);
                        fg = P.text;
                    } else if (i == st->hover) {
                        fg = P.text;
                    }
                    SetTextColor(mem, fg);
                    HFONT old = (HFONT)SelectObject(
                        mem, Font(i == st->sel ? FontRole::Bold : FontRole::Body));
                    DrawTextW(mem, st->labels[i].c_str(), -1, &ir,
                              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS |
                                  DT_NOPREFIX);
                    SelectObject(mem, old);
                }
            }

            BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
            SelectObject(mem, oldBmp);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(h, &ps);
            return 0;
        }
    }
    return DefWindowProcW(h, msg, wp, lp);
}

}  // namespace

void RegisterTabStripClass(HINSTANCE inst) {
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = TabStripProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
    wc.lpszClassName = kTabStripClass;
    RegisterClassExW(&wc);
}

HWND CreateTabStrip(HWND parent, int id, const std::vector<std::wstring>& labels) {
    HWND h = CreateWindowExW(0, kTabStripClass, L"", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10,
                             parent, (HMENU)(INT_PTR)id,
                             (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), nullptr);
    if (!h) return nullptr;
    auto* st = (TabState*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (st) st->labels = labels;
    return h;
}

int TabStripSel(HWND h) {
    auto* st = (TabState*)GetWindowLongPtrW(h, GWLP_USERDATA);
    return st ? st->sel : 0;
}

void TabStripSetSel(HWND h, int i) {
    auto* st = (TabState*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (st && i >= 0 && i < (int)st->labels.size()) {
        st->sel = i;
        InvalidateRect(h, nullptr, FALSE);
    }
}

void TabStripSetLabels(HWND h, const std::vector<std::wstring>& labels) {
    auto* st = (TabState*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (!st || labels.empty()) return;
    st->labels = labels;
    if (st->sel >= (int)labels.size()) st->sel = 0;
    InvalidateRect(h, nullptr, FALSE);
}

}  // namespace theme
