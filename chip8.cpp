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
#include <cstdlib>  //for rand() call
#include <ctime>    //for seeding rand()
#include <ncurses.h>
#include "chip8.h"

using namespace std;

/*
System Memory Map:
0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM 
*/

bool CHIP8_EMULATOR::isRNGSeeded;

CHIP8_EMULATOR::CHIP8_EMULATOR()
{
    if(!CHIP8_EMULATOR::isRNGSeeded)
    { //pseudoRNG hasnt yet been seeded...
        srand(time(0));  //seed the pseudo-RNG
    }
    initEmulator();
}

CHIP8_EMULATOR::~CHIP8_EMULATOR()
{
    //do nothing currently
}

int CHIP8_EMULATOR::initEmulator()
{
    memset(memory, 0, MEMORY_SIZE * sizeof(unsigned char));             //zero out the chips memory regions
    memset(v, 0, CPU_GPR_COUNT * sizeof(unsigned char));                //zero out the cpu's GPRs
    memset(graphics, 255, GFX_BUFFER_SIZE*sizeof(unsigned char));       //zero gfx buffer.  Should this be seperate from system memory?
    delayTimer = 0;
    soundTimer = 0;

    indexRegister = 0;
    programCounter = 0;

    sb = logicalAddressToPhysical(BASE_STACK_OFFSET, 0);
    sp = sb; //stack empty, top is the same as bottom

    isRomLoaded = false;
    sizeOfROM = 0;

    initFont(); //load font into emulator RAM
    initGFX();  //init's our graphics display method

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
    this -> sizeOfROM = (ushort) filesize;
    inFile.close();     //close the ROM file
    positionPC();       //move the program counter to point to start of execution RAM
    return STATUS_SUCCESS;
}

void CHIP8_EMULATOR::positionPC()
{
    programCounter = (ushort*) &memory[BASE_RAM_OFFSET];
}

int CHIP8_EMULATOR::incrementPC(ushort offset)
{   //TODO: perform bounds checking to ensure we dont increment PC outside of executable RAM region
    programCounter += offset; //increment program counter by offset
    return STATUS_SUCCESS;
}

int CHIP8_EMULATOR::setPC(ushort address)
{   //TODO: perform bounds checking
    // programCounter = &memory[BASE_RAM_OFFSET] + address;
    programCounter = logicalAddressToPhysical(address);
    return STATUS_SUCCESS;
}

void CHIP8_EMULATOR::pushAddrToStack(ushort address)
{   //TODO: perform bounds checking on incrementing SP
    if(sp != sb)
    {
        ++sp; //move the stack pointer up by 1 so we can push new value on
    }
    // *sp = ( (address & 0x00FF) << 8 | (address & 0xFF00) >> 8 );    //put new value onto the stack [will be big endian in memory]
    *sp = address;    //put new value onto the stack [will be big endian in memory]    
}

ushort CHIP8_EMULATOR::popAddrFromStack()
{
    ushort addressValue;
    addressValue = (*sp);
    --sp; //move the stack pointer down by 1 since we have removed the value.
    return addressValue;   //validate bounds?
}

ushort CHIP8_EMULATOR::getSizeOfLoadedROM()
{
    return sizeOfROM;
}

ushort* CHIP8_EMULATOR::logicalAddressToPhysical(ushort logicalAddr, ushort base) //this should be validated...
{
    // return (ushort*) &memory[base + (logicalAddr & 0xFFF)];
    return (unsigned short*) &memory[(logicalAddr & 0xFFF)];
}

ushort CHIP8_EMULATOR::physicalAddressToLogical(ushort *physicalAddr)  //this should be validated...
{
    return (ushort) ((char*)physicalAddr - (char*)&memory[BASE_RAM_OFFSET]) &0xFFF;
}

