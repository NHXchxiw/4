# 四路遥控输入与电调输出

按本次电路图分配如下，软件中的通道号均为 1～4：

| 通道 | 接收器 PWM 输入 | 电调 PWM 输出 |
| --- | --- | --- |
| 1 | PA8 / TIM1_CH1 | PC6 / TIM3_CH1 |
| 2 | PA9 / TIM1_CH2 | PC7 / TIM3_CH2 |
| 3 | PA10 / TIM1_CH3 | PC8 / TIM3_CH3 |
| 4 | PA11 / TIM1_CH4 | PC9 / TIM3_CH4 |

接收器的四路 PWM 分别表示遥控通道，不代表与四个电机逐一对应。控制飞行需要将油门、姿态指令经过 PID 和机架混控，再生成四路电调脉宽。

## 接收器输入

`ppm.c/.h` 保留原文件名与 `TIM1_PPM_Init()`，实现改为图中的**四根独立 PWM 信号线**，不再按单根 PA9 的复合 PPM 帧解析。若实际接收器输出的是单线 PPM 或 SBUS，需要相应的另一种解码器。

每路分别捕获上升沿、下降沿，定时器计数单位为 1 μs，接受 900～2100 μs 高电平。四路可以同时产生脉冲；计数器跨越 65535→0、脉冲越界和捕获溢出均有处理。

```c
TIM1_PPM_Init(); /* 初始化一次；先配置 NVIC 优先级分组。 */

/* uC/OS-II 任务中每 5～20 ms 调用，不要仅在 frame_ready 时调用。 */
uint16_t channels[PPM_CHANNEL_COUNT];
uint8_t mask = PPM_ReadChannels(channels);
if (mask == PPM_ALL_CHANNELS_MASK)
{
    /* channels[0..3] 分别对应 PA8..PA11，单位 μs。 */
}
else
{
    /* 有输入无效或超过 100 ms 没有新有效脉冲，不能沿用旧油门。 */
}
```

`PPM_ReadChannels()` 在临界区中复制通道值并检查超时，返回 bit0～bit3 有效掩码；无效通道返回 0，同时清除 `ppm_frame_ready`。超时使用 uC/OS-II tick，必须启动 OS SysTick。此接口只报告输入有效性，电调的失联处理由调用方执行。

Watch 可看 `ppm_channels[0..3]` 和 `PPM_Debug`。兼容保留的 `ppm_channels[4..7]` 始终为 0；`PPM_Debug.valid_pulses[]`、`invalid_pulses[]`、`overcaptures[]` 分别是有效脉冲、越界/过长脉冲、捕获溢出计数。`valid_mask` 的超时状态由任务调用读取接口刷新，单看原始数组不能判断信号是否仍然有效。

`ppm_frame_ready=1` 表示四路自上次分组后各收到至少一个新有效脉冲，`frame_channel_count=4`。这是软件分组通知，不能保证四路属于接收器同一无线帧。`throttle_channel=3` 默认选择 PA10，实际通道顺序以遥控器设置为准。

若接收器在无线失联后仍输出固定 PWM，脉冲超时无法识别这种情况，需配置接收器自身的失控保护。

## 电调输出

`PWM_Init()` 一次初始化 TIM3 四个通道，输出 50 Hz / 20 ms 周期，初始脉宽均为 1000 μs。保留原有 900～2100 μs 限幅，常规油门测试使用 1000～2000 μs。初始化不执行电调高低油门行程校准。

```c
PWM_Init();
ESC_StopAll();                       /* 四路均设为 1000 μs。 */
ESC_SetThrottleChannel(2u, 1100u);    /* 单独设置 PC7。 */
ESC_SetThrottleAll(1100u);            /* 四路设为相同油门。 */
ESC_SetThrottles(1100u, 1200u, 1300u, 1400u); /* PC6、PC7、PC8、PC9。 */
```

旧接口 `ESC_SetThrottle(pulse_us)` 仍仅控制 PC6。新接口 `ESC_SetThrottles()` 通过四路 CCR 预装载和更新事件屏蔽，使整组脉宽在同一周期边界生效；不会在写完部分通道时加载半组数据。各设置函数在任务中调用，`ESC_StopAll()` 在下一个周期边界生效，不代表物理切断电调电源。

## 接入现有任务

`User/BSP.c` 中的 `TIM1_PPM_Init()`、`PWM_Init()` 和 `User/app.c` 中的两个任务创建语句保持注释。要启用台架油门测试：

1. 在 `BSP_Init()` 中启用这两个初始化调用，保持 `NVIC_PriorityConfig()` 在前。
2. 恢复 `TaskPPM`、`TaskMotor` 的函数体与创建语句。现有注释示例已改用 `PPM_ReadChannels()` 和 `ESC_SetThrottleAll()`；任一路无效时，将四路油门恢复为 1000 μs。
3. 该示例只是四电机相同油门的台架测试，尚无解锁逻辑、姿态 PID 或混控。电机测试先拆桨。

只检查接收器时，可以仅启用 `TIM1_PPM_Init()` 和输入读取任务。

保持接收器、电调与主控共地。图中电调排针的 VIN 是电源网络，不是 MCU PWM 信号；需按实际电调供电要求连接。接收器电源 +5V 也不表示 MCU 信号线应接电源。

这些引脚与当前 PB6/PB7 有线串口、PB8/PB9 GY86 I²C 不冲突。不要启用工程外的 `System/usart1.c`（占用 PA9/PA10）或 `System/usart6.c`（占用 PC6/PC7），也不要同时把 PA11 用作 USB。

PWM、PPM 源文件原本已在 Keil 工程中，无需增加工程条目。按用户要求，未运行 Keil 构建或下载。
