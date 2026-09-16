#include "fmtpicker.h"

#include "i18n.h"
#include "json.h"
#include "resource.h"
#include "theme.h"

#include <commctrl.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <algorithm>

// ---------------------------------------------------------------- 解析

namespace {

std::wstring W(const std::string& s) { return util::Utf8ToW(s); }

std::wstring NumOrDash(double v, const wchar_t* suffix = L"") {
    if (v <= 0) return L"—";
    return util::Format(L"%.0f%s", v, suffix);
}

}  // namespace

ProbeResult ParseProbeJson(const std::string& jsonText) {
    ProbeResult pr;
    json::Value root;
    if (!json::Parse(jsonText, root)) {
        pr.error = i18n::T(S_FP_ERR_JSON);
        return pr;
    }

    // 播放列表的 -J 是 {_type:"playlist", entries:[...]}，取第一条来看格式
    const json::Value* v = &root;
    if (root.getStr("_type") == "playlist") {
        const json::Array* entries = root.getArr("entries");
        if (!entries || entries->empty()) {
            pr.error = i18n::T(S_FP_ERR_EMPTYPL);
            return pr;
        }
        v = &(*entries)[0];
    }

    pr.title = W(v->getStr("title"));
    pr.uploader = W(v->getStr("uploader"));
    pr.duration = (int)v->getNum("duration");

    const json::Array* fa = v->getArr("formats");
    if (!fa) {
        pr.error = i18n::T(S_FP_ERR_NOFMT);
        return pr;
    }

    for (const json::Value& f : *fa) {
        std::string ext = f.getStr("ext");
        if (ext == "mhtml") continue;  // 故事板缩略图，不是能下的流

        FormatRow r;
        r.id = W(f.getStr("format_id"));
        r.ext = W(ext);

        double w = f.getNum("width"), h = f.getNum("height");
        std::string vc = f.getStr("vcodec", "none");
        std::string ac = f.getStr("acodec", "none");
        r.videoOnly = (ac == "none" || ac.empty());
        r.audioOnly = (vc == "none" || vc.empty());

        if (r.audioOnly) {
            r.resolution = i18n::T(S_FP_AUDIOONLY);
        } else if (w > 0 && h > 0) {
            r.resolution = util::Format(L"%.0f×%.0f", w, h);
        } else {
            r.resolution = W(f.getStr("resolution", "—"));
        }

        double fps = f.getNum("fps");
        r.fps = fps > 0 ? util::Format(L"%.0f", fps) : L"—";
        r.vcodec = r.audioOnly ? L"—" : W(vc);
        r.acodec = r.videoOnly ? L"—" : W(ac);

        double tbr = f.getNum("tbr");
        if (tbr <= 0) tbr = f.getNum("vbr") + f.getNum("abr");
        r.tbr = NumOrDash(tbr, L"k");

        double size = f.getNum("filesize");
        if (size <= 0) size = f.getNum("filesize_approx");
        r.size = size > 0 ? util::FormatBytes(size) : L"—";

        std::string note = f.getStr("format_note");
        if (note.empty()) note = f.getStr("format");
        r.note = W(note);
        if (r.videoOnly && !r.audioOnly) r.note += i18n::T(S_FP_NOAUDIO);

        r.sortKey = h * 100000.0 + tbr;
        pr.formats.push_back(std::move(r));
    }

    // yt-dlp 是从差到好排的，反过来让最好的在最上面
    std::stable_sort(pr.formats.begin(), pr.formats.end(),
                     [](const FormatRow& a, const FormatRow& b) {
                         return a.sortKey > b.sortKey;
                     });

    pr.ok = !pr.formats.empty();
    if (!pr.ok) pr.error = i18n::T(S_FP_ERR_NOPARSE);
    return pr;
}

// ---------------------------------------------------------------- 弹窗

