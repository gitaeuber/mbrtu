LFLAGS=-I/usr/include/modbus/
CFLAGS=-std=c99 -O3 -Wall -D_BSD_SOURCE
LDFLAGS=-lmodbus
SRCS=mbrtu.c

all: mbrtu

mbrtu: $(SRCS) mbrtu.h
	$(CC) $(CFLAGS) $(LFLAGS) $(LDFLAGS) $(SRCS) -o $@

clean:
	rm -f *.o mbrtu

install: mbrtu
	install -m0755 -s mbrtu /usr/local/bin/
