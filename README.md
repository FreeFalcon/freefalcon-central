# FreeFalcon

A campaign based, multiplayer, open source flight simulator.

## About FreeFalcon

FreeFalcon was once a project to mod the original Microprose Falcon 4.0
Combat Simulator. Most of the development was done from a small group of
people dedicated to the scene. Now we're going open source, and anybody can
contribute.

## Build Instructions

This is only a summary of the requirements and assumes a knowledge of your
way around installing libraries on your system. For more detail, see
[our documentation](https://github.com/FreeFalcon/docs).

FreeFalcon currently requires Visual Studio 2010, updated to SP1.

To load the Installer project, you'll need the latest version of the WiX
Toolset, which can be found [here](http://wixtoolset.org/). Without this, you
will get an error when loading the solution, but you will still be able to
build the FreeFalcon source code- you just won't be able to package it into
an installer.

You'll need the Windows SDK 7.1 (get it
[here](http://www.microsoft.com/en-us/download/details.aspx?id=8279)); to
install it successfully, you'll need to uninstall any existing Visual C++
2010 redistributables on your machine. They will be reinstalled with the SDK.
Then install
[this](http://www.microsoft.com/en-us/download/details.aspx?id=4422) to update
the compiler.

The DirectX 8.1 SDK is also required (download
[here](http://www.darwinbots.com/numsgil/dx81sdk_full.exe)). Install it
anywhere on your hard drive, and point Visual Studio to where ever you
installed it.
[This tutorial](http://takinginitiative.wordpress.com/2010/07/02/setting-up-the-directx-sdk-with-visual-studio-2010/)
covers how to do that- follow the instructions under "Setting up the Include
and Library Paths", you can ignore "Linking the DirectX Static Libraries In
Your Projects" as that is already taken care of.

To set up the source code to run in Debug mode, you'll need an install of
FreeFalcon on your computer. To tell Visual Studio where this installation is,
right click on the FFViper project and select Properties from the dropdown.
Under Configuration Options in the window that pops up, select Debugging and
edit the Working Directory to the root of your FreeFalcon install.

To compile the code, do a build on the solution or the FFViper project. To run
the code, run the Debug target of the FFViper project. You should set FFViper
as the startup project in the solution's settings so that this is done
automatically. Once you've built the code once, you usually won't have to do
full rebuilds; just run the Debug target again on FFViper and it should build
any changes. If you pull down changes from GitHub, you may have to rebuild the
projects that were changed.

Also, if you are planning on sending patches, be sure that you set Visual
Studio to use spaces instead of tabs! This setting is located at
Tools -> Options... -> Text Editor -> C/C++ -> Tabs, select "Insert spaces".

## Building the Installer

To build the installer, you **must** be on the Release target. You can then
right click the project and click Build and it will create the installer. If
you don't have an up-to-date Release build of FFViper, this may take a while-
it will build FFViper for you if it needs to. The installer will build into
the `pkg/` directory at the root of the repository.

## Contributing

You can contribute to the project in various ways.

 * If you're a programmer, go ahead, fork us and hack away! If you feel that
   you have something good going, send a pull request and we'll talk about
   getting your code into the main source tree.
 * If you're not a programmer, not to worry! Just let us know if you find any
   bugs or have any suggestions in the issue tracker.

## Legal

FreeFalcon doesn't have the cleanest history regarding its terms of use, but
that all ends here. The code is now licensed under the rather liberal BSD
2-clause license; for the first time in its history, FreeFalcon is truly free.
See the LICENSE.md file for the full text of the license.
