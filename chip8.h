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

#define MEMORY_SIZE         4096        //chip 8 vm only has 4k
#define GFX_BUFFER_SIZE     64 * 32     //Buffer Array

typedef unsigned short int ushort;

class CHIP8_EMULATOR
{
    public:
    chip8();
    ~chip8();

    initializeEmulator();                       //initializes emulator to fresh state after "xxReset"
    ushort fetch_instruction(unsigned int pc); //fetches the instruction at the pc


    private:
    unsigned short int programCounter;
    unsigned short int indexRegister;
    unsigned char v[15];                        //General purpose registers
    unsigned char memory[MEMORY_SIZE];          //this holds the entirety of the emulator's memory
    unsigned char graphics[GFX_BUFFER_SIZE]
    unsigned char delayTimer;                   //counts down @60hz
    unsigned char soundTimer;                   //counts down @60hz
}



/*
System Memory Map:
0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM 
*/