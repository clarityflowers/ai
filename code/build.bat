@echo off

REM set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Ox -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -DALPHA_CUBE_INTERNAL=1 -DALPHA_CUBE_SLOW=1 -DALPHA_CUBE_WIN32=1 -FC -Z7
set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -DALPHA_CUBE_INTERNAL=1 -DALPHA_CUBE_SLOW=1 -DALPHA_CUBE_WIN32=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

REM TODO: - can we just build both with one exe?

IF NOT EXIST \build mkdir \build
pushd \build
IF EXIST win32.ilk DEL /F /S /Q /A win32.ilk
IF EXIST win32.pdb DEL /F /S /Q /A win32.pdb

REM 32-bit build
REM cl %CommonCompilerFlags% ..\code\win32.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% \roguelike\code\alpha_cube.cpp -Fmalpha_cube.map -LD /I \build\SDL2-2.0.4\include /link -incremental:no -opt:ref -PDB:alpha_cube_%random%.pdb  -EXPORT:GameUpdateAndRender /LIBPATH:\build\SDL2-2.0.4\lib\x64 SDL2.lib SDL2main.lib
REM -EXPORT:GameGetSoundSamples
cl %CommonCompilerFlags% \roguelike\code\alpha_cube_win.cpp -Fmalpha_cube_win32.map /I \build\SDL2-2.0.4\include /link %CommonLinkerFlags%  /LIBPATH:\build\SDL2-2.0.4\lib\x64 SDL2.lib SDL2main.lib
popd
