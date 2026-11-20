#include "delay.h"

/* DWT leaves SysTick exclusively owned by uC/OS-II. */
void Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void Delay_us(uint32_t us)
{
    uint32_t start, cycles;
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0u) Delay_Init();
    while (us != 0u)
    {
        uint32_t chunk = us > 1000u ? 1000u : us;
        cycles = (SystemCoreClock / 1000000u) * chunk;
        start = DWT->CYCCNT;
        while ((uint32_t)(DWT->CYCCNT - start) < cycles) { __NOP(); }
        us -= chunk;
    }
}

/* During task operation use OSTimeDly for long waits. */
void Delay_ms(uint32_t ms)
{
    while (ms-- != 0u) Delay_us(1000u);
}
