#include "Chip8.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>

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
	// Clear all the registers.
	std::fill(registers, registers + 16, 0);
	registerI = 0;
	timerSnd = timerDelay = 0;
	pc = 0x200; // Program memory starts here.
	stack.clear();

	// Initialize the interpreter's dedicated portion of the memory
	// A program can technically overwrite any portion in memory
	// so it needs to be reset before beginning a new one.
	memcpy_s(ram, 80, glpyhs, 80);

	// Zero initialize
	std::fill(&ram[80], &ram[4096], 0);

	// Load the program rom
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		state = Stopped;
		std::cerr << "Couldn't open " << path << ".\n";
		return;
	}
	auto size = file.tellg();
	if (size > 3584) {
		state = Stopped;
		std::cerr << path << " too big for 4k ram by " << ((unsigned)size - 3584) << " bytes.\n";
		return;
	}
	file.seekg(0);
	file.read((char*)(&ram[512]), size); // 4096 bytes, starting at 0x200, leaves 3584 for program

	
	state = Running;
}

void Chip8::Cycle(const float dTime)
{
	// Don't cycle if we're holding for input
	if (state & AwaitingInput) {
		return;
	}

	// fetch
	opcode = (ram[pc] << 8) | (ram[pc + 1]);
	std::stringstream hexconvert;
	hexconvert << std::hex << opcode;
	opcodehex = hexconvert.str();
	opcodenibs[0] = ram[pc] >> 4;
	opcodenibs[1] = ram[pc] & 0x0F;
	opcodenibs[2] = ram[pc + 1] >> 4;
	opcodenibs[3] = ram[pc + 1] & 0x0F;

	// execute
	// switch on the leftmost four bits
	switch (opcode >> 12) {
	default:
		std::cerr << "Fatal error at " << std::hex << pc << ": Opcode " << std::hex << opcode << \
			" not recognized.\nProgram will exit.\n";
		state = Stopped;
		return;
	case 0:
		switch (opcode) {
		default:
			std::cerr << "Fatal error at " << std::hex << pc << ": Opcode " << std::hex << opcode << \
				" calls for a jump to machine instructions and is not supported.\nProgram will exit.\n";
			state = Stopped;
			return;
		case 0x00E0:
			CLS();
			break;
		case 0x00EE:
			RET();
			break;
		}
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
		switch (opcodenibs[3]) {
		default:
			std::cerr << "Fatal error at " << std::hex << pc << ": Opcode " << std::hex << opcode << \
				" not recognized.\nProgram will exit.\n";
			state = Stopped;
			return;
		case 0:
			LD();
			break;
		case 1:
			OR();
			break;
		case 2:
			AND();
			break;
		case 3:
			XOR();
			break;
		case 4:
			ADDVF();
			break;
		case 5:
			SUB();
			break;
		case 6:
			SHR();
			break;
		case 7:
			SUBN();
			break;
		case 0xE:
			SHL();
			break;
		}
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
		switch (opcode & 0x00FF) {
		default:
			std::cerr << "Fatal error at " << std::hex << pc << ": Opcode " << std::hex << opcode << \
				" not recognized.\nProgram will exit.\n";
			state = Stopped;
			return;
		case 0x9E:
			SKP();
			break;
		case 0xA1:
			SKPN();
			break;
		}
		break;
	case 0xF:
		switch (opcode & 0x00FF) {
		default:
			std::cerr << "Fatal error at " << std::hex << pc << ": Opcode " << std::hex << opcode << \
				" not recognized.\nProgram will exit.\n";
			state = Stopped;
			return;
		case 0x07:
			LDT();
			break;
		case 0x0A:
			LDK();
			break;
		case 0x15:
			TIM();
			break;
		case 0x18:
			SND();
			break;
		case 0x1E:
			ADDI();
			break;
		case 0x29:
			LDF();
			break;
		case 0x33:
			LDB();
			break;
		case 0x55:
			LDI();
			break;
		case 0x65:
			LDS();
			break;
		}
		break;
	}

	// timers update
	fTimerAccum += dTime;
	while (fTimerAccum >= 0.016667f) {
		fTimerAccum -= 0.016667f;
		--timerSnd = std::max(timerSnd, (unsigned char)0);
		--timerDelay = std::max(timerDelay, (unsigned char)0);
	}

	// increment program counter
	pc += 2;
	if (pc > 4094) {
		std::cerr << "Fatal error: program counter overflowed.\nProgram will quit.";
		state = Stopped;
	}
}

