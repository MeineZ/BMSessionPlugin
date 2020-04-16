# SessionPlugin
BakkesMod Rocket League plugin to keep track of and display your current session stats per playlist. It shows your current MMR (Match Making Rating) gain/loss, wins, losses and your current win/loss streak.

## Installation
You can send me a DM on Discord to receive the plugin: Meine#8883 or, if you want to build it yourself either:

* Set an environment variable
    1. Add a BAKKESMOD environment variable that has the path to the bakkesmod folder (e.g. 'STEAMLIBRARY/steamapps/common/rocketleague/Binaries/Win64/bakkesmod')
    2. Build the solution in Release | 64-bit
* Edit the project properties to correct the paths (replace BAKKESMOD environment variable with the path to the bakkesmod folder in the rocket league 64 bin folder)
    1. C/C++ -> General -> Additional Include Directories
    2. Linker -> General -> Additional Library Directories
    3. Build Events -> Post-Build Event -> Command Line
        1. SessionPlugin.dll
        2. SessionPlugin.lib
        3. SessionPlugin.set
