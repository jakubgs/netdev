CC = gcc
CFLAGS = -g -Wall
INCLUDE = -I../include -I./lib
DEPS = ../include/protocol.h
OBJ = netlink.o conn.o signal.o proxy.o main.o
TARGET = netdevd

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $^

clean:
	rm ./server ./*.o
