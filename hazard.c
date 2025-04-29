/**
 * hazard.c - Hazard detection logic (stall-based resolution).
 */
#include "hazard.h"

int hazard_detect_data(uint8_t id_ex_regWrite, uint8_t ex_mem_regWrite,
                       uint8_t id_ex_dest, uint8_t ex_mem_dest,
                       uint8_t if_id_rs, uint8_t if_id_rt,
                       uint8_t id_ex_memRead) {
    // Data hazard conditions (no forwarding):
    // If the instruction in EX stage writes a register that the instruction in ID stage needs
    if (id_ex_regWrite && id_ex_dest != 0) {
        if (id_ex_dest == if_id_rs || id_ex_dest == if_id_rt) {
            return 1; // hazard, stall
        }
    }
    // If the instruction in MEM stage writes a register that the ID stage instruction needs
    if (ex_mem_regWrite && ex_mem_dest != 0) {
        if (ex_mem_dest == if_id_rs || ex_mem_dest == if_id_rt) {
            return 1; // hazard, stall
        }
    }
    return 0;
}
