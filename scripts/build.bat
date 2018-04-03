@echo off
set SDLPath=C:\projects\build\SDL2-2.0.4
set GifferPath=C:\projects\build\giffer

REM set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Ox -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -DCLAIRE_AI_INTERNAL=1 -DCLAIRE_AI_SLOW=1 -DCLAIRE_AI_WIN32=1 -FC -Z7
set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -DCLAIRE_AI_INTERNAL=1 -DCLAIRE_AI_SLOW=1 -DCLAIRE_AI_WIN32=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib xaudio2.lib
REM TODO: - can we just build both with one exe?

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

copy %SDLPath%\lib\x64\SDL2.dll SDL2.dll

REM 32-bit build
REM cl %CommonCompilerFlags% ..\code\win32.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\code\ai.cpp -Fmai.map -LD /I %SDLPath%\include /link -incremental:no -opt:ref -PDB:ai_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples /LIBPATH:%SDLPath%\lib\x64 SDL2.lib SDL2main.lib
REM -EXPORT:GameGetSoundSamples
cl %CommonCompilerFlags% ..\code\ai_win.cpp -Fmai_win32.map /I %GifferPath%\ %GifferPath%\giffer.lib /I %SDLPath%\include  /link %CommonLinkerFlags%  /LIBPATH:%SDLPath%\lib\x64 SDL2.lib SDL2main.lib
popd
