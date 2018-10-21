# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC = g++
CFLAGS  = -g -Wall
LFLAGS = 


default: chip8_emulator

chip8_emulator:  chip8.o main.o
	$(CC) $(CFLAGS) $(LFLAGS) -o chip8_emulator chip8.o main.o

chip8.o:  chip8.cpp chip8.h
	$(CC) $(CFLAGS) -c chip8.cpp

main.o:  main.cpp
	$(CC) $(CFLAGS) -c main.cpp


# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean:
	$(RM) *.o *~ chip8_emulator