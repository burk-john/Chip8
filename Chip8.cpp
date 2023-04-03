#include "Chip8.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>

const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;

unsigned char chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x60, 0x20, 0x20, 0x70, //1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
	0x90, 0x90, 0xF0, 0x10, 0x10, //4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
	0xF0, 0x10, 0x20, 0x40, 0x40, //7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
	0xF0, 0x90, 0xF0, 0x90, 0x90, //A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
	0xF0, 0x80, 0x80, 0x80, 0xF0, //C
	0xE0, 0x90, 0x90, 0x90, 0xE0, //D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
	0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};


// CLS clear screen
void Chip8::00E0() {
	memset(video, 0, sizeof(video));
}

// RET return from subroutine
void Chip8::00EE() {
	--sp;
	pc = stack[sp];
}

// JP jump to address
void Chip8::1nnn() {
	uint16_t addr = opcode & 0xFFF;
	pc = addr;
}

// CALL call subroutine at addr nnn
void Chip8::2nnn() {
	uint16_t addr = opcode & 0x0FFF;
	stack[sp] = pc;
	++sp;
	pc = addr;
}

// SE Vx, byte skip if Vx = byte
void Chip8::3xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = opcode & 0x00FF;

	if (regs[Vx] == byte) {
		pc += 2;
	}
}

// SNE Vx, byte skip if Vx != byte
void Chip8::4xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = opcode & 0x00FF;

	if (regs[Vx] != byte) {
		pc += 2;
	}
}

// SE Vx, Vy
void Chip8::5xy0() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	if (regs[Vx] == regs[Vy]) {
		pc += 2;
	}
}

// LD Vx, byte
void Chip8::6xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = (opcode & 0X00FF);
	regs[Vx] = byte;
}

// ADD Vx, byte
void Chip8::7xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = (opcode & 0X00FF);
	regs[Vx] += byte;
}

// LD Vx, Vy
void Chip8::8xy0() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	regs[Vx] = regs[Vy];
}

// OR Vx, Vy
void Chip8::8xy1() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	regs[Vx] |= regs[Vy];
}

// AND Vx, Vy
void Chip8::8xy2() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	regs[Vx] &= regs[Vy];
}

// XOR Vx, Vy
void Chip8::8xy3() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	regs[Vx] ^= regs[Vy];
}

// ADD Vx, Vy
void Chip8::8xy4() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	uint16_t sum = regs[Vx] + regs[Vy];

	if (sum > 255) {
		regs[0xF] = 1;
	}
	else {
		regs[0xF] = 0;
	}

	regs[Vx] = sum & 0x00FF;
}

// SUB Vx, Vy
void Chip8::8xy5() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	uint16_t diff = regs[Vx] - regs[Vy];
	
	if (regs[Vx] > regs[Vy]) {
		regs[0xF] = 1;
	}
	else {
		regs[0xF] = 0;
	}
	regs[Vx] = diff & 0x00FF;
}

// SHR Vx
void Chip8::8xy6() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	regs[0xF] = (regs[Vx] & 0x1);
	regs[Vx] >>= 1;
}

// SUBN Vx, Vy
void Chip8::8xy7() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	uint16_t diff = regs[Vy] - regs[Vx];
	
	if (regs[Vy] > regs[Vx]) {
		regs[0xF] = 1;
	}
	else {
		regs[0xF] = 0;
	}
	regs[Vx] = diff & 0x00FF;
}

// SHL Vx
void Chip8::8xyE() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	regs[0xF] = (regs[Vx] & 0x80);
	regs[Vx] <<= 1;
}

void Chip8::9xy0() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	if (regs[Vx] != regs[Vy]) {
		pc += 2;
	}
}

void Chip8::Annn() {
	uint8_t addr = opcode & 0x0FFF;
	index = addr;
}

void Chip8::Bnnn() {
	uint8_t addr = opcode & 0x0FFF;
	pc = addr + regs[0];
}

void Chip8::Cxkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = (opcode & 0x00FF);
	regs[Vx] = std::uniform_int_distribution < uint8_t > (0, 255)
}