void Chip8::SetInput(const unsigned char number, const bool keydown)
{
	if (!keydown) {
		input &= ~(1 << number);
	}
	else {
		input |= 1 << number;

		if (state & AwaitingInput) {
			registers[opcodenibs[1]] = number;
			state = Running;
		}
	}
}

Chip8::Chip8()
{
}


Chip8::~Chip8()
{
}

// Check to see if the value in Register I is in range of ram.

bool Chip8::CheckIPointerIsValid()
{
	if (registerI > 4095) {
		std::cerr << "Fatal error at " << std::hex << pc << ": Attempted to set register I to an address outside of memory range.\nProgram will quit.\n";
		state = Stopped;
		return false;
	}
	return true;
}

// 00E0: Clear the display.
void Chip8::CLS()
{
	bDisplayStale = true;
	for (size_t y = 0; y < 32; ++y) {
		std::fill(&display[0][y], &display[63][y], 0);
		std::fill(&arrBDisplayStale[0][y], &arrBDisplayStale[63][y], true);
	}
}

// 00EE: Pop stack onto program counter.
// Jump to that address, minus 2 bytes to account for pc increment.
void Chip8::RET()
{
	if (stack.empty()) {
		std::cerr << "Fatal error at " << std::hex << pc << \
			"Attempted to return to an address on the stack, but the stack was empty.\n" << \
			"Program will exit.\n";
		state = Stopped;
		return;
	}
	pc = stack.back();
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
	if (registers[opcodenibs[1]] == (opcode & 0x00FF)) {
		pc += 2;
	}
}

// 4XKK: Compare value in register X with constant KK.
// Skip one instruction if they are not equal.
void Chip8::SNEC()
{
	if (registers[opcodenibs[1]] != (opcode & 0x00FF)) {
		pc += 2;
	}
}

// 5XY0: Compare value in register X with value in register Y.
// Skip one instruction if they are equal.
void Chip8::SE()
{
	if (registers[opcodenibs[1]] == registers[opcodenibs[2]]) {
		pc += 2;
	}
}

// 9XY0: Compare value in register X with value in register Y.
// Skip one instruction if they are not equal.
void Chip8::SNE()
{
	if (registers[opcodenibs[1]] != registers[opcodenibs[2]]) {
		pc += 2;
	}
}

// 6XKK: Load constant KK into register X.
void Chip8::LDC()
{
	registers[opcodenibs[1]] = (opcode & 0x00FF);
}

// 7XKK: Add constant KK into register X.
void Chip8::ADDC()
{
	registers[opcodenibs[1]] += (opcode & 0x00FF);
}

// 8XY0: Store value of register Y in register X.
void Chip8::LD()
{
	registers[opcodenibs[1]] = registers[opcodenibs[2]];
}

// 8XY1: Bitwise Value of X OR value of Y stored in register X.
void Chip8::OR()
{
	registers[opcodenibs[1]] |= registers[opcodenibs[2]];
}

// 8XY2: Bitwise Value of X AND value of Y stored in register X.
void Chip8::AND()
{
	registers[opcodenibs[1]] &= registers[opcodenibs[2]];
}

// 8XY3 Bitwise Value of X XOR value of Y stored in register X.
void Chip8::XOR()
{
	registers[opcodenibs[1]] ^= registers[opcodenibs[2]];
}

// 8XY4: Add value of register X and value of register Y.
// Set carry flag if result is >255.
// Store result of add in register X.
void Chip8::ADDVF()
{
	int result = registers[opcodenibs[1]] + registers[opcodenibs[2]];
	registers[0xF] = (result > 255);
	registers[opcodenibs[1]] = result;
}

// 8XY5: Subtract value of register X and value of register Y.
// Set carry flag if result is >= 0.
// Store result of subtraction in register X.
void Chip8::SUB()
{
	unsigned result = registers[opcodenibs[1]] - registers[opcodenibs[2]];
	registers[0xF] = (result > 255);
	registers[opcodenibs[1]] = result;
}

// 8XY6: Set least-significant bit 
// Store right-hand bit shift of register Y's value in register X.
void Chip8::SHR()
{
	registers[0xF] = registers[opcodenibs[2]] & 0x01;
	registers[opcodenibs[1]] = (registers[opcodenibs[2]] >> 1);
}

// 8XY7: Subtract value of register Y and value of register X.
// Set carry flag if result is >= 0.
// Store result of subtraction in register X.
void Chip8::SUBN()
{
	unsigned result = registers[opcodenibs[2]] - registers[opcodenibs[1]];
	registers[0xF] = (result > 255);
	registers[opcodenibs[1]] = result;
}

