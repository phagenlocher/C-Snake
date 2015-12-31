CC = gcc
CFLAGS = -lncurses -Os -funroll-loops -fPIE -fshort-enums -Wall

csnake: snake.c
	$(CC) $(CFLAGS) snake.c -o csnake

install: csnake
	mv csnake /usr/local/bin/csnake

uninstall:
	rm /usr/local/bin/csnake
