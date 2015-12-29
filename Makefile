csnake: snake.c
	gcc snake.c -lncurses -Ofast -funroll-loops -Wall -o csnake

install: csnake
	mv csnake /usr/local/bin/csnake

uninstall:
	rm /usr/local/bin/csnake