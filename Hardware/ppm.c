#include "ppm.h"
#include "misc.h"

#define PPM_TIMEOUT_TICKS \
    ((OS_TICKS_PER_SEC * PPM_SIGNAL_TIMEOUT_MS + 999u) / 1000u)
#define PPM_MAX_HIGH_TICKS \
    ((PPM_PULSE_MAX_US * OS_TICKS_PER_SEC + 999999u) / 1000000u + 1u)

volatile uint16_t ppm_channels[8] = {0};
volatile uint8_t ppm_frame_ready = 0u;
volatile uint8_t frame_channel_count = 0u;
volatile uint8_t ch_idx = 0u;
uint8_t throttle_channel = 3u;
volatile PPM_Diagnostics PPM_Debug;

static uint16_t RiseCapture[PPM_CHANNEL_COUNT];
static uint32_t RiseTick[PPM_CHANNEL_COUNT];
static volatile uint32_t LastPulseTick[PPM_CHANNEL_COUNT];
static volatile uint8_t WaitingFall[PPM_CHANNEL_COUNT];
static volatile uint8_t UpdatedMask;

static uint32_t PPM_TimerClockHz(void)
{
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);
    if ((RCC->DCKCFGR & RCC_DCKCFGR_TIMPRE) != 0u)
        return clocks.HCLK_Frequency / clocks.PCLK2_Frequency <= 4u
             ? clocks.HCLK_Frequency : 4u * clocks.PCLK2_Frequency;
    return clocks.PCLK2_Frequency == clocks.HCLK_Frequency
         ? clocks.PCLK2_Frequency : 2u * clocks.PCLK2_Frequency;
}

static void PPM_SetFalling(uint8_t index, uint8_t falling)
{
    uint32_t polarity = (uint32_t)TIM_CCER_CC1P << (index * 4u);
    if (falling) TIM1->CCER |= polarity;
    else TIM1->CCER &= ~polarity;
}

static void PPM_Invalidate(uint8_t index)
{
    uint8_t i;
    uint8_t bit = (uint8_t)(1u << index);
    PPM_Debug.valid_mask &= (uint8_t)~bit;
    ppm_channels[index] = 0u;
    UpdatedMask &= (uint8_t)~bit;
    ppm_frame_ready = 0u;
    frame_channel_count = 0u;
    ch_idx = 0u;
    for (i = 0u; i < PPM_CHANNEL_COUNT; i++)
        if ((UpdatedMask & (1u << i)) != 0u) ch_idx++;
}

