CFLAGS= -O2 -Ofast -Wall -std=gnu11

default: clipboard

all: clipboard app_teste app_sendall
apps:  app_teste app_sendall

app_teste: app_teste.c library.c
	gcc app_teste.c library.c -pthread -o app_teste $(CFLAGS)

app_sendall: app_sendall.c library.c
	gcc app_sendall.c library.c -o app_sendall $(CFLAGS)


clipboard: clipboard.c library.c threads.c regions.c utils.c
	gcc clipboard.c library.c threads.c regions.c utils.c -pthread -o clipboard $(CFLAGS)

clean:
	rm -f *.o clipboard app_teste app_sendall
