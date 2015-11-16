LFLAGS=-I/usr/local/include/modbus/
CFLAGS=-std=c99 -O3 -Wall -D_BSD_SOURCE
SRCS=mbrtu.c

all: mbrtu

mbrtu: $(SRCS) mbrtu.h
	$(CC) $(CFLAGS) $(LFLAGS) $(SRCS) -o $@ /usr/local/lib/libmodbus.a

clean:
	rm *.o mbrtu

install: mbrtu
	install -m0755 -s mbrtu /usr/local/bin/
