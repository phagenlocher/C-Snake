CC = gcc
CFLAGS = -lncurses -Os -fomit-frame-pointer -fPIE -fshort-enums -Wall
TARGET = csnake
bindir = /usr/local/bin

all: snake.c
	$(CC) $(CFLAGS) snake.c -o $(TARGET)

install: all
	mv $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(TARGET)

clean:
	rm -f $(TARGET)
