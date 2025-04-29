/**
 * alu.c - Arithmetic Logic Unit implementation.
 */
#include "config.h"
#include "alu.h"

int32_t alu_execute(int op_code, int32_t operand1, int32_t operand2) {
    switch(op_code) {
        case ALU_ADD:
            return operand1 + operand2;
        case ALU_SUB:
            return operand1 - operand2;
        case ALU_AND:
            return operand1 & operand2;
        case ALU_OR:
            return operand1 | operand2;
        case ALU_XOR:
            return operand1 ^ operand2;
        case ALU_NOR:
            return ~(operand1 | operand2);
        case ALU_SLT:
            return (operand1 < operand2) ? 1 : 0;
        case ALU_SLL:
            // operand2 is the shift amount (0-31)
            return (int32_t)((uint32_t)operand1 << (operand2 & 0x1F));
        case ALU_SRL:
            // logical right shift
            return (int32_t)((uint32_t)operand1 >> (operand2 & 0x1F));
        default:
            // NOP or unsupported operation
            return operand1;
    }
}
