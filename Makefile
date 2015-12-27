snake: snake.c
	gcc snake.c -lncurses -Ofast -funroll-loops -Wall -o snake

install: snake
	mv snake /usr/local/bin/snake

uninstall:
	rm /usr/local/bin/snake