// 8XYE: Store left-hand bit shift of register Y's value in register X.
void Chip8::SHL()
{
	registers[0xF] = registers[opcodenibs[2]] & (1 >> 7);
	registers[opcodenibs[1]] = (registers[opcodenibs[2]] << 1);
}

// ANNN: Store address NNN in register I.
void Chip8::LDA()
{
	registerI = (opcode & 0x0FFF);
	CheckIPointerIsValid();
}

// BNNN: Jump to address (NNN + Value of Register 0)
// Subtract 2 to account for program counter increment in cycle loop.
void Chip8::JPM()
{
	pc = (opcode & 0x0FFF) + registers[0] - 2;
}

// CXKK: Random number AND NN is stored in register X.
void Chip8::RND()
{
	registers[opcodenibs[1]] = (rand() % 255) & (opcode & 0x00FF);
}

// DXYN: Draw N lines as a sprite on the screen using bytes from memory at the address in I.
// Position the sprite at (value of register X, value of register Y).
// XOR the drawn sprite with the existing framebuffer at that position.
// Set VF to 1 if an XOR results in a 0; set to 0 otherwise.
void Chip8::DRW()
{
	registerI += 8 * (opcodenibs[3]);
	if (!CheckIPointerIsValid())
		return;
	registerI -= 8 * (opcodenibs[3]);

	registers[0xF] = 0;

	for (size_t i = 0; i < (opcodenibs[3]); ++i) {
		unsigned short y = (i + registers[opcodenibs[2]]) % 32;
		for (size_t j = 0; j < 8; ++j) {
			unsigned short x = (j + registers[opcodenibs[1]]) % 64;
			
			bool drawbit = ram[registerI + i] & (1 << (7 - j));
			if (drawbit) {
				auto before = display[x][y];
				display[x][y] ^= drawbit;
				if (before != display[x][y])
					registers[0xF] = 1;
				bDisplayStale = true;
				arrBDisplayStale[x][y] = true;
			}
		}
	}
}

// EX9E: Skip the next instruction if the input in the value of register X is currently active.
void Chip8::SKP()
{
	if (input & (1 << registers[opcodenibs[1]])) {
		pc += 2;
	}
}

// EXA1: Skip the next instruction if the input in the value of register X is not currently active.
void Chip8::SKPN()
{
	if (! (input & (1 << registers[opcodenibs[1]]))) {
		pc += 2;
	}
	else {

	}
}

// FX07: Load the value of the delay timer into register X.
void Chip8::LDT()
{
	registers[opcodenibs[1]] = timerDelay;
}

// FX0A: Pause execution until a key is pressed.
// Load the key pressed into register X.
void Chip8::LDK()
{
	state = EmulatorFlags((unsigned)state | (unsigned)AwaitingInput);
}

// FX15: Set the delay timer to the value of register X.
void Chip8::TIM()
{
	timerDelay = registers[opcodenibs[1]];
}

// FX18: Set the sound timer to the value of register X.
void Chip8::SND()
{
	timerSnd = registers[opcodenibs[1]];
}

// FX1E: Add the value of register X to register I.
void Chip8::ADDI()
{
	registerI += registers[opcodenibs[1]];
	CheckIPointerIsValid();
}

// FX29: Set I to the address of the glph representing the value of register X.
void Chip8::LDF()
{
	auto glyph = registers[opcodenibs[1]];
	registerI = glyph * 5; // assuming all the glyphs are stored starting at 0 in ram.
	CheckIPointerIsValid();
}

// FX33: Store a 3-digit Binary-Coded Decimal in 3 memory cells in ram, starting at I.
// Use the value of register X to create the BCD value.
void Chip8::LDB()
{
	registerI += 2;
	if (!CheckIPointerIsValid())
		return;

	registerI -= 2;

	auto val = registers[opcodenibs[1]];
	ram[registerI] = val / 100;
	ram[registerI + 1] = (val % 100) / 10;
	ram[registerI + 2] = (val % 10);
}

// FX55: Store registers in memory starting at address I.
void Chip8::LDI()
{
	registerI += opcodenibs[1] + 1;
	if (!CheckIPointerIsValid()) {
		return;
	}
	registerI -= opcodenibs[1] + 1;

	memcpy_s(&ram[registerI], opcodenibs[1], registers, opcodenibs[1]);
	registerI += opcodenibs[1] + 1;
}

// FX65: Load registers from memory, starting at address I.
void Chip8::LDS()
{
	registerI += opcodenibs[1] + 1;
	if (!CheckIPointerIsValid())
		return;
	registerI -= opcodenibs[1] + 1;

	memcpy_s(registers, opcodenibs[1], &ram[registerI], opcodenibs[1]);
	registerI += opcodenibs[1] + 1;
}
