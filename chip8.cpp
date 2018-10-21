/* Chip 8 Emulator  <chip8.cpp>
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

#include <iostream>
#include <fstream>
#include <cstring>
#include "chip8.h"

using namespace std;

/*
System Memory Map:
0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM 
*/

CHIP8_EMULATOR::CHIP8_EMULATOR()
{
    initEmulator();
}

CHIP8_EMULATOR::~CHIP8_EMULATOR()
{
    //do nothing currently
}

int CHIP8_EMULATOR::initEmulator()
{
    memset(memory, 0, MEMORY_SIZE);             //zero out the chips memory regions
    memset(v, 0, CPU_GPR_COUNT);                //zero out the cpu's GPRs
    memset(graphics, 0, GFX_BUFFER_SIZE);       //zero gfx buffer.  Should this be seperate from system memory?
    delayTimer = 0;
    soundTimer = 0;

    indexRegister = 0;
    programCounter = 0;

    isRomLoaded = false;
    return STATUS_SUCCESS;
}

int CHIP8_EMULATOR::loadROM(char filename[MAX_FILENAME_LEN])
{
    ifstream inFile;
    inFile.open(filename, ios::binary | ios::ate);

    if(!inFile.is_open()) //unable to open input file
    {
        cerr << "Unable to Open Input ROM file!" << endl;
        return ERR_UNABLE_OPEN_FILE;
    }
    int filesize = inFile.tellg();      //get the size of the ROM we are trying to load
    if(filesize > SIZE_OF_RAM)          //ROM file exceeds size of RAM  Currently dont load
    {
        inFile.close();
        return ERR_ROM_GREATER_THAN_RAM;
    }

    //Now we need to load the program into RAM
    inFile.seekg(0, ios::beg);  //seek to the beginning of the ROM
    inFile.read((char *) &this->memory[BASE_RAM_OFFSET], filesize);     //block load the ROM into emulator RAM
    this -> isRomLoaded = true;
    inFile.close();     //close the ROM file
    positionPC();       //move the program counter to point to start of execution RAM
    return STATUS_SUCCESS;
}

void CHIP8_EMULATOR::positionPC()
{
    programCounter = (ushort*) &memory[BASE_RAM_OFFSET];
}

int CHIP8_EMULATOR::emulatorTick()
{
    ushort opcode = 0;
    //Fetch Instruction
    opcode = fetchInstruction(programCounter);
    cout << "Fetched Opcode: 0x%4x" << opcode << endl;

    //Decode Instruction

    //Execute Instruction

    return STATUS_SUCCESS;
}

ushort CHIP8_EMULATOR::fetchInstruction(ushort *pc)
{
    ushort opcode = 0;
    // assert((("Tried to fetch an instruction out of bounds!" , (pc >= BASE_RAM_OFFSET && pc <= UPPER_RAM_OFFSET)));
    assert( pc >= (ushort*)&memory[BASE_RAM_OFFSET] && pc <= (ushort*)&memory[UPPER_RAM_OFFSET] );    //tried to fetch an instruction out of bounds
    opcode = *pc;
    pc++; //increment instruction pointer to the next instruction
    return opcode;
}