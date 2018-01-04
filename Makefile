CC = gcc
CFLAGS = -lncurses -s -O2 -fstack-protector-strong -fomit-frame-pointer -fPIE -Wl,-z,now -D_FORTIFY_SOURCE=2 -fshort-enums -Wall
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
