# mbrtu
=======


Description
-----------

mbrtu is a small command line tool to communicate with modbus slaves on a serial (RS485) bus (RTU).
It is based on libmodbus.
mbrtu is a short form of "Modbus RTU".


Requirements
------------

libmodbus libmodbus >= 3.1.3


Installation
------------

* compile static libmodbus library
* copy the resulting libmodbus.a to /usr/local/lib or into this directory and adapt Makefile
* $ make
* $ make install


Settings
--------

The program mbrtu only accepts command line options. It doesn't read any configuration file.
It can only be used as modbus master.
To list all available options:

    $ ./mbrtu -h
