@echo off
rem ===========================================================================
rem  emacs.bat - start Emacs from a context that has the VS2022 environment
rem
rem  Why: then M-x compile inside Emacs, and clangd (eglot), can find cl.exe
rem       without loading the VS environment for every command.
rem
rem  IMPORTANT DIFFERENCE from the official Casey emacs.bat:
rem    his is   runemacs.exe -q -l w:\handmade\misc\.emacs
rem    the -q means "do not load the init file" and -l loads only his .emacs.
rem    If you copied that, it would SKIP your %APPDATA%\.emacs.d\init.el and
rem    you would lose all your keybindings (consult / magit / eglot / VS keys).
rem    So this version does NOT pass -q or -l: your own init.el loads normally.
rem
rem  KEEP THIS FILE ASCII-ONLY (cmd.exe reads .bat with the OEM codepage).
rem ===========================================================================
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >NUL
start "" "C:\THOTH\RuanJian\emacs-30.2\bin\runemacs.exe" %*