int CHIP8_EMULATOR::emulatorTick()
{
    ushort opcode = 0;
    //Fetch Instruction
    opcode = fetchInstruction();
    #ifdef DEBUG_LOG_OUTPUT    
    int maxRows;
    int maxCols;
    getmaxyx(stdscr, maxRows, maxCols);
    // wmove(stdscr, maxRows, 0);
    mvprintw(maxRows-1, 0, "Fetched Opcode: 0x%4x",opcode);
    wrefresh(stdscr);           //refresh display
    #else    
    cout << "Fetched Opcode: 0x" << hex << opcode << dec << endl;
    #endif

    //Decode Instruction
    decodeAndExecuteInstruction(opcode);  //decode opcode and execute

    //update the display
    refreshDisplay();

    return STATUS_SUCCESS;
}

ushort CHIP8_EMULATOR::fetchInstruction()
{
    ushort opcode = 0;
    // assert((("Tried to fetch an instruction out of bounds!" , (pc >= BASE_RAM_OFFSET && pc <= UPPER_RAM_OFFSET)));
    assert( programCounter >= (ushort*)&memory[BASE_RAM_OFFSET] && programCounter <= (ushort*)&memory[UPPER_RAM_OFFSET] );    //tried to fetch an instruction out of RAM bounds
    opcode = *programCounter;
    programCounter++; //increment instruction pointer to the next instruction
    return ( ((opcode & 0x00FF) <<8) | ((opcode & 0xFF00) >> 8) );  //need to convert from little endian to big endian before returning the opcode
}

