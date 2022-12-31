/* Chip 8 Emulator  <chip8.h>
Copyright (C) <2018>  <Bandit>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>. 
*/
#pragma once

#include <cassert>

#define MEMORY_SIZE         4096        //chip 8 vm only has 4k
#define CPU_GPR_COUNT       16
#define GFX_COLS            64
#define GFX_ROWS            32
#define GFX_BUFFER_SIZE     (GFX_COLS/8) * (GFX_ROWS/8)     //Buffer Array
#define MAX_FILENAME_LEN    256

#define BASE_RAM_OFFSET     0x200       //lowest ram offset for program use
#define UPPER_RAM_OFFSET    0xE9F       //highest ram offset available to program
#define SIZE_OF_RAM         (UPPER_RAM_OFFSET - BASE_RAM_OFFSET)
#define BASE_FONT_OFFSET    0x050

#define BASE_STACK_OFFSET   0xEA0
#define UPPER_STACK_OFFSET  0xEFF

#define STATUS_SUCCESS 0
#define ERR_UNABLE_OPEN_FILE        -20
#define ERR_CORRUPTED_ROM           -21
#define ERR_ROM_GREATER_THAN_RAM    -22
#define ERR_INVALID_OPCODE          -30

typedef unsigned short int ushort;

class CHIP8_EMULATOR
{
    public:
    CHIP8_EMULATOR();
    ~CHIP8_EMULATOR();

    int initEmulator();                                 //initializes emulator to fresh state after "xxReset"
    int resetEmulator();                                //unload ROM and reset GPRs & instruction pointer
    int emulatorTick();                                 //Perform a fetch/execute/decode emulator cycle
    ushort fetchInstruction();                          //fetches the instruction at the pc(program counter)/instruction pointer
    int decodeAndExecuteInstruction(ushort opcode);
    //int executeInstruction()

    int loadROM(char filename[MAX_FILENAME_LEN]);       //loads ROM file into memory
    void positionPC();                                  //moves the PC to point to the start of RAM for execution
    int incrementPC(ushort offset = 1);                 //increments the program counter
    int setPC(ushort address);
    void pushAddrToStack(ushort address);               //pushes return address onto the stack
    ushort popAddrFromStack();                          //pops address off the stack

    ushort getSizeOfLoadedROM();                        //returns the number of bytes taken up by the currently loaded ROM
    ushort* logicalAddressToPhysical(ushort logicalAddr, ushort base = BASE_RAM_OFFSET);
    ushort physicalAddressToLogical(ushort *physicalAddr);

    static void initGFX();                              //initializes the graphics interface (ncurses etc)
    void refreshDisplay();                              //Writes out the graphics memory info to the display


    private:
    ushort *programCounter;
    ushort indexRegister;
    unsigned char v[CPU_GPR_COUNT];                     //General purpose registers
    unsigned char memory[MEMORY_SIZE];                  //this holds the entirety of the emulator's memory
    unsigned char graphics[GFX_BUFFER_SIZE];
    unsigned char delayTimer;                           //counts down @60hz
    unsigned char soundTimer;                           //counts down @60hz
    ushort *sb;
    ushort *sp;
    bool isRomLoaded;                                   //whether a ROM is currently loaded into emulator RAM
    static bool isRNGSeeded;                            //whether pseudo-random number generator has been seeded. (init to false per c++ standard)
    ushort sizeOfROM;

    void initFont();                                    //this function is called on object init to load font sprites into memory
};



/*
System Memory Map:
0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM 
*/