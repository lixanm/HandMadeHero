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
rem  VsDevCmd sets PATH / INCLUDE / LIB so that cl.exe and link.exe exist.
rem  -arch=x64 = 64-bit toolchain.  >NUL hides its startup banner.
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >NUL
rem  errorlevel 1 = VsDevCmd could not be called (VS moved or not installed)
if errorlevel 1 goto vsfail

rem  ROOT = the folder this script lives in, WITH a trailing backslash (%~dp0).
rem  Using %~dp0 means the script works no matter where it is started from.
set ROOT=%~dp0
rem  Create the output folder on the first run (if it already exists, do nothing).
if not exist "%ROOT%build" mkdir "%ROOT%build"
rem  Build INSIDE build\, so .obj / .pdb / .exe never litter code\.
rem  pushd remembers the folder we are in now; popd (further down) returns to it.
pushd "%ROOT%build"

rem --- 2) compiler flags (official Handmade Hero set) ---
rem  ClCompile-only switches, one short note per flag:
rem    -Od      no optimization: easy to step through in the debugger
rem    -nologo  do not print the compiler banner
rem    -FC      full path in diagnostics (Emacs M-n can jump to the file)
rem    -W4      warning level 4: the chatty level Casey uses
rem    -WX      warnings ARE errors (a warning fails the build on purpose)
rem    -wdNNNN  silence a few noisy warnings we do not care about yet:
rem             4201 (nameless struct/union)   4100 (unused parameter)
rem             4505 (unused local function)   4189 (set but never used)
rem             4457 / 4456 (shadowing)        4819 (source codepage)
rem    -MTd     link the static debug CRT, so the .exe needs no VC runtime DLL
rem    -Oi      use compiler intrinsics (e.g. for memcpy / memset)
rem    -GR-     no RTTI;  -EHa- no C++ exceptions: we use neither one
rem    -Zi      emit debug info for the VS debugger
rem    -DHANDMADE_SLOW / _INTERNAL / _WIN32 = the three switches from the video
set CommonCompilerFlags=-Od -nologo -FC -WX -W4 -wd4201 -wd4100 -wd4505 -wd4189 -wd4457 -wd4456 -wd4819 -MTd -Oi -GR- -EHa- -Zi -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1

rem --- 3) linker flags ---
rem   user32.lib = where MessageBox lives (missing it gives
rem                "unresolved external symbol __imp__MessageBoxA/W")
rem   gdi32.lib / winmm.lib = needed later for graphics and sound
rem   -incremental:no = always relink fully;  -opt:ref = drop unreferenced code
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

echo ==== [build] compiling win32_handmade.cpp ====
echo.
rem  One cl command compiles AND links: everything after /link goes to link.exe.
cl %CommonCompilerFlags% "%ROOT%code\win32_handmade.cpp" /link %CommonLinkerFlags%
rem  Save cl's exit code immediately: 0 = success, anything else = failure.
rem  (It must be copied right away - the very next command overwrites errorlevel.)
set BUILD_RESULT=%errorlevel%

rem  Leave build\ and go back to the folder we started in.
popd

rem  Non-zero exit code = compile or link failed -> report it and stop here.
if not "%BUILD_RESULT%"=="0" (
  echo.
  echo ==== [build] FAILED, exit code %BUILD_RESULT% ====
  rem  Hand cl's exit code back to the caller (Emacs shows it as build status).
  exit /b %BUILD_RESULT%
)

echo.
echo ==== [build] OK: %ROOT%build\win32_handmade.exe ====
rem  Success: exit code 0 tells the caller (Emacs M-m) that the build is fine.
exit /b 0

:vsfail
rem  Jumped here only from the errorlevel check near the top.
echo [build] VsDevCmd.bat not found - check the path inside this script
exit /b 1