int CHIP8_EMULATOR::decodeAndExecuteInstruction(ushort opcode)
{
    /*
     * CHIP-8 has 35 opcodes, which are all two bytes long and stored big-endian. The opcodes are listed below, in hexadecimal and with the following symbols:
     * NNN: address
     * NN: 8-bit constant
     * N: 4-bit constant
     * X and Y: 4-bit register identifier
     * PC : Program Counter
     * I : 16bit register (For memory address) (Similar to void pointer)
     */
    int returnValue = STATUS_SUCCESS;
    //skipping implementation of opcode 0NNN :Calls RCA 1802 program at address NNN. Not necessary for most ROMs.
    switch(opcode & 0xF000)
    {
        case 0x0000:
            switch(opcode & 0x00FF)
            {
                case 0x00E0:    //0x00E0
                    //Clears the screen.
                    // TODO: Implement. Should this clear out the gfx buffer too?
                    refresh(); // same as wrefresh(stdscr)
                    break;

                case 0x00EE:    //0x00EE
                    //Returns from a subroutine.
                    //pop return address off the stack and put it into programCounter
                    programCounter = logicalAddressToPhysical(popAddrFromStack());
                    break;

                default:
                    cerr << "Unrecognized opcode: 0x" << hex << opcode << dec << endl;
                    returnValue = ERR_INVALID_OPCODE;
            }
            break;

        case 0x1000:    //0x1NNN
            //goto NNN
            //TODO: perhaps perform bounds checking
            setPC(opcode & 0x0FFF); //jmp to NNN by setting the PC there
            break;

        case 0x2000:    //0x2NNN
            //Call subroutine at NNN
            pushAddrToStack(physicalAddressToLogical(programCounter));    //push current pc value onto the stack
            setPC(opcode & 0x0FFF);             //jump to the subroutine
            break;

        case 0x3000:    //0x3XNN
            //Skips the next instruction if VX equals NN. (Usually the next instruction is a jump to skip a code block)
            if(v[(opcode & 0x0F00)>>8] == (opcode & 0x00FF))
            {                
                incrementPC();  //increment the program counter to skip the next instruction
            }
            break;

        case 0x4000:    //0x4XNN
            //Skips the next instruction if VX doesn't equal NN. (Usually the next instruction is a jump to skip a code block)
            if(v[(opcode & 0x0F00)>>8] != (opcode & 0x00FF))
            {                
                incrementPC();  //increment the program counter to skip the next instruction
            }
            break;

        case 0x5000:    //0x5XY0
            //Skips the next instruction if VX equals VY. (Usually the next instruction is a jump to skip a code block)
            if(v[(opcode & 0x0F00)>>8] == v[(opcode & 0x00F0)>>4])
            {                
                incrementPC();  //increment the program counter to skip the next instruction
            }
            break;

        case 0x6000:    //0x6XNN
            //Sets VX to NN
            v[(opcode & 0x0F00)>>8] = opcode & 0x00FF;
            break;

        case 0x7000:    //0x7XNN
            //Adds NN to VX. (Carry flag is not changed)
            v[(opcode & 0x0F00)>>8] += opcode & 0x00FF;
            break;

        case 0x8000:    //0x8XY0 - 0x8XYE
            switch(opcode & 0x000F)
            {
                case 0x0000:        //0x8XY0
                    //Sets VX to the value of VY.
                    v[(opcode & 0x0F00)>>8] = v[(opcode & 0x00F0)>>4];
                    break;

                case 0x0001:        //0x8XY1
                    //Sets VX to VX or VY. (Bitwise OR operation)
                    v[(opcode & 0x0F00)>>8] |= v[(opcode & 0x00F0)>>4];
                    break;

                case 0x0002:        //0x8XY2
                    //Sets VX to VX and VY. (Bitwise AND operation)
                    v[(opcode & 0x0F00)>>8] &= v[(opcode & 0x00F0)>>4];
                    break;

                case 0x0003:        //0x8XY3
                    //Sets VX to VX xor VY.
                    v[(opcode & 0x0F00)>>8] ^= v[(opcode & 0x00F0)>>4];
                    break;

                case 0x0004:        //0x8XY4
                    //Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
                    if( ((ushort) v[(opcode & 0x0F00)>>8] + (ushort) v[(opcode & 0x00F0)>>4]) >255) //we need to set the carry flag
                    {
                        v[0xF] = 1;
                    }
                    else
                    {
                        v[0xF] = 0;
                    }
                    v[(opcode & 0x0F00)>>8] += v[(opcode & 0x00F0)>>4];
                    break;

                case 0x0005:        //0x8XY5
                    //VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
                    if (v[(opcode & 0x0F00)>>8] < v[(opcode & 0x00F0)>>4])  //Vx < Vy  when computing Vx - Vy a borrow must be performed
                    {
                        //borrow performed, set v[0xF] (carry flag) = 0
                        v[0xF] = 0;
                    }
                    else
                    {
                        //no borrow performed, set v[0xF] (carry flag) = 1
                        v[0xF] = 1;
                    }
                    v[(opcode & 0x0F00)>>8] -= v[(opcode & 0x00F0)>>4];  //Vx = Vx - Vy
                    break;

                case 0x0006:        //0x8XY6
                    //Stores the least significant bit of VX in VF and then shifts VX to the right by 1
                    v[0xF] = v[(opcode & 0x0F00)>>8] & 0x0001;
                    v[(opcode & 0x0F00)>>8] = v[(opcode & 0x0F00)>>8] >> 1;
                    break;

                case 0x0007:        //0x8XY7
                    //Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                    if (v[(opcode & 0x00F0)>>4] < v[(opcode & 0x0F00)>>8])  //Vy < Vx  when computing Vy - Vx a borrow must be performed
                    {
                        //borrow performed, set v[0xF] (carry flag) = 0
                        v[0xF] = 0;
                    }
                    else
                    {
                        //no borrow performed, set v[0xF] (carry flag) = 1
                        v[0xF] = 1;
                    }
                    v[(opcode & 0x0F00)>>8] = v[(opcode & 0x00F0)>>4] - v[(opcode & 0x0F00)>>8];  //Vx = Vy - Vx
                    break;

                case 0x000E:        //0x8XYE
                    //Stores the most significant bit of VX in VF and then shifts VX to the left by 1
                    v[0xF] = (v[(opcode & 0x0F00)>>8] & 0x80) >> 7;
                    v[(opcode & 0x0F00)>>8] <<= 1;
                    break;

                default:
                    cerr << "Unrecognized opcode: 0x" << hex << opcode << dec << endl;
                    returnValue = ERR_INVALID_OPCODE;
            }
            break;

        case 0x9000:    //0x9XY0
            //Skips the next instruction if VX doesn't equal VY. (Usually the next instruction is a jump to skip a code block)
            if(v[(opcode & 0x0F00)>>8] != v[(opcode & 0x00F0)>>4])
            {
                // programCounter++; //increment the program counter to skip the next instruction
                incrementPC();  //increment the program counter to skip the next instruction
            }
            break;

        case 0xA000:    //0xANNN
            //Sets IndexRegister to the address NNN.
            indexRegister = opcode & 0x0FFF;
            break;

        case 0xB000:    //0xBNNN
            //Jumps to the address NNN plus V0.
            setPC(v[0] + (opcode & 0x0FFF) );
            break;

        case 0xC000:    //0xCXNN
            //Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
            v[(opcode & 0x0F00)>>8] = (rand() % 256) & (opcode & 0x00FF);
            break;

        case 0xD000:    //0xDXYN
            /*Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. Each row of 
             *8 pixels is read as bit-coded starting from memory location I; I value doesn’t change after the execution 
             of this instruction. As described above, VF is set to 1 if any screen pixels are flipped from set to unset 
             when the sprite is drawn, and to 0 if that doesn’t happen*/
             cerr << "opcode: 0x not yet implemented..." << hex << opcode << dec << endl;
             break;

        case 0xE000:    //0xEX9E or 0xExA1
            switch(opcode & 0xF0FF)
            {
                case 0xE09E:    //0xEX9E
                    //Skips the next instruction if the key stored in VX is pressed. (Usually the next instruction is a jump to skip a code block)
                    break;

                case 0xE0A1:    //0xE0A1
                    //Skips the next instruction if the key stored in VX isn't pressed. (Usually the next instruction is a jump to skip a code block)
                    break;

                default:
                    cerr << "Unrecognized opcode: 0x" << hex << opcode << dec << endl;
                    returnValue = ERR_INVALID_OPCODE;
            }
            break;

        case 0xF000:    //0xFX07 or 0xFX0A or 0xFX15 or 0xFX18 or 0xFX1E or 0xFX29 or 0xFX33 or 0xFX55 or 0xFX65
            switch(opcode & 0xF0FF)
            {
                case 0xF007:    //0xFX07
                    //Sets VX to the value of the delay timer.
                    v[(opcode & 0x0F00)>>8] = delayTimer;
                    break;

                case 0xF00A:    //0xFX0A
                    //A key press is awaited, and then stored in VX. (Blocking Operation. All instruction halted until next key event)
                    v[(opcode & 0x0F00)>>8] = (unsigned char) getchar();  //TODO: should this be implemented with curses?
                    break;

                case 0xF015:    //0xFX15
                    //Sets the delay timer to VX.
                    delayTimer = v[(opcode & 0x0F00)>>8];
                    break;

                case 0xF018:    //0xFX18
                    //Sets the sound timer to VX.
                    soundTimer = v[(opcode & 0x0F00)>>8];
                    break;

                case 0xF01E:    //0xFX1E
                    //Adds VX to indexRegister
                    indexRegister += v[(opcode & 0x0F00)>>8];
                    break;

                case 0xF029:    //0xFX29
                    //Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font
                    break;

                case 0xF033:    //0xFX33
                    /* Stores the binary-coded decimal representation of VX, with the most significant of three digits at the address in I,
                     * the middle digit at I plus 1, and the least significant digit at I plus 2. (In other words, take the decimal 
                     * representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the 
                     * ones digit at location I+2.)
                    */
                    break;

                case 0xF055:    //0xFX55
                    //Stores V0 to VX (including VX) in memory starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified.
                    for(char i = 0; i <= (opcode & 0x0F00)>>8; i++)
                    {
                        memory[BASE_RAM_OFFSET + indexRegister + (ushort)i] = v[(ushort)i];
                    }
                    break;

                case 0xF065:    //0xFX65
                    //Fills V0 to VX (including VX) with values from memory starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified.
                    for(char i = 0; i <= (opcode & 0x0F00)>>8; i++)
                    {
                        v[(ushort)i] = memory[BASE_RAM_OFFSET + indexRegister + (ushort)i];
                    }
                    break;

                default:
                    cerr << "Unrecognized opcode: 0x" << hex << opcode << dec << endl;
                    returnValue = ERR_INVALID_OPCODE;
            }
            break;

        default:
            cerr << "Unrecognized opcode: 0x" << hex << opcode << dec << endl;
            returnValue = ERR_INVALID_OPCODE;
    }
    return returnValue;
}

