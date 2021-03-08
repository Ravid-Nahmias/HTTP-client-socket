all: client.c
	gcc client.c -o client -Wall -g -Wvla

all-GDB: client.c
	gcc client.c -o client -Wall -g -Wvla