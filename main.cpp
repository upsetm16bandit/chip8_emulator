/* Chip 8 Emulator  <main.cpp>
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
// #include <atomic>
#include "chip8.h"


using namespace std;

int main(int argc, char *argv[])
{
    // std::atomic<int> myVar;
    // myVar = 0;
    // char TESTFILENAME[] = "Breakout.ch8";
    char TESTFILENAME[] = "test_vector.ch8";
    CHIP8_EMULATOR emulator;
    emulator.initEmulator();
    if(emulator.loadROM(TESTFILENAME) != STATUS_SUCCESS)
    {
        return ERR_UNABLE_OPEN_FILE;
    }
    unsigned int emulatorCycleCnt = 0;
    while(1)
    {
        ++emulatorCycleCnt;
        emulator.emulatorTick();
        cout << "Emulator Cycle Count: " << emulatorCycleCnt << endl;
    }

    return 0;
}