void CHIP8_EMULATOR::initGFX()
{
    initscr(); //init ncurses screen
    cbreak();   //disable tty buffering and get 1 character at a time
    noecho();   //dont echo back typed characters
    //default ncurses win is called stdscr
    wmove(stdscr, 0, 0);        //move cursor to position (0,0)
    wrefresh(stdscr);           //refresh display
}

void CHIP8_EMULATOR::refreshDisplay()
{
    wmove(stdscr, 0, 0);

    #ifdef DEBUG_LOG_OUTPUT
    getch();
    #endif

    for (int row = 0; row < GFX_ROWS; row++)
    {
        for(int col = 0; col < GFX_COLS; col++)
        {
            wmove(stdscr, row, col);
            for(int i = 7; i >= 0; i--)
            {
                bool pixelValue = (graphics[(row * GFX_COLS) + col] >> i) & 0x1;
                if(pixelValue)
                {
                    //pixel set, display to screen
                    // wmove(stdscr, row, col*(7-i));
                    waddch(stdscr,'*');
                }
                else
                {
                    //pixel not set
                    // wmove(stdscr, row, col+(7-i));
                    waddch(stdscr,' ');
                }
                
            }
        }
    }
    wmove(stdscr, 0, 0);        //move cursor to position (0,0)
    wrefresh(stdscr);           //refresh display
}