void TIM1_PPM_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_TimeBaseInitTypeDef timer = {0};
    TIM_ICInitTypeDef capture = {0};
    NVIC_InitTypeDef nvic = {0};
    PPM_Diagnostics empty = {0};
    uint8_t i;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    NVIC_DisableIRQ(TIM1_CC_IRQn);
    TIM_DeInit(TIM1);

    PPM_Debug = empty;
    UpdatedMask = ppm_frame_ready = frame_channel_count = ch_idx = 0u;
    for (i = 0u; i < 8u; i++) ppm_channels[i] = 0u;
    for (i = 0u; i < PPM_CHANNEL_COUNT; i++)
    {
        RiseCapture[i] = 0u;
        RiseTick[i] = LastPulseTick[i] = 0u;
        WaitingFall[i] = 0u;
    }

    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_DOWN;
    gpio.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOA, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_TIM1);

    timer.TIM_Period = 0xFFFFu;
    timer.TIM_Prescaler = (uint16_t)(PPM_TimerClockHz() / 1000000u - 1u);
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &timer);
    capture.TIM_ICPolarity = TIM_ICPolarity_Rising;
    capture.TIM_ICSelection = TIM_ICSelection_DirectTI;
    capture.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    capture.TIM_ICFilter = 0x03u;
    capture.TIM_Channel = TIM_Channel_1;
    TIM_ICInit(TIM1, &capture);
    capture.TIM_Channel = TIM_Channel_2;
    TIM_ICInit(TIM1, &capture);
    capture.TIM_Channel = TIM_Channel_3;
    TIM_ICInit(TIM1, &capture);
    capture.TIM_Channel = TIM_Channel_4;
    TIM_ICInit(TIM1, &capture);

    TIM_GenerateEvent(TIM1, TIM_EventSource_Update);
    TIM_SetCounter(TIM1, 0u);
    TIM_ClearFlag(TIM1, TIM_FLAG_Update | TIM_FLAG_CC1 | TIM_FLAG_CC2 |
                        TIM_FLAG_CC3 | TIM_FLAG_CC4 | TIM_FLAG_CC1OF |
                        TIM_FLAG_CC2OF | TIM_FLAG_CC3OF | TIM_FLAG_CC4OF);
    NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
    nvic.NVIC_IRQChannel = TIM1_CC_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 6u;
    nvic.NVIC_IRQChannelSubPriority = 0u;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    TIM_ITConfig(TIM1, TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

static void PPM_CaptureEdge(uint8_t index, uint16_t captured,
                            uint32_t ticks, uint8_t overcapture)
{
    uint16_t width;
    uint8_t bit = (uint8_t)(1u << index);
    uint8_t i;

    if (overcapture)
    {
        PPM_Debug.overcaptures[index]++;
        WaitingFall[index] = 0u;
        PPM_SetFalling(index, 0u);
        PPM_Invalidate(index);
        return;
    }
    if (!WaitingFall[index])
    {
        RiseCapture[index] = captured;
        RiseTick[index] = ticks;
        WaitingFall[index] = 1u;
        PPM_SetFalling(index, 1u);
        return;
    }

    WaitingFall[index] = 0u;
    PPM_SetFalling(index, 0u);
    /* Unsigned 16-bit subtraction includes the 65535 -> 0 timer wrap.
     * OS ticks reject long stuck-high pulses which could alias after a wrap. */
    width = (uint16_t)(captured - RiseCapture[index]);
    if (width < PPM_PULSE_MIN_US || width > PPM_PULSE_MAX_US ||
        (uint32_t)(ticks - RiseTick[index]) > PPM_MAX_HIGH_TICKS)
    {
        PPM_Debug.invalid_pulses[index]++;
        PPM_Invalidate(index);
        return;
    }

    ppm_channels[index] = width;
    LastPulseTick[index] = ticks;
    PPM_Debug.valid_pulses[index]++;
    PPM_Debug.valid_mask |= bit;
    UpdatedMask |= bit;
    ch_idx = 0u;
    for (i = 0u; i < PPM_CHANNEL_COUNT; i++)
        if ((UpdatedMask & (1u << i)) != 0u) ch_idx++;
    if (UpdatedMask == PPM_ALL_CHANNELS_MASK)
    {
        UpdatedMask = ch_idx = 0u;
        frame_channel_count = PPM_CHANNEL_COUNT;
        ppm_frame_ready = 1u;
    }
}

void TIM1_CC_IRQHandler(void)
{
    uint32_t ticks, status;
    uint16_t captured, flag, overflow;
    uint8_t i;

    OSIntEnter();
    ticks = OSTimeGet();
    for (i = 0u; i < PPM_CHANNEL_COUNT; i++)
    {
        flag = (uint16_t)(TIM_FLAG_CC1 << i);
        overflow = (uint16_t)(TIM_FLAG_CC1OF << i);
        status = TIM1->SR;
        if ((status & (flag | overflow)) == 0u) continue;

        /* Reading CCR clears CCxIF. Do not clear it again after processing:
         * that would erase a new edge arriving during the handler. */
        switch (i)
        {
            case 0u: captured = (uint16_t)TIM_GetCapture1(TIM1); break;
            case 1u: captured = (uint16_t)TIM_GetCapture2(TIM1); break;
            case 2u: captured = (uint16_t)TIM_GetCapture3(TIM1); break;
            default: captured = (uint16_t)TIM_GetCapture4(TIM1); break;
        }
        status |= TIM1->SR;
        if ((status & overflow) != 0u) TIM_ClearFlag(TIM1, overflow);
        PPM_CaptureEdge(i, captured, ticks, (status & overflow) != 0u);
    }
    OSIntExit();
}

uint8_t PPM_ReadChannels(uint16_t pulse_us[PPM_CHANNEL_COUNT])
{
    uint32_t ticks;
    uint8_t i, mask;
#if OS_CRITICAL_METHOD == 3u
    OS_CPU_SR cpu_sr;
#endif
    if (pulse_us == 0) return 0u;

    OS_ENTER_CRITICAL();
    ticks = OSTimeGet();
    for (i = 0u; i < PPM_CHANNEL_COUNT; i++)
    {
        if (WaitingFall[i] && (uint32_t)(ticks - RiseTick[i]) > PPM_MAX_HIGH_TICKS)
        {
            WaitingFall[i] = 0u;
            PPM_SetFalling(i, 0u);
            PPM_Invalidate(i);
        }
        if ((PPM_Debug.valid_mask & (1u << i)) != 0u &&
            (uint32_t)(ticks - LastPulseTick[i]) >= PPM_TIMEOUT_TICKS)
            PPM_Invalidate(i);
        pulse_us[i] = ppm_channels[i];
    }
    mask = PPM_Debug.valid_mask;
    ppm_frame_ready = 0u;
    OS_EXIT_CRITICAL();
    return mask;
}
