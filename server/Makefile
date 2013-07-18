CC = gcc
CFLAGS = -g -Wall
INCLUDE = -I../include
DEPS = ../include/protocol.h
OBJ = main.o
TARGET = server

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $^

clean:
	rm ./server ./*.o
