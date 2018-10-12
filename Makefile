CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Werror -lX11

default: CFLAGS += -O3
default:
	$(CC) $(CFLAGS) main.c -o ximgcopy

debug: CFLAGS += -g
debug: 
	$(CC) $(CFLAGS) main.c -o ximgcopy
