#ifndef __CONTROL_TIMER_H
#define __CONTROL_TIMER_H

#include <stdint.h>

#define CONTROL_INNER_HZ       1000u
#define CONTROL_OUTER_HZ       500u
#define CONTROL_OUTER_DIVIDER  (CONTROL_INNER_HZ / CONTROL_OUTER_HZ)
#define CONTROL_PERIOD_US      (1000000u / CONTROL_INNER_HZ)
#define CONTROL_TIMER_IRQ_PRIO 6u

typedef struct
{
    uint32_t irq_count;
    uint32_t wake_count;
    uint32_t coalesced_ticks; /* Additional ticks while a wake is still pending. */
    uint32_t post_errors;
    uint32_t last_interval_us;
    uint32_t max_interval_us;
    uint32_t last_work_us;    /* Previous wake -> next Wait() entry. */
    uint32_t max_work_us;
    uint32_t overrun_count;   /* Previous work took longer than 1 ms. */
    uint8_t running;
} ControlTimer_DebugData;

extern volatile ControlTimer_DebugData ControlTimer_Debug;

/* Call once from a task, after OSInit(). Owns TIM2; no GPIO is needed.
 * Creates a private semaphore and configures TIM2, but leaves it stopped. */
uint8_t ControlTimer_Init(void);
/* Call after sensor/PID initialization. Start resets timing statistics.
 * Start/Stop/Wait belong to the same control task. Stop before reinitializing. */
void ControlTimer_Start(void);
void ControlTimer_Stop(void);
/* Blocks until a timer release; 1=success, 0=not started/OS error.
 * Call exactly once per control iteration, at the top of its loop.
 * At most one unconsumed release is kept; stale ticks are not replayed.
 * This does not read sensors or run any control/attitude calculations. */
uint8_t ControlTimer_Wait(void);

#endif
