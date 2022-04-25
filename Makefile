CC = cc
CFLAGS = -O3 -fomit-frame-pointer -fPIE -fshort-enums -Wall -pedantic
TARGET = csnake
bindir = /usr/local/bin

all: snake.c
	$(CC) $(CFLAGS) snake.c -o $(TARGET) -lncurses

install: all
	mv $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(TARGET)

clean:
	rm -f $(TARGET)
