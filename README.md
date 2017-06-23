# SDL-tetris
I'm making a little game in C++, inspired by tetris and emergent gameplay systems.

## How to build
Make sure you have Visual Studio's C++ developer tools and SDL 2.0.4 installed
somewhere.

You'll need to run **vcvarsall.bat** in a command prompt before you build. I like a command prompt shortcut that automatically does this. Set the target of the shortcut to `%windir%\\system32\\cmd.exe /k <PATH_TO_YOUR_DIRECTORY>\\scripts\\shell.bat` and it will do this for you, assuming your Visual Studio is installed in the same place as mine.

Edit **build.bat** to point SDLPath to the appropriate directory on your machine, then run the script. Its output goes to the build folder. The `debug` command should open the file Visual Studio 2017 for you to debug with, and the `run` command will run the app. Any commands should be run from the `/code` directory. 

No promises it'll actually work on your machine, since this is just me messing around.
