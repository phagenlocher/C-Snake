snake: snake.c
	gcc snake.c -lncurses -Ofast -funroll-loops -o snake

install: snake
	mv snake /usr/bin/snake

uninstall:
	rm /usr/bin/snake