#include <cstdint>
#include <random>

constant unsigned int MEM_SIZE = 4096;
constant unsigned int NUM_REGS = 16;
constant unsigned int NUM_KEYS = 16;
constant unsigned int VID_H = 32;
constant unsigned int VID_W = 64;
constant unsigned int FRAME_BUFF = VID_H * VID_W;
constant unsigned int STACK_LEVELS = 16;

class Chip8 {
public:
	Chip8();
	void load(char const* file);
	void Cycle();

	uint8_t keypad[NUM_KEYS]{};
	uint32_t video[FRAME_BUFF]{};
	
private:

}