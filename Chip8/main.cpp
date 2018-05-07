#include <iostream>
#include <chrono>
#include <ctime>
#include <Windows.h>
#include "Chip8.h"

Chip8 emulator;

void UpdateInput()
{
	emulator.SetInput(1, (GetAsyncKeyState('1') & 0x8000) != 0);
	emulator.SetInput(2, (GetAsyncKeyState('2') & 0x8000) != 0);
	emulator.SetInput(3, (GetAsyncKeyState('3') & 0x8000) != 0);
	emulator.SetInput(4, (GetAsyncKeyState('4') & 0x8000) != 0);

	emulator.SetInput(5, (GetAsyncKeyState('Q') & 0x8000) != 0);
	emulator.SetInput(6, (GetAsyncKeyState('W') & 0x8000) != 0);
	emulator.SetInput(7, (GetAsyncKeyState('E') & 0x8000) != 0);
	emulator.SetInput(8, (GetAsyncKeyState('R') & 0x8000) != 0);

	emulator.SetInput(9, (GetAsyncKeyState('A') & 0x8000) != 0);
	emulator.SetInput(10, (GetAsyncKeyState('S') & 0x8000) != 0);
	emulator.SetInput(11, (GetAsyncKeyState('D') & 0x8000) != 0);
	emulator.SetInput(12, (GetAsyncKeyState('F') & 0x8000) != 0);

	emulator.SetInput(13, (GetAsyncKeyState('Z') & 0x8000) != 0);
	emulator.SetInput(0,  (GetAsyncKeyState('X') & 0x8000) != 0);
	emulator.SetInput(14, (GetAsyncKeyState('C') & 0x8000) != 0);
	emulator.SetInput(15, (GetAsyncKeyState('V') & 0x8000) != 0);
}

int main(int argc, const char *argv[])
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	HWND hWnd = GetConsoleWindow();
	SetConsoleTextAttribute(hConsole, 15 + 96);
	RECT r;
	GetWindowRect(hWnd, &r);
	MoveWindow(hWnd, r.left, r.top, 1024, 512, true);
	COORD coord;

#ifdef DICKS
	if (argc < 2) {
		std::cout << "Drag a chip8 program file onto the program or onto this window to play it." << std::endl;
	}
	else {
		std::cout << "Loading chip8 rom " << argv[1] << "..." std::endl;
		emulator.LoadRom(argv[1]);
	}
#else
	emulator.LoadRom("PONG");
#endif

	auto timeNow = std::chrono::high_resolution_clock::now();
	auto timeLast = timeNow;

	while (emulator.IsRunning()) {
		float time = std::chrono::duration<float>(timeNow - timeLast).count();

		UpdateInput();

		emulator.Cycle(time);
		timeLast = timeNow;
		timeNow = std::chrono::high_resolution_clock::now();

		if (emulator.IsDisplayStale()) {
			// Draw to the console:
			coord.X = coord.Y = 0;
			std::string output = "";
			for (size_t y = 0; y < 32; ++y) {
				for (size_t x = 0; x < 64; ++x) {
					if (!emulator.IsPixelStale(x, y))
						continue;

					coord.X = x * 2;
					coord.Y = y;
					SetConsoleCursorPosition(hConsole, coord);
					if (emulator.GetPixel(x, y)) {
						std::cout << (unsigned char)219;
						std::cout << (unsigned char)219;
						//output += (unsigned char)219;
						//output += (unsigned char)219;
					}
					else std::cout << "  "; //output += "  ";
				}
				//std::cout << /*output <<*/ std::endl;
			}
		}
		Sleep(1);
	}

	system("pause");
	return 0;
}