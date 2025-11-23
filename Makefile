all: smash

# Compiler settings
CC = g++
CFLAGS = -g -Wall -Werror -DNDEBUG -pthread
CCLINK = $(CC)
OBJS = smash.o commands.o signals.o
SYSTEM_CALL_OBJ = my_system_call.o
RM = rm -f
EXEC = smash

# Creating the executable
$(EXEC): $(OBJS) $(SYSTEM_CALL_OBJ)
	$(CCLINK) -o $(EXEC) $(OBJS) $(SYSTEM_CALL_OBJ)

# Creating object files
commands.o: commands.c commands.h
	$(CC) $(CFLAGS) -c commands.c

smash.o: smash.c commands.h signals.h
	$(CC) $(CFLAGS) -c smash.c

signals.o: signals.c signals.h
	$(CC) $(CFLAGS) -c signals.c

# my_system_call.o is precompiled - just check it exists
$(SYSTEM_CALL_OBJ):
	@test -f $@ || (echo "Error: $@ not found" && exit 1)

# Cleaning old files
clean:
	$(RM) $(EXEC) $(OBJS)
