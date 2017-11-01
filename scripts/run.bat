@echo off

IF NOT EXIST ..\data mkdir ..\data
pushd ..\data
..\build\alpha_cube_win.exe
popd
