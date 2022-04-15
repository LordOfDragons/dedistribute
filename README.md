# Drag[en]gine Distribution

Contains Distribution Launcher files for different operating systems for use
with game distribution platforms like Steam.

# Overview
Drag[en]gine bases games are distributed using *.delga files. These files
contain only the game but no engine executable which makes them safe to use.
The user requires the Drag[en]gine game engine to be installed on his system
to run *.delga files. While he can do this by on his own it is easier if a
small launcher file takes care of this process if possible. Furthermore
game distribution platforms like Steam expect games to deal with requirements
themselves.

# Usage
To get your *.delga file ready for distribution on Steam and similar platforms
unpack the files into a new directory, for example "Content".

Choose the distribution platform to use. For Microsoft App Store use the
directory "microsoft_appstore". For SteamWorks use the directory "steamworks".

Copy your *.delga file next to the Launcher.ini file of your chosen platform.
The directory structure should look like this:

For Microsoft App Store:
- Content/VFS/ProgramFilesX64/MyGame
  - Launcher.ini
  - MyGame.delga
  - Launcher64.exe

For SteamWorks:
- Content
  - Launcher.ini
  - MyGame.delga
  - Launcher64.exe  (windows only)
  - install-dragengine-{VERSION}-windows64.exe  (windows only)
  - installscript-dragengine-{VERSION}.vdf  (windows only)
  - launcher64  (linux only)
  - install-dragengine-{VERSION}-linux64.sh  (linux only)

Edit Launcher.ini and change the "File=" value to match your *.delga file.
Optionally you can put your *.delga file (and patches) into a subdirectory.
In this case enter the *.delga path relative to the Launcher.ini file
using "/" as path separator.

For Microsoft App Store Use content of "Content" directory as input for building
your MSIX upload file together with necessary files like manifest and icons.

For SteamWorks upload the content of the "Content" directory. If you want to
support only one platform you can delete all files marked for this platform.

In the distribution platform enter the executable name of the matching
platform without any arguments:
- Windows use "Launcher64.exe"
- Linux use "launcher64"

For Windows add the "installscript-dragengine-{VERSION}.vdf" as install
script. If the user does not have the game engine it will be silently installed.

For Linux make sure the files "launcher64" and "install_dragengine.sh" have
the executable bit set after uploading or your game will not run.

# How it works
If the user runs the game the launchers check if the Drag[en]gine Game Engine
is installed on the user system.

If the game engine is not installed on the system it is installed using the
platform specific way:
- Windows:
  - Microsoft App Store: Redirects the user to the Drag[en]gine App Store page.
    He can then install the package for free.
  - SteamWorks: Silently installs the game engine using the included
    install-dragengine-{VERSION}-windows64.exe
- Linux: Opens a terminal window with the installer script. The user has
  to enter the super user password to allow the installation to carry on.

If the game engine is installed the Drag[en]gine Launcher is invoked with
the included *.delga file. The distribution launcher file then closes and
the Drag[en]gine Launcher takes care of running the game.
