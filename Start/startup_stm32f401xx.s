;******************** (C) COPYRIGHT 2016 STMicroelectronics ********************
;* 文件名            : startup_stm32f401xx.s
;* 作者              : MCD Application Team
;* @版本             : V1.8.0
;* @日期             : 2016年11月9日
;* 描述              : STM32F401xx系列设备的MDK-ARM工具链向量表
;*                    本模块执行以下操作：
;*                    - 设置初始栈指针SP
;*                    - 设置初始程序计数器PC == Reset_Handler
;*                    - 用异常中断服务程序地址填充向量表项
;*                    - 配置系统时钟
;*                    - 跳转到C库中的__main（最终调用main()函数）
;*                    复位后，Cortex-M4处理器处于线程模式，
;*                    优先级为特权级，栈设置为主栈。
;* <<< 在上下文菜单中使用配置向导 >>>   
;*******************************************************************************
; 
; 根据MCD-ST Liberty软件许可协议V2（"许可"）授权；
; 除非遵守许可，否则不得使用此文件。
; 您可以在以下网址获取许可副本：
; 
;        http://www.st.com/software_license_agreement_liberty_v2
; 
; 除非适用法律要求或书面同意，否则按"原样"分发软件，
; 无任何明示或暗示的保证或条件。
; 请参阅许可中管理权限和限制的具体语言。
; 
;*******************************************************************************

; 为栈分配的内存大小（字节）
; 根据您的应用需求调整此值
; <h> 栈配置
;   <o> 栈大小（字节） <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x00000400

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


; <h> 堆配置
;   <o>  堆大小（字节） <0x0-0xFFFFFFFF:8>
; </h>

Heap_Size       EQU     0x00000200

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB


; 复位时映射到地址0的向量表
                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp               ; 栈顶
                DCD     Reset_Handler              ; 复位处理程序
                DCD     NMI_Handler                ; 不可屏蔽中断处理程序
                DCD     HardFault_Handler          ; 硬件故障处理程序
                DCD     MemManage_Handler          ; 内存管理单元故障处理程序
                DCD     BusFault_Handler           ; 总线故障处理程序
                DCD     UsageFault_Handler         ; 用法错误处理程序
                DCD     0                          ; 保留
                DCD     0                          ; 保留
                DCD     0                          ; 保留
                DCD     0                          ; 保留
                DCD     SVC_Handler                ; 系统服务调用处理程序
                DCD     DebugMon_Handler           ; 调试监视器处理程序
                DCD     0                          ; 保留
                DCD     OS_CPU_PendSVHandler             ; PendSV处理程序
                DCD     OS_CPU_SysTickHandler            ; SysTick处理程序

                ; 外部中断
                DCD     WWDG_IRQHandler                   ; 窗口看门狗
                DCD     PVD_IRQHandler                    ; 通过EXTI线的可编程电压检测器
                DCD     TAMP_STAMP_IRQHandler             ; 通过EXTI线的篡改和时间戳
                DCD     RTC_WKUP_IRQHandler               ; 通过EXTI线的RTC唤醒
                DCD     FLASH_IRQHandler                  ; FLASH
                DCD     RCC_IRQHandler                    ; RCC
                DCD     EXTI0_IRQHandler                  ; EXTI线0
                DCD     EXTI1_IRQHandler                  ; EXTI线1
                DCD     EXTI2_IRQHandler                  ; EXTI线2
                DCD     EXTI3_IRQHandler                  ; EXTI线3
                DCD     EXTI4_IRQHandler                  ; EXTI线4
                DCD     DMA1_Stream0_IRQHandler           ; DMA1流0
                DCD     DMA1_Stream1_IRQHandler           ; DMA1流1
                DCD     DMA1_Stream2_IRQHandler           ; DMA1流2
                DCD     DMA1_Stream3_IRQHandler           ; DMA1流3
                DCD     DMA1_Stream4_IRQHandler           ; DMA1流4
                DCD     DMA1_Stream5_IRQHandler           ; DMA1流5
                DCD     DMA1_Stream6_IRQHandler           ; DMA1流6
                DCD     ADC_IRQHandler                    ; ADC1
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     EXTI9_5_IRQHandler                ; 外部中断线[9:5]
                DCD     TIM1_BRK_TIM9_IRQHandler          ; TIM1中断和TIM9
                DCD     TIM1_UP_TIM10_IRQHandler          ; TIM1更新和TIM10
                DCD     TIM1_TRG_COM_TIM11_IRQHandler     ; TIM1触发、换向和TIM11
                DCD     TIM1_CC_IRQHandler                ; TIM1捕获比较
                DCD     TIM2_IRQHandler                   ; TIM2
                DCD     TIM3_IRQHandler                   ; TIM3
                DCD     TIM4_IRQHandler                   ; TIM4
                DCD     I2C1_EV_IRQHandler                ; I2C1事件
                DCD     I2C1_ER_IRQHandler                ; I2C1错误
                DCD     I2C2_EV_IRQHandler                ; I2C2事件
                DCD     I2C2_ER_IRQHandler                ; I2C2错误
                DCD     SPI1_IRQHandler                   ; SPI1
                DCD     SPI2_IRQHandler                   ; SPI2
                DCD     USART1_IRQHandler                 ; USART1
                DCD     USART2_IRQHandler                 ; USART2
                DCD     0                                 ; 保留
                DCD     EXTI15_10_IRQHandler              ; 外部中断线[15:10]
                DCD     RTC_Alarm_IRQHandler              ; 通过EXTI线的RTC闹钟（A和B）
                DCD     OTG_FS_WKUP_IRQHandler            ; 通过EXTI线的USB OTG FS唤醒
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     DMA1_Stream7_IRQHandler           ; DMA1流7
                DCD     0                                 ; 保留
                DCD     SDIO_IRQHandler                   ; SDIO
                DCD     TIM5_IRQHandler                   ; TIM5
                DCD     SPI3_IRQHandler                   ; SPI3
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     DMA2_Stream0_IRQHandler           ; DMA2流0
                DCD     DMA2_Stream1_IRQHandler           ; DMA2流1
                DCD     DMA2_Stream2_IRQHandler           ; DMA2流2
                DCD     DMA2_Stream3_IRQHandler           ; DMA2流3
                DCD     DMA2_Stream4_IRQHandler           ; DMA2流4
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     OTG_FS_IRQHandler                 ; USB OTG FS
                DCD     DMA2_Stream5_IRQHandler           ; DMA2流5
                DCD     DMA2_Stream6_IRQHandler           ; DMA2流6
                DCD     DMA2_Stream7_IRQHandler           ; DMA2流7
                DCD     USART6_IRQHandler                 ; USART6
                DCD     I2C3_EV_IRQHandler                ; I2C3事件
                DCD     I2C3_ER_IRQHandler                ; I2C3错误
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     FPU_IRQHandler                    ; 浮点运算单元
                DCD     0                                 ; 保留
                DCD     0                                 ; 保留
                DCD     SPI4_IRQHandler                   ; SPI4
                                         
