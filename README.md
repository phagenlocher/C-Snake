# C-Snake

The classic game: **Snake**

This is a basic implementation focusing on a simple design, small code base, minimal external dependencies and good code documentation. Therefore, this project is a good starting-off point if you want to create your own snake implementation.

## How to play

Control the snake with arrow keys. Holding down a key in a certain direction will make the snake faster! You can pause the game by pressing **ENTER**. 

**SHIFT + Q** will exit the current game and will leave you at the title screen. **SHIFT + R** will restart the round. If the size of your terminal has changed you can use this to refit the game to your terminal.

The highscore is saved in a file (called *.csnake*) in your home directory.

Rules:
* If you bite yourself you will die!
* The faster you eat the fruit, the more points you'll get!

Arguments:
* `--open-bounds`, `-o` will make the outer bounds open so you can exit the screen and come out on the other side
* `--walls <0-5>`, `-w <0-5>` activates the usage of walls within the level. *1-5* are predefined wall patterns and *0* are randomly created walls.
* `--color <1-5>`, `-c <1-5>` changes the color of the snake
* `--skip-title`, `-s` skips the title screen
* `--remove-savefile`, `-r` removes the savefile
* `--ignore-savefile`, `-i` will ignore the savefile
* `--filepath path`, `-f path` will use *path* as the savefile
* `--vim` changes controls with arrow keys to H, J, K and L
* `--maximum-speed` changes the maximum speed
* `--help`, `-h` displays help information
* `--version`, `-v` displays information about the version and license

## Requirements / Dependencies
* gcc
* make
* libncurses

On Ubuntu or Debian you can use this to install the dependencies:
```
apt-get install gcc make libncurses-dev
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


