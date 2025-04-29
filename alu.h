/**
 * alu.h - Arithmetic Logic Unit interface.
 */
#ifndef ALU_H
#define ALU_H

#include <stdint.h>

// Execute an ALU operation. 
// op_code is one of the ALUOps defined in config.h.
// operand1 and operand2 are the input values.
// Returns the result of the operation.
int32_t alu_execute(int op_code, int32_t operand1, int32_t operand2);

#endif // ALU_H
