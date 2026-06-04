#include "ppm.h"



volatile uint16_t ppm_channels[8] = {0};
volatile uint8_t ppm_frame_ready = 0;
volatile uint8_t frame_channel_count = 0;



static volatile uint32_t last_cap = 0;
volatile uint8_t  ch_idx   = 0;
	
void TIM1_PPM_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // F4系列使用不同的时钟使能函数
    // PA9使用TIM1_CH2，TIM1在APB2上
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    
    // 配置PA9为TIM1_CH2输入（PPM信号输入）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;  // 复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;  // 下拉输入
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置PA9复用为TIM1_CH2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);
    
    // 定时器基础配置
    // F401: APB2定时器时钟为84MHz
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 83;    // 84MHz/84 = 1MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;  // 高级定时器特有
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
    
    // 输入捕获配置
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;
    TIM_ICInit(TIM1, &TIM_ICInitStructure);
    
    // 使能捕获中断
    TIM_ITConfig(TIM1, TIM_IT_CC2, ENABLE);
    
    // 配置NVIC - 注意中断向量改为TIM1_CC_IRQn
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 启动定时器
    TIM_Cmd(TIM1, ENABLE);
}

void TIM1_CC_IRQHandler(void)
{
    // 通知μC/OS-II进入中断服务程序
    OSIntEnter();
    
    if (TIM_GetITStatus(TIM1, TIM_IT_CC2) != RESET)
    {
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC2);

        uint32_t cap = TIM_GetCapture2(TIM1);
        uint32_t diff = (cap >= last_cap) ? (cap - last_cap)
                                          : (0xFFFF - last_cap + cap);
        last_cap = cap;

        if (diff > 2500)          // 同步间隔，新帧开始
        {
            ch_idx = 0;
            ppm_frame_ready = 1;  // 通知任务有完整帧就绪
        }
        else if (ch_idx < 8)
        {
            ppm_channels[ch_idx++] = diff;
            // 注意：此处不应清零 ppm_frame_ready
            // ppm_frame_ready 应由任务读取后清零，或在同步间隔时设置
        }
    }
    
    OSIntExit();
}
