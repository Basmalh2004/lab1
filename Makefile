CC = gcc
CXX = g++
CFLAGS = -std=c99 -Wall -Werror -pedantic-errors -pthread -DNDEBUG
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = smash

all: my_system_call_c.o $(TARGET)

my_system_call_c.o: my_system_call.o
	objcopy --redefine-sym _Z14my_system_calliz=my_system_call my_system_call.o my_system_call_c.o

$(TARGET): $(OBJS) my_system_call_c.o
	$(CXX) $(OBJS) my_system_call_c.o -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJS)
