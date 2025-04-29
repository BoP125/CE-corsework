/**
 * config.h - Configuration constants and macros for the pipeline simulator.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Constants for sizes
#define NUM_REGS 32               /* number of registers (MIPS has 32 general purpose) */
#define INST_MEM_SIZE 1024        /* instruction memory size in words */
#define DATA_MEM_SIZE 4096        /* data memory size in bytes */

// Debug/printing configuration
#define DEBUG 0   /* Set to 1 for detailed pipeline debug output */

// MIPS instruction field extraction macros
#define OPCODE(instr)   (((instr) >> 26) & 0x3F)
#define RS(instr)       (((instr) >> 21) & 0x1F)
#define RT(instr)       (((instr) >> 16) & 0x1F)
#define RD(instr)       (((instr) >> 11) & 0x1F)
#define SHAMT(instr)    (((instr) >> 6) & 0x1F)
#define FUNCT(instr)    ((instr) & 0x3F)
#define IMM16(instr)    ((uint16_t)((instr) & 0xFFFF))
#define IMM_SE(instr)   ((int32_t)((int16_t)IMM16(instr)))  /* sign-extended immediate */
#define ADDR26(instr)   ((instr) & 0x03FFFFFF)             /* 26-bit address for jumps */

// ALU operation codes (for internal use)
enum ALUOps {
    ALU_NOP = 0,
    ALU_ADD,
    ALU_SUB,
    ALU_AND,
    ALU_OR,
    ALU_XOR,
    ALU_NOR,
    ALU_SLT,
    ALU_SLL,
    ALU_SRL
};

#endif // CONFIG_H
