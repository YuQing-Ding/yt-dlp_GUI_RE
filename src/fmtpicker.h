// fmtpicker.h —— 解析 yt-dlp -J 的元数据，并弹一个可视化的格式选择窗
#pragma once

#include "util.h"

#include <string>
#include <vector>

struct FormatRow {
    std::wstring id, ext, resolution, fps, vcodec, acodec, tbr, size, note;
    double sortKey = 0;    // 排序用：分辨率 * 10000 + 码率
    bool videoOnly = false;
    bool audioOnly = false;
};

struct ProbeResult {
    bool ok = false;
    std::wstring error;
    std::wstring title;
    std::wstring uploader;
    int duration = 0;
    std::vector<FormatRow> formats;
};

// 解析 yt-dlp -J 输出（UTF-8）
ProbeResult ParseProbeJson(const std::string& jsonText);

void RegisterFormatPickerClass(HINSTANCE inst);

// 模态弹窗。返回用户选中的 -f 表达式；取消返回空串。
std::wstring ShowFormatPicker(HWND owner, const ProbeResult& pr);
