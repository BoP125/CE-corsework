/**
 * main.c - Main pipeline simulation controller (5-stage pipeline without forwarding).
 */
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "util.h"
#include "hazard.h"
#include "alu.h"

// Pipeline register structures
typedef struct {
    uint8_t valid;
    uint32_t instr;
    uint32_t pc;
} IFID_t;

typedef struct {
    uint8_t valid;
    uint32_t instr;
    uint32_t pc;
    uint8_t rs, rt, rd;
    int32_t rs_val, rt_val;
    int32_t imm;       // sign-extended immediate or shift amount
    uint8_t destReg;
    uint8_t regWrite;
    uint8_t memRead;
    uint8_t memWrite;
    uint8_t ALUop;
    uint8_t branch; // 1 for beq, 2 for bne
    uint8_t jump;   // 1 for J, 2 for JR
} IDEX_t;

typedef struct {
    uint8_t valid;
    uint32_t instr;
    uint32_t pc;
    int32_t alu_result;
    int32_t store_val;
    uint8_t destReg;
    uint8_t regWrite;
    uint8_t memRead;
    uint8_t memWrite;
} EXMEM_t;

typedef struct {
    uint8_t valid;
    uint32_t instr;
    int32_t write_val;
    uint8_t destReg;
    uint8_t regWrite;
} MEMWB_t;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <program.bin>\n", argv[0]);
        return 1;
    }
    // Initialize state
    reg_init();
    mem_init();
    // Load program
    int inst_count = load_program(argv[1]);
    if (inst_count < 0) {
        return 1;
    }
    // Pipeline registers initial state (empty)
    IFID_t IFID = {0};
    IDEX_t IDEX = {0};
    EXMEM_t EXMEM = {0};
    MEMWB_t MEMWB = {0};
    uint32_t PC = 0;
    uint8_t fetch_enable = 1;
    int cycle = 0;
    int instructions_executed = 0;
    // Simulation loop
    while (1) {
        cycle++;
        // Write-Back stage (WB) - write result to register file
        if (MEMWB.valid && MEMWB.regWrite) {
            reg_write(MEMWB.destReg, MEMWB.write_val);
        }
        // Check termination: if no new fetch and pipeline is empty, break
        if (!fetch_enable && !IFID.valid && !IDEX.valid && !EXMEM.valid && !MEMWB.valid) {
            break;
        }
        // Memory stage (MEM) - access data memory for load or store
        // Prepare new MEM/WB pipeline register
        MEMWB_t MEMWB_new = {0};
        MEMWB_new.instr = EXMEM.instr;
        MEMWB_new.valid = EXMEM.valid;
        MEMWB_new.destReg = EXMEM.destReg;
        MEMWB_new.regWrite = EXMEM.regWrite;
        if (EXMEM.valid) {
            if (EXMEM.memRead) {
                // Load from data memory
                MEMWB_new.write_val = mem_read_word((uint32_t)EXMEM.alu_result);
            } else {
                MEMWB_new.write_val = EXMEM.alu_result;
            }
            if (EXMEM.memWrite) {
                // Store to data memory
                mem_write_word((uint32_t)EXMEM.alu_result, EXMEM.store_val);
            }
        }
        // Execute stage (EX) - perform ALU operations, branch decisions
        // Prepare new EX/MEM pipeline register
        EXMEM_t EXMEM_new = {0};
        EXMEM_new.instr = IDEX.instr;
        EXMEM_new.pc = IDEX.pc;
        EXMEM_new.valid = IDEX.valid;
        EXMEM_new.destReg = IDEX.destReg;
        EXMEM_new.regWrite = IDEX.regWrite;
        EXMEM_new.memRead = IDEX.memRead;
        EXMEM_new.memWrite = IDEX.memWrite;
        int branch_taken = 0;
        uint32_t branch_target = 0;
        if (IDEX.valid) {
            if (IDEX.jump) {
                // Unconditional jump (J or JR)
                branch_taken = 1;
                if (IDEX.jump == 1) {
                    // J: target = (upper PC bits | imm<<2)
                    branch_target = (IDEX.pc & 0xF0000000) | ((uint32_t)IDEX.imm << 2);
                } else if (IDEX.jump == 2) {
                    // JR: target = value in rs (rs_val holds it)
                    branch_target = (uint32_t)IDEX.rs_val;
                }
                // Jump does not produce a result in ALU
            } else if (IDEX.branch) {
                // Branch instruction
                // Compute branch target: PC of this instr + 4 + (imm << 2)
                branch_target = IDEX.pc + 4 + ((uint32_t)IDEX.imm << 2);
                // Compare registers for branch condition
                int equal = (IDEX.rs_val == IDEX.rt_val);
                if ((IDEX.branch == 1 && equal) || (IDEX.branch == 2 && !equal)) {
                    branch_taken = 1;
                }
                // No register result to write for branch
            }
            // ALU operation (only if not a jump)
            if (!IDEX.jump) {
                int32_t opA = IDEX.rs_val;
                int32_t opB;
                if (IDEX.memRead || IDEX.memWrite || (IDEX.instr != 0 && OPCODE(IDEX.instr) == 0x08) ) {
                    // I-type with immediate (ADDI, LW, SW) uses imm as second operand
                    opB = IDEX.imm;
                } else if (IDEX.ALUop == ALU_SLL || IDEX.ALUop == ALU_SRL) {
                    // Shift instructions use imm field as shift amount
                    opB = (int32_t)(IDEX.imm & 0x1F);
                    // opA already set to value to shift (rs_val holds the value to shift for SLL which we put from rt)
                } else {
                    // R-type or branch uses rt_val as second operand
                    opB = IDEX.rt_val;
                }
                EXMEM_new.alu_result = alu_execute(IDEX.ALUop, opA, opB);
            }
            // For store, pass the value to write
            if (IDEX.memWrite) {
                EXMEM_new.store_val = IDEX.rt_val;
            }
        }
        // If branch or jump taken, will flush the following fetched/decode stages
        if (branch_taken) {
            // Override PC to branch target
            PC = branch_target;
            // Flush IFID and IDEX
            IFID = (IFID_t){0};
            IDEX = (IDEX_t){0};
        }
        // Instruction Decode stage (ID) - decode IF/ID and read registers
        // Prepare new ID/EX pipeline register
        IDEX_t IDEX_new = {0};
        if (IFID.valid) {
            uint32_t instr = IFID.instr;
            uint8_t op = OPCODE(instr);
            IDEX_new.instr = instr;
            IDEX_new.pc = IFID.pc;
            IDEX_new.valid = 1;
            // Decode fields
            IDEX_new.rs = RS(instr);
            IDEX_new.rt = RT(instr);
            IDEX_new.rd = RD(instr);
            uint8_t funct = FUNCT(instr);
            // Default control signals
            IDEX_new.regWrite = 0;
            IDEX_new.memRead = 0;
            IDEX_new.memWrite = 0;
            IDEX_new.branch = 0;
            IDEX_new.jump = 0;
            IDEX_new.destReg = 0;
            IDEX_new.ALUop = ALU_NOP;
            // Read register values
            int32_t rs_val = reg_read(IDEX_new.rs);
            int32_t rt_val = reg_read(IDEX_new.rt);
            // Decode based on opcode
            if (op == 0x00) {
                // R-type instructions
                IDEX_new.regWrite = 1;
                IDEX_new.destReg = IDEX_new.rd;
                switch (funct) {
                    case 0x20: case 0x21: // ADD/ADDU
                        IDEX_new.ALUop = ALU_ADD;
                        break;
                    case 0x22: case 0x23: // SUB/SUBU
                        IDEX_new.ALUop = ALU_SUB;
                        break;
                    case 0x24: // AND
                        IDEX_new.ALUop = ALU_AND;
                        break;
                    case 0x25: // OR
                        IDEX_new.ALUop = ALU_OR;
                        break;
                    case 0x26: // XOR
                        IDEX_new.ALUop = ALU_XOR;
                        break;
                    case 0x27: // NOR
                        IDEX_new.ALUop = ALU_NOR;
                        break;
                    case 0x2A: // SLT
                        IDEX_new.ALUop = ALU_SLT;
                        break;
                    case 0x00: // SLL
                        IDEX_new.ALUop = ALU_SLL;
                        // For SLL, rs_val should actually hold the value to shift (which is in rt register)
                        rs_val = rt_val;
                        IDEX_new.destReg = IDEX_new.rd;
                        // Use shamt as imm
                        IDEX_new.imm = SHAMT(instr);
                        break;
                    case 0x02: // SRL
                        IDEX_new.ALUop = ALU_SRL;
                        // rs_val gets value from rt
                        rs_val = rt_val;
                        IDEX_new.destReg = IDEX_new.rd;
                        IDEX_new.imm = SHAMT(instr);
                        break;
                    case 0x08: // JR
                        IDEX_new.regWrite = 0;
                        IDEX_new.jump = 2;
                        break;
                    default:
                        // Unsupported funct
                        break;
                }
            } else {
                // I-type or J-type
                IDEX_new.rs_val = rs_val;
                IDEX_new.rt_val = rt_val;
                int32_t imm_se = IMM_SE(instr);
                IDEX_new.imm = imm_se;
                switch (op) {
                    case 0x08: // ADDI
                        IDEX_new.regWrite = 1;
                        IDEX_new.destReg = IDEX_new.rt;
                        IDEX_new.ALUop = ALU_ADD;
                        break;
                    case 0x23: // LW
                        IDEX_new.regWrite = 1;
                        IDEX_new.memRead = 1;
                        IDEX_new.destReg = IDEX_new.rt;
                        IDEX_new.ALUop = ALU_ADD;
                        break;
                    case 0x2B: // SW
                        IDEX_new.memWrite = 1;
                        IDEX_new.destReg = 0;
                        IDEX_new.ALUop = ALU_ADD;
                        break;
                    case 0x04: // BEQ
                        IDEX_new.branch = 1;
                        IDEX_new.regWrite = 0;
                        // ALUop can be SUB for comparison
                        IDEX_new.ALUop = ALU_SUB;
                        break;
                    case 0x05: // BNE
                        IDEX_new.branch = 2;
                        IDEX_new.regWrite = 0;
                        IDEX_new.ALUop = ALU_SUB;
                        break;
                    case 0x02: // J
                        IDEX_new.jump = 1;
                        IDEX_new.regWrite = 0;
                        // Set imm to the jump address immediate (26 bits)
                        IDEX_new.imm = ADDR26(instr);
                        break;
                    default:
                        // Unsupported opcode
                        break;
                }
            }
            // Set read values after any adjustments (for SLL/SRL)
            IDEX_new.rs_val = rs_val;
            IDEX_new.rt_val = rt_val;
        }
        // Instruction Fetch stage (IF) - fetch next instruction from instruction memory
        // Prepare new IF/ID pipeline register
        IFID_t IFID_new = {0};
        IFID_new.valid = 0;
        IFID_new.instr = 0;
        IFID_new.pc = PC;
        if (fetch_enable) {
            if (PC / 4 < (uint32_t)inst_count) {
                IFID_new.instr = instr_read(PC / 4);
                IFID_new.pc = PC;
                IFID_new.valid = 1;
            } else {
                // No more instructions to fetch
                fetch_enable = 0;
            }
        }
        // Hazard detection for data hazards (no forwarding)
        int stall = 0;
        if (IFID.valid) {
            stall = hazard_detect_data(IDEX.regWrite, EXMEM.regWrite,
                                       IDEX.destReg, EXMEM.destReg,
                                       RS(IFID.instr), RT(IFID.instr),
                                       IDEX.memRead);
        }
        // Update pipeline registers with consideration for stall or flush
        if (branch_taken) {
            // Flush the fetched and decoded instructions
            IFID.valid = 0;
            IDEX.valid = 0;
            // Update IFID and IDEX as flushed
            IFID = (IFID_t){0};
            IDEX = (IDEX_t){0};
        } else if (stall) {
            // Stall: keep IFID the same (don't advance), insert bubble in IDEX
            IDEX = (IDEX_t){0}; // bubble in EX stage
            // Do not update IFID (remain the same instruction for next cycle)
            // Cancel the fetched instruction (as if we didn't fetch this cycle)
            IFID_new.valid = 0;
            // Also adjust PC to not advance (we reset PC below)
            PC = IFID.pc;
        } else {
            // Normal flow: transfer IFID_new to IFID, and IDEX_new to IDEX
            IDEX = IDEX_new;
            IFID = IFID_new;
        }
        // If no stall and no flush happened, then PC should be advanced
        if (!branch_taken && !stall) {
            PC += 4;
        }
        // Update EXMEM and MEMWB to the new values (older pipeline stages progress)
        EXMEM = EXMEM_new;
        MEMWB = MEMWB_new;
        // Count instruction in WB stage if it was a real instruction (exclude bubbles)
        if (MEMWB.valid && MEMWB.instr != 0) {
            instructions_executed++;
        }
    }
    // Simulation finished, output results
    printf("Simulation completed in %d cycles.\n", cycle);
    printf("Total instructions executed (completed): %d\n", instructions_executed);
    // Display squares results from memory (base address 0x0100)
    printf("Square table 0^2 to 200^2:\n");
    for (int n = 0; n <= 200; ++n) {
        int32_t result = mem_read_word(0x0100 + n * 4);
        printf("%3d^2 = %d\n", n, result);
    }
    return 0;
}
