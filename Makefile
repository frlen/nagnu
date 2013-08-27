CC=clang

CFLAGS=-Wall /usr/lib/libcurl.dylib /usr/lib/libncurses.dylib

all: nagnu

nagnu: src/nagnu.c
	$(CC) $(CFLAGS) src/nagnu.c -o nagnu.out

clean:
	rm -rf nagnu.out
	rm -rf *.dSYM
