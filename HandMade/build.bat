@echo off
rem ===========================================================================
rem  Handmade Hero build script (adapted for this machine)
rem
rem  Usage: open code\win32_handmade.cpp in Emacs, press M-m to build.
rem         After that F5 / F7 rebuilds directly.
rem         Errors: M-n / M-N  (or F11 / F10).
rem  Output: build\win32_handmade.exe
rem
rem  Differences from the official Handmade Hero misc\build.bat:
rem    - VsDevCmd is called here. The official one puts that in shell.bat and
rem      launches Emacs from that shell; doing it here means M-m works no
rem      matter how Emacs was started.
rem    - removed -Gm-            (deprecated minimal-rebuild option; tested --
rem                               VS2022 still accepts it but it is a no-op)
rem    - removed -I..\iaca-win64 (bad path in the official copy; not needed day 1)
rem    - builds only win32_handmade.cpp (no handmade.cpp / hot-reload DLL yet)
rem
rem  KEEP THIS FILE ASCII-ONLY. cmd.exe reads .bat files using the OEM
rem  codepage, so UTF-8 non-ASCII comments can break parsing.
rem ===========================================================================

rem --- 1) load the VS2022 x64 developer environment (otherwise no cl.exe) ---
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >NUL
if errorlevel 1 goto vsfail

set ROOT=%~dp0
if not exist "%ROOT%build" mkdir "%ROOT%build"
pushd "%ROOT%build"

rem --- 2) compiler flags (official Handmade Hero set) ---
rem   -WX = warnings as errors (Casey does this on purpose; drop it if too strict)
set CommonCompilerFlags=-Od -nologo -FC -WX -W4 -wd4201 -wd4100 -wd4505 -wd4189 -wd4457 -wd4456 -wd4819 -MTd -Oi -GR- -EHa- -Zi -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1

rem --- 3) linker flags ---
rem   user32.lib = where MessageBox lives (missing it gives
rem                "unresolved external symbol __imp__MessageBoxA/W")
rem   gdi32.lib / winmm.lib = needed later for graphics and sound
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

echo ==== [build] compiling win32_handmade.cpp ====
echo.
cl %CommonCompilerFlags% "%ROOT%code\win32_handmade.cpp" /link %CommonLinkerFlags%
set BUILD_RESULT=%errorlevel%

popd

if not "%BUILD_RESULT%"=="0" (
  echo.
  echo ==== [build] FAILED, exit code %BUILD_RESULT% ====
  exit /b %BUILD_RESULT%
)

echo.
echo ==== [build] OK: %ROOT%build\win32_handmade.exe ====
exit /b 0

:vsfail
echo [build] VsDevCmd.bat not found - check the path inside this script
exit /b 1
