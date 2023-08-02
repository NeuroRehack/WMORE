CC=gcc
CFLAGS=-Wall -g -pedantic

all: zmodemFileReceive usbdevice_list

zmodemFileReceive: zmodemFileReceive.c
	$(CC) $(CFLAGS) -o zmodemFileReceive zmodemFileReceive.c

usbdevice_list: usbdevice_list.c
	$(CC) $(CFLAGS) -g -o usbdevice_list usbdevice_list.c

clean:
	rm -f zmodemFileReceive
	rm -f usbdevice_list
	
