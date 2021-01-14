# SessionPlugin
BakkesMod Rocket League plugin to keep track of and display your current session stats per playlist. It shows your current MMR (Match Making Rating) gain/loss, wins, losses and your current win/loss streak.

## Installation
Install this plugin from [BakkesPlugins](https://bakkesplugins.com/plugins/view/151) 
Install plugin with plugin manager: ID = 151

Note: When installing, be sure to restart game after to fully install. 

Build it yourself either:

* Set an environment variable
    1. Add a BAKKES_MOD environment variable that has the path to the bakkesmod folder (e.g. 'STEAMLIBRARY/steamapps/common/rocketleague/Binaries/Win64/bakkesmod')
    2. Restart computer
    3. Build the solution in Release | 64-bit
* Edit the project properties to correct the paths (replace BAKKES_MOD environment variable with the path to the bakkesmod folder in the rocket league 64 bin folder)
    1. C/C++ -> General -> Additional Include Directories
    2. Linker -> General -> Additional Library Directories
    3. Build Events -> Post-Build Event -> Command Line
        1. SessionPlugin.dll
        2. SessionPlugin.set