namespace {

const wchar_t* kPickerClass = L"YtDlpGuiFmtPicker";

struct PickerData {
    const ProbeResult* pr = nullptr;
    std::wstring result;
    bool done = false;
    HWND list = nullptr, ok = nullptr, cancel = nullptr, autoAudio = nullptr;
    HWND titleLbl = nullptr;
};

void LayoutPicker(HWND h, PickerData* d) {
    RECT rc;
    GetClientRect(h, &rc);
    const int pad = theme::S(14);
    const int btnH = theme::S(32);
    const int lblH = theme::S(40);

    MoveWindow(d->titleLbl, pad, pad, rc.right - pad * 2, lblH, TRUE);

    int listTop = pad + lblH + theme::S(6);
    int listBottom = rc.bottom - pad - btnH - theme::S(10);
    MoveWindow(d->list, pad, listTop, rc.right - pad * 2, listBottom - listTop, TRUE);

    int by = rc.bottom - pad - btnH;
    MoveWindow(d->autoAudio, pad, by + theme::S(5), theme::S(280), theme::S(22), TRUE);
    int bw = theme::S(100);
    MoveWindow(d->cancel, rc.right - pad - bw, by, bw, btnH, TRUE);
    MoveWindow(d->ok, rc.right - pad - bw * 2 - theme::S(8), by, bw, btnH, TRUE);
}

void FillList(PickerData* d) {
    HWND lv = d->list;
    struct Col { StrId t; int w; };
    const Col cols[] = {
        {S_FP_COL_ID, 90},      {S_FP_COL_RES, 120},    {S_FP_COL_FPS, 55},
        {S_FP_COL_VCODEC, 140}, {S_FP_COL_ACODEC, 130}, {S_FP_COL_BITRATE, 90},
        {S_FP_COL_SIZE, 95},    {S_FP_COL_EXT, 90},     {S_FP_COL_NOTE, 200},
    };
    for (int i = 0; i < (int)std::size(cols); ++i) {
        LVCOLUMNW c{};
        c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        c.pszText = (LPWSTR)i18n::T(cols[i].t);
        c.cx = theme::S(cols[i].w);
        c.iSubItem = i;
        ListView_InsertColumn(lv, i, &c);
    }

    for (size_t i = 0; i < d->pr->formats.size(); ++i) {
        const FormatRow& f = d->pr->formats[i];
        LVITEMW it{};
        it.mask = LVIF_TEXT | LVIF_PARAM;
        it.iItem = (int)i;
        it.pszText = (LPWSTR)f.id.c_str();
        it.lParam = (LPARAM)i;
        int row = ListView_InsertItem(lv, &it);
        ListView_SetItemText(lv, row, 1, (LPWSTR)f.resolution.c_str());
        ListView_SetItemText(lv, row, 2, (LPWSTR)f.fps.c_str());
        ListView_SetItemText(lv, row, 3, (LPWSTR)f.vcodec.c_str());
        ListView_SetItemText(lv, row, 4, (LPWSTR)f.acodec.c_str());
        ListView_SetItemText(lv, row, 5, (LPWSTR)f.tbr.c_str());
        ListView_SetItemText(lv, row, 6, (LPWSTR)f.size.c_str());
        ListView_SetItemText(lv, row, 7, (LPWSTR)f.ext.c_str());
        ListView_SetItemText(lv, row, 8, (LPWSTR)f.note.c_str());
    }
    if (!d->pr->formats.empty()) {
        ListView_SetItemState(lv, 0, LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void Accept(HWND h, PickerData* d) {
    int sel = ListView_GetNextItem(d->list, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)d->pr->formats.size()) return;
    const FormatRow& f = d->pr->formats[sel];

    bool wantAudio = Button_GetCheck(d->autoAudio) == BST_CHECKED;
    if (f.videoOnly && !f.audioOnly && wantAudio) {
        // 纯视频流要自己配音轨，否则下出来是哑片
        d->result = f.id + L"+bestaudio/" + f.id;
    } else {
        d->result = f.id;
    }
    d->done = true;
    DestroyWindow(h);
}

LRESULT CALLBACK PickerProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    auto* d = (PickerData*)GetWindowLongPtrW(h, GWLP_USERDATA);
    switch (msg) {
        case WM_ERASEBKGND: {
            RECT rc;
            GetClientRect(h, &rc);
            theme::FillRect((HDC)wp, rc, theme::P.bg);
            return 1;
        }
        case WM_CTLCOLORSTATIC: {
            // 用 DC_BRUSH 省掉自己管理画刷生命周期
            SetTextColor((HDC)wp, theme::P.text);
            SetBkColor((HDC)wp, theme::P.bg);
            SetDCBrushColor((HDC)wp, theme::P.bg);
            return (LRESULT)GetStockObject(DC_BRUSH);
        }
        case WM_SIZE:
            if (d) LayoutPicker(h, d);
            return 0;
        case WM_DRAWITEM:
            theme::DrawButton((LPDRAWITEMSTRUCT)lp);
            return TRUE;
        case WM_NOTIFY: {
            auto* nm = (LPNMHDR)lp;
            if (d && nm->idFrom == IDC_FMTLIST && nm->code == NM_DBLCLK) {
                Accept(h, d);
                return 0;
            }
            break;
        }
        case WM_COMMAND:
            if (!d) break;
            switch (LOWORD(wp)) {
                case IDC_FMTOK: Accept(h, d); return 0;
                case IDC_FMTCANCEL: DestroyWindow(h); return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(h);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

}  // namespace

void RegisterFormatPickerClass(HINSTANCE inst) {
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = PickerProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kPickerClass;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APPICON));
    RegisterClassExW(&wc);
}

std::wstring ShowFormatPicker(HWND owner, const ProbeResult& pr) {
    PickerData d;
    d.pr = &pr;

    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW(owner, GWLP_HINSTANCE);
    int w = theme::S(1020), ht = theme::S(600);
    RECT orc;
    GetWindowRect(owner, &orc);
    int x = orc.left + ((orc.right - orc.left) - w) / 2;
    int y = orc.top + ((orc.bottom - orc.top) - ht) / 2;

    HWND h = CreateWindowExW(WS_EX_DLGMODALFRAME, kPickerClass, i18n::T(S_FP_TITLE),
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                                 WS_CLIPCHILDREN,
                             x, y, w, ht, owner, nullptr, inst, nullptr);
    if (!h) return {};
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)&d);
    theme::ApplyDarkTitleBar(h);

