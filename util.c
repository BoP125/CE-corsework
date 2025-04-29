/**
 * util.c - Implementation of registers, memory, and program loading utilities.
 */
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "util.h"

// Register file and memories
static int32_t registers[NUM_REGS];
static uint8_t data_mem[DATA_MEM_SIZE];
static uint32_t instr_mem[INST_MEM_SIZE];
static int instr_count = 0; // number of instructions loaded

// Initialize registers and memory to zero
void reg_init() {
    for (int i = 0; i < NUM_REGS; ++i) {
        registers[i] = 0;
    }
}
void mem_init() {
    memset(data_mem, 0, sizeof(data_mem));
}

// Read register (returns 0 for register 0 regardless of value, as in MIPS)
int32_t reg_read(int reg_index) {
    if (reg_index < 0 || reg_index >= NUM_REGS) {
        return 0;
    }
    if (reg_index == 0) {
        return 0; // $zero is always 0
    }
    return registers[reg_index];
}

// Write register (ignores writes to register 0)
void reg_write(int reg_index, int32_t value) {
    if (reg_index < 0 || reg_index >= NUM_REGS) {
        return;
    }
    if (reg_index == 0) {
        return; // cannot write to $zero
    }
    registers[reg_index] = value;
}

// Read a 32-bit word from data memory (assume word-aligned address)
int32_t mem_read_word(uint32_t address) {
    if (address + 3 < DATA_MEM_SIZE) {
        // combine bytes (assuming little-endian)
        int32_t value = 0;
        value |= data_mem[address];
        value |= data_mem[address + 1] << 8;
        value |= data_mem[address + 2] << 16;
        value |= data_mem[address + 3] << 24;
        return value;
    } else {
        fprintf(stderr, "Data memory read out of bounds at 0x%08x\n", address);
        return 0;
    }
}

// Write a 32-bit word to data memory (assume word-aligned address)
void mem_write_word(uint32_t address, int32_t value) {
    if (address + 3 < DATA_MEM_SIZE) {
        // break value into bytes (assuming little-endian)
        data_mem[address]     = (uint8_t)(value & 0xFF);
        data_mem[address + 1] = (uint8_t)((value >> 8) & 0xFF);
        data_mem[address + 2] = (uint8_t)((value >> 16) & 0xFF);
        data_mem[address + 3] = (uint8_t)((value >> 24) & 0xFF);
    } else {
        fprintf(stderr, "Data memory write out of bounds at 0x%08x\n", address);
    }
}

// Load a binary program file into instruction memory. Returns number of instructions loaded.
int load_program(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open program file: %s\n", filename);
        return -1;
    }
    // Read file content into instruction memory array
    uint8_t byte;
    uint32_t word = 0;
    int byte_count = 0;
    instr_count = 0;
    while (fread(&byte, 1, 1, file) == 1) {
        word = (word << 8) | byte; // assemble bytes into a 32-bit word (big-endian order)
        byte_count++;
        if (byte_count % 4 == 0) {
            // one instruction (4 bytes) assembled
            if (instr_count < INST_MEM_SIZE) {
                instr_mem[instr_count++] = word;
            } else {
                fprintf(stderr, "Instruction memory overflow, too many instructions\n");
                break;
            }
            word = 0;
        }
    }
    fclose(file);
    // If byte_count not multiple of 4, pad remaining bytes to complete a word
    if (byte_count % 4 != 0 && instr_count < INST_MEM_SIZE) {
        while (byte_count % 4 != 0) {
            word <<= 8;
            byte_count++;
        }
        instr_mem[instr_count++] = word;
    }
    return instr_count;
}

// Get instruction from instruction memory at a given word address (PC/4)
uint32_t instr_read(uint32_t index) {
    if (index < INST_MEM_SIZE) {
        return instr_mem[index];
    } else {
        return 0;
    }
}
