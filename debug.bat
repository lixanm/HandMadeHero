@echo off
rem ===========================================================================
rem  debug.bat -- build, then open the result in RAD Debugger.
rem
rem  Usage:  debug.bat        (from anywhere; paths are derived from %~dp0)
rem
rem  IMPORTANT: this uses raddbg.exe, NOT raddbg.com.
rem    raddbg.com (the console wrapper) DROPS the target program path, so the
rem    debugger opens with no target and F5 does nothing but log an error.
rem    Verified by experiment on 2026-09-21. Use raddbg.exe.
rem
rem  KEEP THIS FILE ASCII-ONLY. cmd.exe reads .bat files using the OEM
rem  codepage, so UTF-8 non-ASCII comments can break parsing.
rem ===========================================================================
setlocal

set "RADDBG=C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\raddbg.exe"
rem  TARGET is the MSBuild solution output. Since 2026-09-30 the VS project is the
rem  single source of truth, so builds land at <repo>\x64\<Config>\HandMade.exe
rem  (NOT HandMade\build\win32_handmade.exe any more).
set "TARGET=%~dp0x64\Debug\HandMade.exe"

if not exist "%RADDBG%" (
  echo [debug] RAD Debugger not found: "%RADDBG%"
  echo [debug] Fix the RADDBG path inside this script.
  exit /b 1
)

echo ==== [debug] building ====
call "%~dp0HandMade\build.bat"
if errorlevel 1 (
  echo.
  echo ==== [debug] BUILD FAILED - not starting the debugger. ====
  exit /b 1
)

if not exist "%TARGET%" (
  echo [debug] target not found: "%TARGET%"
  exit /b 1
)

echo ==== [debug] opening RAD Debugger with:
echo              %TARGET%
echo ==== [debug] press F5 in the debugger to run. ====
start "" "%RADDBG%" "%TARGET%"
exit /b 0
