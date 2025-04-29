/**
 * util.h - Utility functions for register and memory operations, and program loading.
 */
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

// Initialize registers and memories
void reg_init();
void mem_init();

// Register file access
int32_t reg_read(int reg_index);
void reg_write(int reg_index, int32_t value);

// Data memory access (word-aligned)
int32_t mem_read_word(uint32_t address);
void mem_write_word(uint32_t address, int32_t value);

// Load program (binary) into instruction memory. Returns number of instructions loaded.
int load_program(const char *filename);

// Fetch an instruction by index (PC >> 2)
uint32_t instr_read(uint32_t index);

#endif // UTIL_H
