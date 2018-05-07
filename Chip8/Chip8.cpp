#include "Chip8.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// Glyph memory, loaded into interpreter ram on each reset
const unsigned char Chip8::glpyhs[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void Chip8::LoadRom(const std::string & path)
{
	// Initialize the interpreter's dedicated portion of the memory
	// A program can technically overwrite any portion in memory
	// so it needs to be reset before beginning a new one.
	memcpy_s(ram, 80, glpyhs, 80);

	// Zero initialize
	std::fill(&ram[80], &ram[4096], 0);

	// Load the program rom
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		state = EmulatorState::Stopped;
		std::cerr << "Couldn't open " << path << ".\n";
		return;
	}
	auto size = file.tellg();
	if (size > 3584) {
		state = EmulatorState::Stopped;
		std::cerr << path << " too big for 4k ram by " << ((unsigned)size - 3584) << " bytes.\n";
		return;
	}
	file.seekg(0);
	file.read((char*)(&ram[512]), size); // 4096 bytes, starting at 0x200, leaves 3584 for program
}

void Chip8::Cycle(const float dTime)
{
	// fetch
	opcode = (ram[pc] << 8) | (ram[pc + 1]);
	// execute
	// switch on the leftmost four bits
	switch (opcode >> 12) {
	default: break;
	case 0:
		std::cout << "0x0 instructions not implemented.\n";
		break;
	case 1:
		JP();
		break;
	case 2:
		CALL();
		break;
	case 3:
		SEC();
		break;
	case 4:
		SNEC();
		break;
	case 5:
		SE();
		break;
	case 6:
		LDC();
		break;
	case 7:
		ADDC();
		break;
	case 8:
		std::cout << "0x8 instructions not implemented.\n";
		break;
	case 9:
		SNE();
		break;
	case 0xA:
		LDA();
		break;
	case 0xB:
		JPM();
		break;
	case 0xC:
		RND();
		break;
	case 0xD:
		DRW();
		break;
	case 0xE:
		std::cout << "0xE instructions not implemented.\n";
		break;
	case 0xF:
		std::cout << "0xF instructions not implemented.\n";
	}

	// timers update
	fTimerAccum += dTime;
	while (fTimerAccum >= (1 / 60)) {
		fTimerAccum -= (1 / 60);
		--timerSnd = std::max(timerSnd, (unsigned char)0);
		--timerDelay = std::max(timerDelay, (unsigned char)0);
	}
}

Chip8::Chip8()
{
}


Chip8::~Chip8()
{
}

// 00E0: Clear the display.
void Chip8::CLS()
{
	std::fill(display, display + (8 * 32), 0);
}

// 00EE: Pop stack onto program counter.
// Jump to that address, minus 2 bytes to account for pc increment.
void Chip8::RET()
{
	pc = stack.back() - 2;
	stack.pop_back();
}

// 1NNN: Jump to the specified instruction.
// Subtract 2 bytes to account for program counter increment.
void Chip8::JP()
{
	pc = (opcode & 0x0FFF) - 2;
}

// 2NNN: Push current program counter value onto the stack.
// Jump to the specified instruction.
// Subtract 2 bytes to account for program counter increment.
void Chip8::CALL()
{
	stack.push_back(pc);
	pc = (opcode & 0x0FFF) - 2;
}

// 3XKK: Compare value in register X with constant KK.
// Skip one instruction if they are equal.
void Chip8::SEC()
{
	if (registers[(opcode >> 8) & 0x0F] == (opcode & 0x00FF)) {
		pc += 2;
	}
}

// 4XKK: Compare value in register X with constant KK.
// Skip one instruction if they are not equal.
void Chip8::SNEC()
{
	if (registers[(opcode >> 8) & 0x0F] != (opcode & 0x00FF)) {
		pc += 2;
	}
}

// 5XY0: Compare value in register X with value in register Y.
// Skip one instruction if they are equal.
void Chip8::SE()
{
	if (registers[(opcode >> 8) & 0x0F] == registers[(opcode >> 4) & 0x00F]) {
		pc += 2;
	}
}
