# the compiler: gcc for C program, define as g++ for C++

CC = gcc



# compiler flags:

#  -g    adds debugging information to the executable file

#  -Wall turns on most, but not all, compiler warnings

CFLAGS  = -g -Wall



# the build target executable:

TARGET = xmod



all: $(TARGET)



$(TARGET): $(TARGET).c logs.c signals.c utils.c

	$(CC) $(CFLAGS) -o $(TARGET) logs.c signals.c utils.c $(TARGET).c



clean:
	$(RM) $(TARGET)