void CHIP8_EMULATOR::initFont()
{
    const unsigned char fontData[16][5] = {{0xF0, 0x90, 0x90, 0x90, 0xF0},    //0
                               {0x20, 0x60, 0x20, 0x20, 0x70},    //1
                               {0xF0, 0x10, 0xF0, 0x80, 0xF0},    //2
                               {0xF0, 0x10, 0xF0, 0x10, 0xF0},    //3
                               {0x90, 0x90, 0xF0, 0x10, 0x10},    //4
                               {0xF0, 0x80, 0xF0, 0x10, 0xF0},    //5
                               {0xF0, 0x80, 0xF0, 0x90, 0xF0},    //6
                               {0xF0, 0x10, 0x20, 0x40, 0x40},    //7
                               {0xF0, 0x90, 0xF0, 0x90, 0xF0},    //8
                               {0xF0, 0x90, 0xF0, 0x10, 0xF0},    //9
                               {0xF0, 0x90, 0xF0, 0x90, 0x90},    //A
                               {0xE0, 0x90, 0xE0, 0x90, 0xE0},    //B
                               {0xF0, 0x80, 0x80, 0x80, 0xF0},    //C
                               {0xE0, 0x90, 0x90, 0x90, 0xE0},    //D
                               {0xF0, 0x80, 0xF0, 0x80, 0xF0},    //E
                               {0xF0, 0x80, 0xF0, 0x80, 0x80}     //F
                               };
    memcpy(memory, fontData, 80); //load the Fonts into emulator memory 
}