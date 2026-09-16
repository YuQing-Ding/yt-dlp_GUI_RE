@echo off
rem ============================================================
rem  YouTube 下载器 —— 构建脚本 (MSVC x64)
rem    build.bat          构建
rem    build.bat clean    清理中间产物
rem
rem  本文件以 UTF-8 + CRLF 保存。cmd 是按「当前代码页」逐行读取
rem  批处理的，所以先把代码页切到 65001，后面几行里的中文才不会
rem  被切错字节、把命令拆散。结束时再切回去。
rem ============================================================
for /f "tokens=2 delims=:" %%c in ('chcp') do set "OLDCP=%%c"
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "OUTNAME=yt-dlp_GUI_RE.exe"
set "OBJDIR=obj"
set "BINDIR=bin"
set "RC=0"

if /i "%~1"=="clean" goto :doclean
goto :setupvc

:doclean
if exist "%OBJDIR%" rd /s /q "%OBJDIR%"
if exist "%BINDIR%" rd /s /q "%BINDIR%"
echo 已清理。
goto :done

:setupvc
if defined VCINSTALLDIR goto :vcready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto :novs
echo 使用 %VSPATH%
rem vcvars64.bat 内部会去 PATH 上找一次 vswhere 并失败，这条噪音要挡掉
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
if errorlevel 1 goto :novs
goto :vcready

:novs
echo [错误] 找不到 MSVC 生成工具。
echo        需要 Visual Studio 2019 或更新版本，并勾选「使用 C++ 的桌面开发」工作负载。
set "RC=1"
goto :done

:vcready
if not exist "%OBJDIR%" md "%OBJDIR%"
if not exist "%BINDIR%" md "%BINDIR%"

if exist "res\app.ico" goto :compile
where python >nul 2>&1
if errorlevel 1 goto :noicon
python tools\make_icon.py
if exist "res\app.ico" goto :compile

:noicon
echo [错误] res\app.ico 不存在，且没有 Python 可以生成它。
set "RC=1"
goto :done

:compile
echo [1/3] 编译资源...
rc /nologo /fo "%OBJDIR%\app.res" res\app.rc
if errorlevel 1 goto :failed

echo [2/3] 编译源码...
cl /nologo /c /EHsc /W3 /std:c++17 /utf-8 /O2 /GL /MT /DNDEBUG /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS /Fo:"%OBJDIR%\\" src\main.cpp src\i18n.cpp src\util.cpp src\config.cpp src\proc.cpp src\theme.cpp src\fmtpicker.cpp
if errorlevel 1 goto :failed

echo [3/3] 链接...
link /nologo /LTCG /SUBSYSTEM:WINDOWS /MACHINE:X64 /OUT:"%BINDIR%\%OUTNAME%" "%OBJDIR%\main.obj" "%OBJDIR%\i18n.obj" "%OBJDIR%\util.obj" "%OBJDIR%\config.obj" "%OBJDIR%\proc.obj" "%OBJDIR%\theme.obj" "%OBJDIR%\fmtpicker.obj" "%OBJDIR%\app.res" user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib ole32.lib shlwapi.lib uxtheme.lib dwmapi.lib gdiplus.lib urlmon.lib advapi32.lib
if errorlevel 1 goto :failed

echo.
echo [完成] %BINDIR%\%OUTNAME%
for %%f in ("%BINDIR%\%OUTNAME%") do echo        %%~zf 字节
goto :done

:failed
echo.
echo [失败] 构建中断，看上面的报错。
set "RC=1"

:done
endlocal & set "RC=%RC%"
if defined OLDCP chcp %OLDCP% >nul
if "%RC%"=="1" exit /b 1
exit /b 0
