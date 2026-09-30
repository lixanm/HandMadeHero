@echo off
rem ===========================================================================
rem  build.bat -- ONE-LINE convenience wrapper around the real build.
rem
rem  [!] The single source of truth for compiler/linker options is now
rem      HandMade\HandMade.vcxproj  (changed 2026-09-30). This file holds NO
rem      options of its own -- it just calls MSBuild on HandMade.sln, so
rem      "build.bat" and "F7 in Visual Studio" produce identical results.
rem
rem  Usage:  build.bat            -> Debug|x64   (default)
rem          build.bat release    -> Release|x64
rem
rem  Output: x64\Debug\HandMade.exe   or   x64\Release\HandMade.exe
rem          NOTE: at the REPO ROOT, not inside HandMade\ -- that is where
rem          MSBuild puts solution builds. (debug.bat already points there.)
rem
rem  Debugging: F5 in Visual Studio, or run debug.bat (RAD Debugger).
rem
rem  KEEP THIS FILE ASCII-ONLY. cmd.exe reads .bat files using the OEM
rem  codepage, so UTF-8 non-ASCII comments can break parsing.
rem ===========================================================================
setlocal
set "CONFIG=Debug"
if /i "%~1"=="release" set "CONFIG=Release"

rem  Load the VS2022 x64 developer environment: this is what puts msbuild.exe
rem  on PATH (otherwise "msbuild is not recognized").
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >NUL
if errorlevel 1 (
  echo [build] VsDevCmd.bat not found - check the path inside this script
  exit /b 1
)

echo ==== [build] msbuild HandMade.sln  Config=%CONFIG%  Platform=x64 ====
rem  %~dp0 = the folder this script lives in (HandMade\), so ".." = repo root,
rem  which is where HandMade.sln lives.
pushd "%~dp0.."
msbuild HandMade.sln /t:Build /p:Configuration=%CONFIG% /p:Platform=x64 /v:minimal /nologo
rem  Copy the exit code immediately: the next command would overwrite errorlevel.
set BUILD_RESULT=%errorlevel%
popd

if not "%BUILD_RESULT%"=="0" (
  echo.
  echo ==== [build] FAILED, exit code %BUILD_RESULT% ====
  exit /b %BUILD_RESULT%
)

echo.
echo ==== [build] OK: x64\%CONFIG%\HandMade.exe ====
exit /b 0
