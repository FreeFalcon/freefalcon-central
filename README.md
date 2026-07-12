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

FreeFalcon currently requires Visual Studio 2026 community edition. You will
need Windows SDK and DirectX SDK installed with Visual Studio and Kronos
OpenXR Headers and Loader packages installed via nuget package manager.

To load the Installer project, you'll need the latest version of the WiX
Toolset, which can be found [here](http://wixtoolset.org/). Without this, you
will get an error when loading the solution, but you will still be able to
build the FreeFalcon source code- you just won't be able to package it into
an installer.

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

## Adding new resources to the game directory

Inside installer folder there's res subfolder that contains new textures, setup pages
and other files needed to run the game correctly. copy contents of that folder into
the game folder.

## Building the Installer

For patch installer build run build_patch.cmd from installer directory. It will 
take exe from game dir and other files from res subfolder inside installer dir.

## Contributing

You can contribute to the project in various ways.

 * If you're a programmer, go ahead, fork us and hack away! If you feel that
   you have something good going, send a pull request and we'll talk about
   getting your code into the main source tree.
 * If you're not a programmer, not to worry! Just let us know if you find any
   bugs or have any suggestions in the issue tracker.

## Credits for 3rd party models used for touch controllers visualization

"Andrews Oculus Controller Left" (https://skfb.ly/6ACKG) by djvivid is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/)

"Andrews Oculus Controller Right" (https://skfb.ly/6ACKI) by djvivid is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/)

"Valve Index Controller Left" (https://skfb.ly/6SpQ7) by F53 is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/)

"Valve Index Controller Right" (https://skfb.ly/6QXCY) by F53 is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/)

"Gloves for fps game" (https://skfb.ly/6TICP) by bobeer is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/)

## Legal

FreeFalcon doesn't have the cleanest history regarding its terms of use, but
that all ends here. The code is now licensed under the rather liberal BSD
2-clause license; for the first time in its history, FreeFalcon is truly free.
See the LICENSE.md file for the full text of the license.
