# C-Snake

![Status](http://home.in.tum.de/~hagenloc/CSNAKE/Status-Beta-green.svg)

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
* **--open-bounds, -o** will make the outer bounds open so you can exit the screen and come out on the other side
* **--walls <0-5>, -w <0-5>** activates the usage of walls within the level. *1-5* are predefined wall patterns and *0* are randomly created walls.
* **--color <1-5>, -c <1-5>** changes the color of the snake
* **--skip-title -s** skips the title screen
* **--remove-savefile, -r** removes the savefile
* **--ignore-savefile, -i** will ignore the savefile
* **--filepath path, -f path** will use *path* as the savefile
* **--vim** changes controls with arrow keys to H, J, K and L
* **--help, -h** displays help information
* **--version, -v** displays information about the version and license

## Requirements / Dependencies
* gcc
* make
* libncurses5

On Ubuntu or Debian you can use this to install the dependencies:
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

By default the binary will be called *csnake*. If you want to change that you can use:
```
make TARGET=<New Name> install
```
Be sure to use the same target name for the uninstall.

## Screenshots

### XTerm
![Screenshot 1](http://home.in.tum.de/~hagenloc/CSNAKE/screen1.png)
### Terminator
![Screenshot 2](http://home.in.tum.de/~hagenloc/CSNAKE/screen2.png)
### Debian TTY in Virtualbox
![Screenshot 3](http://home.in.tum.de/~hagenloc/CSNAKE/screen3.png)
### cool-retro-term
![Screenshot 4](http://home.in.tum.de/~hagenloc/CSNAKE/screen4.png)
