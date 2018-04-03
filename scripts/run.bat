@echo off

IF NOT EXIST ..\data mkdir ..\data
pushd ..\data
..\build\ai_win.exe
popd