    std::wstring header = pr.title;
    if (pr.duration > 0)
        header += L"    " + std::wstring(i18n::T(S_FP_DURATION)) +
                  util::FormatDuration(pr.duration);
    if (!pr.uploader.empty())
        header += L"    " + std::wstring(i18n::T(S_FP_UPLOADER)) + pr.uploader;
    header += L"\n";
    header += i18n::T(S_FP_HINT);

    d.titleLbl = CreateWindowExW(0, L"STATIC", header.c_str(),
                                 WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, nullptr, inst, nullptr);
    SendMessageW(d.titleLbl, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Body), TRUE);

    d.list = CreateWindowExW(0, WC_LISTVIEWW, L"",
                             WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT |
                                 LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                             0, 0, 10, 10, h, (HMENU)IDC_FMTLIST, inst, nullptr);
    ListView_SetExtendedListViewStyle(d.list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER |
                                                  LVS_EX_GRIDLINES);
    SetWindowTheme(d.list, L"DarkMode_Explorer", nullptr);
    ListView_SetBkColor(d.list, theme::P.input);
    ListView_SetTextBkColor(d.list, theme::P.input);
    ListView_SetTextColor(d.list, theme::P.text);
    SendMessageW(d.list, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Body), TRUE);
    if (HWND hdr = ListView_GetHeader(d.list))
        SetWindowTheme(hdr, L"DarkMode_ItemsView", nullptr);

    d.autoAudio = CreateWindowExW(0, L"BUTTON", i18n::T(S_FP_AUTOAUDIO),
                                  WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                  0, 0, 10, 10, h, (HMENU)IDC_FMTAUDIO, inst, nullptr);
    Button_SetCheck(d.autoAudio, BST_CHECKED);
    SendMessageW(d.autoAudio, WM_SETFONT, (WPARAM)theme::Font(theme::FontRole::Body), TRUE);
    theme::ApplyDarkToWindow(d.autoAudio);

    d.ok = theme::CreateButton(h, IDC_FMTOK, i18n::T(S_FP_USE), theme::BtnStyle::Primary);
    d.cancel =
        theme::CreateButton(h, IDC_FMTCANCEL, i18n::T(S_FP_CANCEL), theme::BtnStyle::Secondary);

    FillList(&d);
    LayoutPicker(h, &d);
    ShowWindow(h, SW_SHOW);

    // 自己跑一个模态循环：屏蔽 owner，直到窗口销毁
    EnableWindow(owner, FALSE);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            DestroyWindow(h);
            continue;
        }
        if (!IsDialogMessageW(h, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    return d.done ? d.result : std::wstring();
}
