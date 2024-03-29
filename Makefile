CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Werror -lxcb

default: CFLAGS += -O3
default:
	$(CC) $(CFLAGS) main.c -o ximgcopy

debug: CFLAGS += -g
debug: 
	$(CC) $(CFLAGS) main.c -o ximgcopy

homeInstall: default
homeInstall:
	cp ximgcopy ~/bin/

install: homeInstall
