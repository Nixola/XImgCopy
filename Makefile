CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Werror -lX11 -g

default:
	$(CC) $(CFLAGS) main.c -o xclipboard