__Vectors_End

__Vectors_Size  EQU  __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

; 复位处理程序
Reset_Handler    PROC
                 EXPORT  Reset_Handler             [WEAK]
        IMPORT  SystemInit
        IMPORT  __main

                 LDR     R0, =SystemInit
                 BLX     R0
                 LDR     R0, =__main
                 BX      R0
                 ENDP

; 虚拟异常处理程序（可修改的无限循环）

NMI_Handler     PROC
                EXPORT  NMI_Handler                [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler          [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler          [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler           [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler         [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler                [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler           [WEAK]
                B       .
                ENDP
OS_CPU_PendSVHandler  PROC
                EXPORT  OS_CPU_PendSVHandler             [WEAK]
                B       .
                ENDP
OS_CPU_SysTickHandler PROC
                EXPORT  OS_CPU_SysTickHandler            [WEAK]
                B       .
                ENDP

Default_Handler PROC

                EXPORT  WWDG_IRQHandler                   [WEAK]
                EXPORT  PVD_IRQHandler                    [WEAK]
                EXPORT  TAMP_STAMP_IRQHandler             [WEAK]
                EXPORT  RTC_WKUP_IRQHandler               [WEAK]
                EXPORT  FLASH_IRQHandler                  [WEAK]
                EXPORT  RCC_IRQHandler                    [WEAK]
                EXPORT  EXTI0_IRQHandler                  [WEAK]
                EXPORT  EXTI1_IRQHandler                  [WEAK]
                EXPORT  EXTI2_IRQHandler                  [WEAK]
                EXPORT  EXTI3_IRQHandler                  [WEAK]
                EXPORT  EXTI4_IRQHandler                  [WEAK]
                EXPORT  DMA1_Stream0_IRQHandler           [WEAK]
                EXPORT  DMA1_Stream1_IRQHandler           [WEAK]
                EXPORT  DMA1_Stream2_IRQHandler           [WEAK]
                EXPORT  DMA1_Stream3_IRQHandler           [WEAK]
                EXPORT  DMA1_Stream4_IRQHandler           [WEAK]
                EXPORT  DMA1_Stream5_IRQHandler           [WEAK]
                EXPORT  DMA1_Stream6_IRQHandler           [WEAK]
                EXPORT  ADC_IRQHandler                    [WEAK]
                EXPORT  EXTI9_5_IRQHandler                [WEAK]
                EXPORT  TIM1_BRK_TIM9_IRQHandler          [WEAK]
                EXPORT  TIM1_UP_TIM10_IRQHandler          [WEAK]
                EXPORT  TIM1_TRG_COM_TIM11_IRQHandler     [WEAK]
                EXPORT  TIM1_CC_IRQHandler                [WEAK]
                EXPORT  TIM2_IRQHandler                   [WEAK]
                EXPORT  TIM3_IRQHandler                   [WEAK]
                EXPORT  TIM4_IRQHandler                   [WEAK]
                EXPORT  I2C1_EV_IRQHandler                [WEAK]
                EXPORT  I2C1_ER_IRQHandler                [WEAK]
                EXPORT  I2C2_EV_IRQHandler                [WEAK]
                EXPORT  I2C2_ER_IRQHandler                [WEAK]
                EXPORT  SPI1_IRQHandler                   [WEAK]
                EXPORT  SPI2_IRQHandler                   [WEAK]
                EXPORT  USART1_IRQHandler                 [WEAK]
                EXPORT  USART2_IRQHandler                 [WEAK]
                EXPORT  EXTI15_10_IRQHandler              [WEAK]
                EXPORT  RTC_Alarm_IRQHandler              [WEAK]
                EXPORT  OTG_FS_WKUP_IRQHandler            [WEAK]
                EXPORT  DMA1_Stream7_IRQHandler           [WEAK]
                EXPORT  SDIO_IRQHandler                   [WEAK]
                EXPORT  TIM5_IRQHandler                   [WEAK]
                EXPORT  SPI3_IRQHandler                   [WEAK]
                EXPORT  DMA2_Stream0_IRQHandler           [WEAK]
                EXPORT  DMA2_Stream1_IRQHandler           [WEAK]
                EXPORT  DMA2_Stream2_IRQHandler           [WEAK]
                EXPORT  DMA2_Stream3_IRQHandler           [WEAK]
                EXPORT  DMA2_Stream4_IRQHandler           [WEAK]
                EXPORT  OTG_FS_IRQHandler                 [WEAK]
                EXPORT  DMA2_Stream5_IRQHandler           [WEAK]
                EXPORT  DMA2_Stream6_IRQHandler           [WEAK]
                EXPORT  DMA2_Stream7_IRQHandler           [WEAK]
                EXPORT  USART6_IRQHandler                 [WEAK]
                EXPORT  I2C3_EV_IRQHandler                [WEAK]
                EXPORT  I2C3_ER_IRQHandler                [WEAK]
                EXPORT  FPU_IRQHandler                    [WEAK]
                EXPORT  SPI4_IRQHandler                   [WEAK]

WWDG_IRQHandler
PVD_IRQHandler
TAMP_STAMP_IRQHandler
RTC_WKUP_IRQHandler
FLASH_IRQHandler
RCC_IRQHandler
EXTI0_IRQHandler
EXTI1_IRQHandler
EXTI2_IRQHandler
EXTI3_IRQHandler
EXTI4_IRQHandler
DMA1_Stream0_IRQHandler
DMA1_Stream1_IRQHandler
DMA1_Stream2_IRQHandler
DMA1_Stream3_IRQHandler
DMA1_Stream4_IRQHandler
DMA1_Stream5_IRQHandler
DMA1_Stream6_IRQHandler
ADC_IRQHandler
EXTI9_5_IRQHandler
TIM1_BRK_TIM9_IRQHandler
TIM1_UP_TIM10_IRQHandler
TIM1_TRG_COM_TIM11_IRQHandler
TIM1_CC_IRQHandler
TIM2_IRQHandler
TIM3_IRQHandler
TIM4_IRQHandler
I2C1_EV_IRQHandler
I2C1_ER_IRQHandler
I2C2_EV_IRQHandler
I2C2_ER_IRQHandler
SPI1_IRQHandler
SPI2_IRQHandler
USART1_IRQHandler
USART2_IRQHandler
EXTI15_10_IRQHandler
RTC_Alarm_IRQHandler
OTG_FS_WKUP_IRQHandler
DMA1_Stream7_IRQHandler
SDIO_IRQHandler
TIM5_IRQHandler
SPI3_IRQHandler
DMA2_Stream0_IRQHandler
DMA2_Stream1_IRQHandler
DMA2_Stream2_IRQHandler
DMA2_Stream3_IRQHandler
DMA2_Stream4_IRQHandler
ETH_IRQHandler
OTG_FS_IRQHandler
DMA2_Stream5_IRQHandler
DMA2_Stream6_IRQHandler
DMA2_Stream7_IRQHandler
USART6_IRQHandler
I2C3_EV_IRQHandler
I2C3_ER_IRQHandler
FPU_IRQHandler
SPI4_IRQHandler
   
                B       .

                ENDP

                ALIGN

;*******************************************************************************
; 用户栈和堆初始化
;*******************************************************************************
                 IF      :DEF:__MICROLIB
                
                 EXPORT  __initial_sp
                 EXPORT  __heap_base
                 EXPORT  __heap_limit
                
                 ELSE
                
                 IMPORT  __use_two_region_memory
                 EXPORT  __user_initial_stackheap
                 
__user_initial_stackheap

                 LDR     R0, =  Heap_Mem
                 LDR     R1, =(Stack_Mem + Stack_Size)
                 LDR     R2, = (Heap_Mem +  Heap_Size)
                 LDR     R3, = Stack_Mem
                 BX      LR

                 ALIGN

                 ENDIF

                 END

;************************ (C) COPYRIGHT STMicroelectronics *****文件结束*****
