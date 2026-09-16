# yt-dlp GUI RE

A native Windows GUI for [yt-dlp](https://github.com/yt-dlp/yt-dlp). Written in C++ against the
Win32 API — a single ~540 KB executable with **no .NET, no runtime redistributables, and no
installer**.

Available in **English, 简体中文, 繁體中文, 日本語, and 한국어**. The interface starts in English;
switch it any time from the dropdown in the top-right corner, and your choice is remembered.

**yt-dlp is bundled**, so the download button works straight out of the box.

---

## 1. Getting started

1. Extract anywhere. Paths with spaces or non-Latin characters are fine.
2. Run `yt-dlp_GUI_RE.exe`.
3. Paste a link and press **Download**.

The three status indicators in the top-right corner show what the program found. `yt-dlp` ships in
this package and will already be green. For the other two, see section 3 — **ffmpeg is required**,
and **Deno is strongly recommended**.

Settings are saved to `settings.ini` next to the executable and restored on the next launch.

### What's in the package

```
yt-dlp_GUI_RE.exe    the program
yt-dlp.exe           bundled, official x64 single-file build
README.md            this file
使用说明.md           Chinese documentation
source/              full C++ source and build script
```

`yt-dlp` is released into the public domain under the Unlicense, so it is redistributed here
unmodified. You can always replace it with a newer build, or use **Update** on the Advanced tab.

---

## 2. The default is "highest bitrate", on purpose

The default quality ordering is **"Best quality · resolution first, then highest bitrate"**, which
maps to `-S res,fps,br`.

This deliberately differs from yt-dlp's own default. yt-dlp ranks *codec efficiency* above bitrate
and therefore prefers AV1: for the same 480p video it will pick a 444 kbps AV1 stream over a
796 kbps H.264 one, on the grounds that AV1 looks comparable at half the size.

That is a reasonable default for saving disk space. It is not what you want if you asked for "the
best one". This program gives you the biggest numbers by default.

| Preset | Passed to yt-dlp | Use it when |
|---|---|---|
| Best quality · resolution first | `-S res,fps,br` | **Default.** Highest resolution, then highest bitrate within it |
| Highest bitrate only | `-S br,res,fps` | Bitrate above all else; may select a lower resolution |
| Highest frame rate first | `-S fps,res,br` | 60 fps content |
| Prefer HDR / 10-bit | `-S hdr:12,res,fps,br` | HDR sources |
| yt-dlp default | *(no `-S`)* | You'd rather save disk space |
| Smallest file | `-S +size,+br,res` | Limited bandwidth or storage |

---

## 3. Three external dependencies

### yt-dlp (required — already included)

This package ships `yt-dlp.exe` next to the program, so there is nothing to do. **Update** on the
Advanced tab upgrades it in place; **Get yt-dlp (x64)** re-downloads the latest official build.

If you ever replace it yourself, note that the program **actually runs `--version` on each
candidate** rather than trusting the filename. That sidesteps a common trap: the `yt-dlp.exe`
inside the official `yt-dlp_win.zip` is the ~7 MB *onedir* build, which only works alongside its
`_internal\` folder. Copying just the `.exe` gives you:

```
Failed to load Python DLL '...\_internal\python310.dll'
```

The file you want is the ~17 MB **single-file** `yt-dlp.exe` from the releases page — which is the
one bundled here.

### Deno (strongly recommended)

YouTube now requires actually executing the obfuscated JavaScript in its player to solve the `sig`
and `n` parameters, and yt-dlp needs an external JS engine for that. Without one you get:

```
WARNING: Signature solving failed
WARNING: n challenge solving failed
```

The practical consequences are no high-quality streams and downloads throttled to a few tens of
KB/s.

Install it from PowerShell — no administrator rights needed:

```powershell
irm https://deno.land/install.ps1 | iex
```

The default location (`%USERPROFILE%\.deno\bin`) is fine. **This program finds it automatically;
you do not need to touch your PATH.**

### ffmpeg (required)

YouTube serves video and audio as separate streams, so ffmpeg is needed to merge them. The program
looks in `C:\Program Files\ffmpeg\bin`, `C:\ffmpeg\bin`, and its own folder before falling back to
the system PATH.

---

## 4. Why your cookies keep expiring

Typical symptoms:

```
WARNING: The provided YouTube account cookies are no longer valid.
ERROR: Sign in to confirm you're not a bot.
```

**The cause:** YouTube rotates login cookies. What you exported is a snapshot of one moment. As
long as your browser keeps using that session, the server hands it fresh cookies and invalidates the
old ones — and when yt-dlp uses the old ones, *it* triggers a rotation that breaks your browser
session too. The two clients keep knocking each other out.

**So exporting more carefully will not help.** As long as the exported cookies share a session with
a live browser, they will expire.

There is exactly one fix: **make yt-dlp the only user of that session.**

### Option A — freeze a session in a private window

1. Open a private/incognito window and sign in to YouTube (a throwaway account is wise).
2. Open a new tab, then **close the tab you signed in with**.
3. In the new tab, navigate to `https://www.youtube.com/robots.txt` — a plain-text page that does
   not run YouTube's front-end code and therefore will not trigger a rotation.
4. Export `cookies.txt` with a cookie-export extension.
5. **Close the entire private window immediately.**

Once that window is gone, nothing on the browser side is using the session, so nothing rotates it
out from under yt-dlp. In the program, set Cookies to **Cookie file** and point it at the export.

### Option B — a dedicated browser profile (recommended, no manual exporting)

Install Firefox, create a separate profile with `firefox -P` (call it `ytdlp`), sign in there, and
then **never open that profile again**. In the program, set Cookies to **Read from browser**, pick
`firefox`, and type `ytdlp` as the profile name.

Firefox rather than Chrome or Edge because Chromium-based browsers on Windows now encrypt their
cookie database with App-Bound Encryption, which yt-dlp frequently cannot read — and when it can,
the browser has to be fully closed first. Firefox has neither restriction.

### Also: stop tripping the bot detector

**Polite mode** on the Network tab is on by default (`--sleep-requests 1.5` plus a random 3–12 s
pause between videos). When you queue up dozens of videos, request density alone is one of the main
triggers for "Sign in to confirm you're not a bot". Fixing your cookies will not save you if the
requests still come in a flood.

---

## 5. Features

**Links**
- One link per line; single videos, playlists, and channels all work
- Lines starting with `#` are treated as comments and skipped
- Drop a `.txt` file on the window to bulk-import links; drop a `cookies.txt` and the cookie path is
  filled in for you

**Format listing**
- Runs `yt-dlp -J` and opens a sortable table: format ID, resolution, FPS, video/audio codec,
  bitrate, size, container, notes
- Double-click any row to select that format. Video-only streams automatically get `+bestaudio`
  appended so you never end up with a silent file.

**Output**
- Five filename templates, or edit the template string directly
- Optional per-playlist subfolders
- "Remember downloads" uses `--download-archive` so re-running a playlist skips what you already
  have

**Quality and format**
- Six quality orderings plus a resolution cap (unlimited / 4K / 2K / 1080p / 720p / 480p)
- Container: auto, MP4 (prefers native H.264 + AAC streams so no re-encoding is needed), MKV, or
  WebM, with optional forced re-encoding
- Audio-only extraction to MP3, M4A, Opus, FLAC, or WAV with 11 quality steps
- A raw `-f` expression field that overrides all of the above

**Cookies** — none, a `cookies.txt` file, or read directly from one of eight browsers (with an
optional profile name)

**Subtitles and metadata** — subtitle languages, auto-generated captions, SRT conversion, embedding
into the video; cover art, metadata, and chapter embedding; SponsorBlock removal of sponsor and
self-promotion segments

**Network** — HTTP/SOCKS5 proxy, rate limit, concurrent fragment count, retry count, polite mode

**Playlists** — item ranges (`1-10` or `3,7,12-20`), per-playlist subfolders, skip already
downloaded, continue past errors, never overwrite

**Advanced** — arbitrary extra yt-dlp arguments, one-click download/update of yt-dlp, and
**Preview command**, which prints the exact command line to the log so you can check it or copy it
into a terminal

**While running**
- Live progress bar with speed, ETA, current filename, and playlist position
- Colour-coded log that can be copied, saved, or cleared; auto-scroll can be turned off
- **Stop** terminates the whole process tree via a Job Object, taking ffmpeg with it — no orphans

---

## 6. Building from source

Requires Visual Studio 2019 or newer with the **Desktop development with C++** workload.

```
cd source
build.bat            produces bin\yt-dlp_GUI_RE.exe
build.bat clean      removes intermediates
```

`build.bat` locates Visual Studio via `vswhere` and invokes `vcvars64.bat` itself, so you do not
need to run it from a Developer Command Prompt.

The icon is generated by `tools\make_icon.py`, which writes the PNG and ICO containers by hand and
does not need Pillow. A prebuilt `res\app.ico` is included, so Python is optional.

### Source layout

| File | Contents |
|---|---|
| `src/main.cpp` | Window, layout, event handling |
| `src/i18n.cpp`, `src/strings.inc` | Translation table and language selection |
| `src/theme.cpp` | Dark palette, DPI scaling, owner-drawn button / progress bar / tab strip (GDI+ for antialiased rounded shapes, GDI for text so ClearType survives) |
| `src/proc.cpp` | Process spawning, pipe reading, process-tree termination |
| `src/config.cpp` | Every setting, yt-dlp command assembly, `settings.ini` I/O |
| `src/fmtpicker.cpp` | `-J` metadata parsing and the format picker dialog |
| `src/json.h` | Minimal JSON parser, written only to chew through yt-dlp's output |
| `src/util.cpp` | Strings, paths, command-line quoting |

### Adding or changing a translation

All user-facing text lives in `src/strings.inc`, one entry per line:

```c
X(S_BTN_PASTE,
  L"粘贴", L"Paste", L"貼上", L"貼り付け", L"붙여넣기")
```

The argument order is fixed: `ID, 简体中文, English, 繁體中文, 日本語, 한국어`. Both the `StrId`
enum and the lookup table are generated from this one list by X-macro expansion, so it is
structurally impossible to add a string and forget a language, or to misalign the table.

---

## 7. Implementation notes

- **Encoding.** Child processes are given `PYTHONUTF8=1` and `PYTHONIOENCODING=utf-8`, so what
  comes back over the pipe is always UTF-8. Invalid byte sequences fall back to the system ANSI
  code page rather than dropping the line.
- **Fonts.** Each language gets its own UI font — Segoe UI, Microsoft YaHei UI, Microsoft JhengHei
  UI, Yu Gothic UI, Malgun Gothic. Rendering Japanese with a Simplified Chinese font produces wrong
  glyph shapes, and Korean simply comes out as missing characters.
- **Layout.** Control widths are measured from the actual text rather than hard-coded, because
  "Re-encode" is more than twice as wide as "重编码". Fixed pixel widths tuned for one language
  truncate badly in the others.
- **`build.bat` must be UTF-8 with CRLF line endings.** `cmd` parses batch files line by line using
  the current code page, so the script switches to `chcp 65001` up front and restores the previous
  code page on exit. LF-only line endings make `cmd` split commands apart and produce baffling
  errors.
- **Playlist subfolders** use the output template `%(playlist_title|.)s/`. The fallback must be `.`
  and not empty: an empty fallback yields `\filename` for non-playlist downloads, and a leading
  backslash on Windows resolves to the root of the current drive.
- The log is trimmed by a third once it exceeds 2 MB, so long sessions do not grow without bound.

---

## 8. Changelog

### 1.3

- Renamed to **yt-dlp GUI RE** throughout: executable, window title, header, and version resource.
  The product name is deliberately not translated — it reads the same in all five languages.

### 1.2

- `yt-dlp.exe` (official x64 single-file build) is now bundled — the package works with no setup
- The interface now starts in English regardless of the Windows display language. To make it follow
  the system language instead, change one line in `src/main.cpp`; the comment there says which.

### 1.1

- Interface available in English, 简体中文, 繁體中文, 日本語, and 한국어, switchable at runtime
- Control widths are now measured from the rendered text, so no label is truncated in any language
- Subtitle language defaults follow the interface language (`en`, `ja,en`, `ko,en`, and so on)
- The download archive is now the language-neutral `downloaded.txt`. An existing `已下载记录.txt`
  from 1.0 is still honoured when present, so upgrading will not cause re-downloads.
