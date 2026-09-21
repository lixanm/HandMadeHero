@echo off
rem ===========================================================================
rem  shell.bat - open a command prompt with the VS2022 developer environment
rem  Equivalent of the official Handmade Hero misc\shell.bat
rem  (that one points at VS2019 and his w:\ drive).
rem
rem  Use it to type raw cl commands by hand, or to start Emacs from a shell
rem  that already has the VS environment loaded.
rem
rem  Note: with VsDevCmd already inside build.bat, you do NOT need this file
rem        to use M-m. It is a convenience.
rem
rem  KEEP THIS FILE ASCII-ONLY (cmd.exe reads .bat with the OEM codepage).
rem ===========================================================================
rem  Load the VS2022 x64 toolchain into THIS command prompt (sets PATH/INCLUDE/LIB).
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
rem  Put this project folder in front of PATH, so "build.bat" can be typed
rem  from any directory without a path (%~dp0 = folder of this script).
set PATH=%~dp0;%PATH%
echo.
echo [shell] VS2022 x64 environment loaded.
echo [shell] Project dir %~dp0 added to PATH, so just typing build.bat works.
echo.
rem  /k = keep this window open with the environment still active.
rem  (Without /k the window would close at once and the whole file be pointless.)
cmd /k
