#pragma once
#ifndef CHIP8_H
#define CHIP8_H

#include <string>
#include <vector>

class Chip8
{
private:
	// Emulator-specific data
	enum EmulatorFlags {
		Stopped = 0,
		Running = 0x1,
		AwaitingInput = 0x2,
	} state;
	float fTimerAccum; // Timer
	bool bDisplayStale{ true };
	unsigned char arrBDisplayStale[64][32];
	unsigned char input; // Keyboard input bitmask. Higher numbers are stored further left.
	static const unsigned char glpyhs[80];
	bool CheckIPointerIsValid(); // Prevent out-of-bounds memory access.
	std::string opcodehex; // easier debugging
	unsigned char opcodenibs[4]; // inspect individual nybbles easier

	// Memory and registers
	unsigned char ram[4096]; // 4k memory
	unsigned char registers[16]; // 16 8-bit registers
	unsigned short registerI; // 1 16-bit pointer register
	unsigned char timerSnd, timerDelay; // 2 8-bit timer registers
	unsigned short pc; // 1 16-bit program counter
	std::vector<unsigned short> stack; // Specs occasionally differ on stack depth for chip8; this one is pretty arbitrary.

	unsigned short opcode;

	// Instructions
	void CLS(); // clear screen
	void RET(); // return from subroutine
	void JP(); // jump
	void CALL(); // call
	void SEC(); // skip if equal constant
	void SNEC(); // skip if not equal constant
	void SE(); // skip if equal
	void SNE(); // skip if not equal
	void LDC(); // load constant
	void ADDC(); // add constant
	void LD(); // load register
	void OR(); // or regsisters
	void AND(); // and
	void XOR();
	void ADDVF(); // add with carry
	void SUB(); // sub with borrow
	void SHR(); // right shift
	void SUBN(); // inverse of SUB apparently
	void SHL(); // left shift
	void LDA(); // load address from ram
	void JPM(); // jump (?)
	void RND(); // random
	void DRW(); // draw sprite
	void SKP(); // skip if key down
	void SKPN(); // skip if not down
	void LDT(); // load delay timer into register
	void LDK(); // wait for keypress and store that
	void TIM(); // set delay timer
	void SND(); // set sound timer
	void ADDI(); // add to I (?)
	void LDF(); // load glyph pointer
	void LDB(); // BCD to ram
	void LDI(); // store registers to ram
	void LDS(); // read registers from ram

	// Graphics
	unsigned char display[64][32]; // 64x32 one-bit pixels

public:
	void LoadRom(const std::string &path);
	void Cycle(const float dTime);

	const inline bool IsRunning() {
		return state & 1;
	}
	const inline bool IsSoundOn() {
		return timerSnd > 0;
	};
	inline bool GetPixel(size_t x, size_t y) {
		bDisplayStale = false;
		arrBDisplayStale[x][y] = false;
		return display[x][y] != 0;
	};
	const inline bool IsDisplayStale() {
		return bDisplayStale;
	}
	const inline bool IsPixelStale(size_t x, size_t y) {
		return arrBDisplayStale[x][y];
	}

	void SetInput(const unsigned char number, const bool keydown);

	Chip8();
	~Chip8();
};

#endif
