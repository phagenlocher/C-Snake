# C-Snake

The classic game: **Snake**

Written with ncurses, it uses the terminal to display its gameplay.

## How to play

Control the snake with the arrow keys. Holding down a key in a certain direction will make the snake faster! You can pause the game by pressing **ENTER**. **SHIFT + Q** will exit the current game and
will leave you at the title screen. 

Rules:
* If you bite yourself you will die!
* The faster you eat the fruit, the more points you'll get!

Arguments:
* **-o** will make the outer bounds open so you can exit the screen and come out on the other side!

## How to compile

```
gcc snake.c -lncurses -o snake
```
