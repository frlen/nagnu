CC=clang

CFLAGS=-Wall -g /usr/lib/libcurl.dylib

all: nagnu

nagnu: src/nagnu.c
	$(CC) $(CFLAGS) src/nagnu.c -o nagnu.out

clean:
	rm -rf nagnu.out
	rm -rf *.dSYM
