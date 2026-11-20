#ifndef __ppm_h
#define __ppm_h

#include "stm32f4xx.h"
#include "ucos_ii.h"

#define PPM_CHANNEL_COUNT       4u
#define PPM_ALL_CHANNELS_MASK   0x0Fu
#define PPM_PULSE_MIN_US        900u
#define PPM_PULSE_MAX_US        2100u
#define PPM_SIGNAL_TIMEOUT_MS   100u

/* Historical filename/API retained. This schematic uses four independent
 * receiver PWM signals, NOT one wire carrying a summed PPM frame.
 * TIM1_CH1..4 = PA8, PA9, PA10, PA11; values below are high-pulse widths (us).
 * Slots 4..7 are retained for source compatibility and remain zero. */
extern volatile uint16_t ppm_channels[8];
/* All four inputs have supplied a new valid pulse since the last group.
 * Independent inputs are not guaranteed to belong to one radio frame. */
extern volatile uint8_t ppm_frame_ready;
extern volatile uint8_t frame_channel_count;
extern volatile uint8_t ch_idx;  /* Number collected in the current group: 0..3. */
extern uint8_t throttle_channel; /* Logical input 1..4; default 3 (PA10). */

typedef struct
{
    uint32_t valid_pulses[PPM_CHANNEL_COUNT];
    uint32_t invalid_pulses[PPM_CHANNEL_COUNT];
    uint32_t overcaptures[PPM_CHANNEL_COUNT];
    uint8_t valid_mask; /* Freshness is refreshed by PPM_ReadChannels(). */
} PPM_Diagnostics;

extern volatile PPM_Diagnostics PPM_Debug;

/* Initialize once after NVIC_PriorityConfig(); uC/OS-II provides the timeout
 * timebase. Start SysTick before relying on freshness and timeout checks. */
void TIM1_PPM_Init(void);
void TIM1_CC_IRQHandler(void);
/* Call every 5..20 ms from a task, even when ppm_frame_ready == 0.
 * Atomically copies four inputs and consumes ppm_frame_ready. The returned
 * bit0..3 mask marks fresh/valid inputs; invalid output slots are zero.
 * Always use this API for control: the raw globals alone do not age out. */
uint8_t PPM_ReadChannels(uint16_t pulse_us[PPM_CHANNEL_COUNT]);

#endif
