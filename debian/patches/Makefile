Index: mbrtu/Makefile
===================================================================
--- mbrtu.orig/Makefile
+++ mbrtu/Makefile
@@ -1,11 +1,12 @@
-LFLAGS=-I/usr/local/include/modbus/
+LFLAGS=-I/usr/include/modbus/
 CFLAGS=-std=c99 -O3 -Wall -D_BSD_SOURCE
+LDFLAGS=-lmodbus
 SRCS=mbrtu.c
 
 all: mbrtu
 
 mbrtu: $(SRCS) mbrtu.h
-	$(CC) $(CFLAGS) $(LFLAGS) $(SRCS) -o $@ /usr/local/lib/libmodbus.a
+	$(CC) $(CFLAGS) $(LFLAGS) $(LDFLAGS) $(SRCS) -o $@
 
 clean:
 	rm -f *.o mbrtu
