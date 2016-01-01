# C-Snake

The classic game: **Snake**

Written with ncurses, it uses the terminal to display its gameplay.

## How to play

Control the snake with the arrow keys. Holding down a key in a certain direction will make the snake faster! You can pause the game by pressing **ENTER**. 

**SHIFT + Q** will exit the current game and will leave you at the title screen. **SHIFT + R** will restart the round. If the size of your terminal has changed you can use this to refit the game to your terminal.

The highscore is saved in a file (called *.csnake*) in your home directory.

Rules:
* If you bite yourself you will die!
* The faster you eat the fruit, the more points you'll get!

Arguments:
* **-o** will make the outer bounds open so you can exit the screen and come out on the other side!
* **-w <0-4>** activates the usage of walls within the level. *1-4* are predefined wall patterns and *0* are randomly created walls.
* **-c <1-5>** changes the color of the snake
* **-s** skips the titlescreen
* **-r** removes the savefile
* **-i** will ignore the savefile
* **-h** displays help information
* **-v** displays information about the version and license

## Requirements / Dependencies
* gcc
* make
* libncurses5

On Ubuntu or Debian you can use this install the dependencies:
```
apt-get install gcc make libncurses5-dev
```

## How to compile

```
make
```

If you want to install C-Snake to your local binary directory you can use:
```
make install
```

To uninstall it again:
```
make uninstall
```
