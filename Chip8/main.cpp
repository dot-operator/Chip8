#include <iostream>
#include "Chip8.h"

Chip8 emulator;

int main(int argc, const char *argv[])
{
#ifndef _DEBUG
	if (argc < 2) {
		std::cout << "Drag a chip8 program file onto the program or onto this window to play it." << std::endl;
	}
	else {
		emulator.LoadRom(argv[1]);
	}
#endif
	emulator.LoadRom("pong.ch8");
	return 0;
}