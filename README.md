# C-Snake

The classic game: **Snake**

Written with ncurses, it uses the terminal to display its gameplay.

## How to play

Control the snake with the arrow keys. Holding down a key in a certain direction will make the snake faster! You can pause the game by pressing the enter-key. You can quit the game by pressing **SHIFT + Q**.

Rules:
* If you bite yourself you will die!
* You **can** exit the screen! (You'll come out on the other side.)
* The faster you eat the fruit, the more points you'll get!

## How to compile

```
gcc snake.c -lncurses -o snake
```
