/**
 * hazard.h - Hazard detection module interface.
 */
#ifndef HAZARD_H
#define HAZARD_H

#include <stdint.h>

// Check for data hazard (without forwarding) between pipeline stages.
// Returns 1 if a stall is needed, 0 otherwise.
int hazard_detect_data(uint8_t id_ex_regWrite, uint8_t ex_mem_regWrite,
                       uint8_t id_ex_dest, uint8_t ex_mem_dest,
                       uint8_t if_id_rs, uint8_t if_id_rt,
                       uint8_t id_ex_memRead);

#endif // HAZARD_H
