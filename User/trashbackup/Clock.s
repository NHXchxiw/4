; Clock.s - STM32F401RET6 系统时钟初始化
    AREA    CLOCK_INIT, CODE, READONLY
    EXPORT  SystemClock_Init
    THUMB

SystemClock_Init PROC
    ; 使能外部晶振 (HSE)
    LDR R0, =0x40023800      ; RCC基地址 (F401)
    LDR R1, [R0, #0x00]      ; RCC_CR
    ORR R1, R1, #0x00010000  ; 使能HSE
    STR R1, [R0, #0x00]
    
    ; 等待HSE就绪
wait_hse
    LDR R1, [R0, #0x00]
    ANDS R1, R1, #0x00020000
    BEQ wait_hse
    
    ; 配置PLL (8MHz * 84 = 336MHz, 然后/2 = 168MHz系统时钟)
    LDR R1, [R0, #0x04]      ; RCC_PLLCFGR
    BIC R1, R1, #0xFFFFFFFF  ; 清除所有配置
    ORR R1, R1, #0x00400000  ; PLL源选择HSE
    ORR R1, R1, #0x00000004  ; PLLM = 4 (8MHz/4 = 2MHz)
    ORR R1, R1, #0x0000A800  ; PLLN = 168 (2MHz * 168 = 336MHz)
    ORR R1, R1, #0x00010000  ; PLLP = 2 (336MHz/2 = 168MHz)
    ORR R1, R1, #0x00000000  ; PLLQ = 4 (336MHz/4 = 84MHz)
    STR R1, [R0, #0x04]
    
    ; 配置预分频器
    LDR R1, [R0, #0x08]      ; RCC_CFGR
    BIC R1, R1, #0x000000F0  ; 清除AHB预分频
    ORR R1, R1, #0x00000000  ; AHB不分频 (168MHz)
    BIC R1, R1, #0x00001C00  ; 清除APB1预分频
    ORR R1, R1, #0x00001400  ; APB1 4分频 (42MHz)
    BIC R1, R1, #0x0000E000  ; 清除APB2预分频
    ORR R1, R1, #0x00008000  ; APB2 2分频 (84MHz)
    STR R1, [R0, #0x08]
    
    ; 使能PLL
    LDR R1, [R0, #0x00]
    ORR R1, R1, #0x01000000
    STR R1, [R0, #0x00]
    
    ; 等待PLL就绪
wait_pll
    LDR R1, [R0, #0x00]
    ANDS R1, R1, #0x02000000
    BEQ wait_pll
    
    ; 设置Flash等待状态
    LDR R0, =0x40023C00      ; FLASH基地址 (F401)
    LDR R1, =0x00000502      ; 5等待状态，预取指使能，指令缓存使能，数据缓存使能
    STR R1, [R0, #0x00]
    
    ; 切换系统时钟到PLL
    LDR R0, =0x40023800      ; RCC基地址
    LDR R1, [R0, #0x08]      ; RCC_CFGR
    ORR R1, R1, #0x00000002  ; 选择PLL作为系统时钟
    STR R1, [R0, #0x08]
    
    ; 等待时钟切换完成
wait_clock
    LDR R1, [R0, #0x08]
    AND R1, R1, #0x0000000C
    CMP R1, #0x00000008
    BNE wait_clock
    
    BX LR
    ENDP
    END