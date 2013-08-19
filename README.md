netdev
======

Description
-----------

This project has a goal of allowing the sharing of physical devices like character and block devices between linux machines over TCP/IP.

Disclamer
---------

The code is not yet stable or really usable. Test at your own peril. This WILL cause kernel panics.

Usage
-----

Both kernel module and the server program can be compiled with make.

Server can be started with or without arguments.

    Usage: ./server [-f CONFIGFILE] [-p SERVERPORT] [-h]

Format of configuration file:

    RemoteDevicePath;DummyDeviceName;ServerIP;ServerPort

Example:

    /dev/urandom;netdev;192.168.1.13;9999

Some static limitations apply. For example limit of devices is 32 and is defined in include/protocol.h with NETDEV_MAX_DEVICES and NETDEV_HTABLE_DEV_SIZE.

Notes
-----

If you want to play with this software please do, and please report any crashes and bugs you might encounter to my github at https://github.com/PonderingGrower/netdev .
