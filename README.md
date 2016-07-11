# SDL-tetris
I'm testing out the SDL libraries by making a simple tetris clone.

## How to build
Make sure you have Visual Studio's C++ developer tools and SDL 2.0.4 installed
somewhere.

You'll need to run **vcvarsall.bat** in a command prompt before you build. I like a command prompt shortcut that automatically does this. Set the target of the shortcut to `%windir%\\system32\\cmd.exe /k <PATH_TO_SHELL_SCRIPT\\shell.bat` and include the line `call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64` in that shell script.

Edit **build.bat** to point SDLPath to the appropriate directory on your machine, then run the script.
It's that easy! No promises it'll actually work on your machine, since this is just me messing around.
