/* =============================================================================
 * SENG21213-OS :: Programmable Interval Timer (PIT / 8253) Header
 * File   : kernel/pit.h
 * ============================================================================*/
#ifndef PIT_H
#define PIT_H

#include "../include/types.h"

#define PIT_FREQUENCY_HZ 100

void     pit_init(uint32_t frequency);
uint32_t pit_get_ticks(void);
void     pit_sleep(uint32_t ticks);

#endif /* PIT_H */
