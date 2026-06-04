; I2C_Init.s - STM32F401RET6 I2C3初始化 (使用PC0-SCL, PC1-SDA)
    AREA    I2C_INIT, CODE, READONLY
    EXPORT  I2C1_Init
    EXPORT  I2C_WriteByte
    EXPORT  I2C_ReadBytes
    THUMB

I2C1_Init PROC
    ; 使能GPIOC和I2C3时钟
    LDR R0, =0x40023800      ; RCC基地址 (F401)
    LDR R1, [R0, #0x30]      ; RCC_AHB1ENR
    ORR R1, R1, #0x00000004  ; 使能GPIOC时钟
    STR R1, [R0, #0x30]
    
    LDR R1, [R0, #0x40]      ; RCC_APB1ENR
    ORR R1, R1, #0x40000000  ; 使能I2C3时钟
    STR R1, [R0, #0x40]
    
    ; 配置PC0(SCL), PC1(SDA)为AF4开漏输出
    LDR R0, =0x40020800      ; GPIOC基地址
    LDR R1, [R0, #0x00]      ; GPIOC_MODER
    BIC R1, R1, #0x0000000F  ; 清除PC0,PC1模式
    ORR R1, R1, #0x0000000A  ; PC0,PC1设置为AF模式 (10)
    STR R1, [R0, #0x00]
    
    LDR R1, [R0, #0x08]      ; GPIOC_OTYPER
    ORR R1, R1, #0x00000003  ; PC0,PC1设置为开漏输出
    STR R1, [R0, #0x08]
    
    LDR R1, [R0, #0x0C]      ; GPIOC_OSPEEDR
    ORR R1, R1, #0x0000000F  ; PC0,PC1高速模式
    STR R1, [R0, #0x0C]
    
    LDR R1, [R0, #0x20]      ; GPIOC_AFRL
    BIC R1, R1, #0x000000FF  ; 清除PC0,PC1 AF配置
    ORR R1, R1, #0x00000044  ; PC0,PC1设置为AF4 (I2C3)
    STR R1, [R0, #0x20]
    
    ; 配置I2C3
    LDR R0, =0x40005C00      ; I2C3基地址
    
    ; 禁用I2C3
    MOV R1, #0x0000
    STR R1, [R0, #0x00]      ; I2C_CR1 PE=0
    
    ; 配置时钟频率 (42MHz PCLK1, 标准模式100kHz)
    MOV R1, #0x00D2          ; 42MHz/100kHz/2 = 210, 0xD2 = 210
    STR R1, [R0, #0x1C]      ; I2C_CCR
    
    ; 配置上升时间
    MOV R1, #0x2B            ; 1000ns最大上升时间 (42MHz * 1000ns = 42 -> 0x2B)
    STR R1, [R0, #0x20]      ; I2C_TRISE
    
    ; 使能I2C3
    MOV R1, #0x0001
    STR R1, [R0, #0x00]      ; I2C_CR1 PE=1
    
    BX LR
    ENDP

; I2C启动条件
I2C_GenerateSTART PROC
    LDR R1, =0x40005C00      ; I2C3基地址
    MOV R2, #0x0100          ; START位
    LDR R3, [R1, #0x00]      ; I2C_CR1
    ORR R3, R3, R2
    STR R3, [R1, #0x00]
    
    ; 等待START条件发送完成
wait_start
    LDR R3, [R1, #0x18]      ; I2C_SR1
    TST R3, #0x0001          ; 检查SB标志
    BEQ wait_start
    
    BX LR
    ENDP

; I2C发送地址
I2C_SendAddress PROC
    ; R0: 地址 + 读/写位
    LDR R1, =0x40005C00
    STR R0, [R1, #0x10]      ; I2C_DR
    
    ; 等待地址发送完成
wait_addr
    LDR R2, [R1, #0x18]      ; I2C_SR1
    TST R2, #0x0002          ; 检查ADDR标志
    BEQ wait_addr
    
    ; 清除ADDR标志
    LDR R2, [R1, #0x18]      ; 读SR1
    LDR R2, [R1, #0x14]      ; 读SR2
    BX LR
    ENDP

; I2C发送数据
I2C_SendData PROC
    ; R0: 数据
    LDR R1, =0x40005C00
    STR R0, [R1, #0x10]      ; I2C_DR
    
    ; 等待数据发送完成
wait_data
    LDR R2, [R1, #0x18]      ; I2C_SR1
    TST R2, #0x0084          ; 检查BTF或TXE
    BEQ wait_data
    
    BX LR
    ENDP

; I2C停止条件
I2C_GenerateSTOP PROC
    LDR R1, =0x40005C00
    MOV R2, #0x0200          ; STOP位
    LDR R3, [R1, #0x00]      ; I2C_CR1
    ORR R3, R3, R2
    STR R3, [R1, #0x00]
    
    ; 等待总线空闲
wait_stop
    LDR R3, [R1, #0x18]      ; I2C_SR1
    TST R3, #0x0004          ; 检查STOPF标志
    BEQ wait_stop
    
    BX LR
    ENDP

; I2C写单字节
I2C_WriteByte PROC
    ; R0: I2C地址, R1: 寄存器地址, R2: 数据
    PUSH {R3-R5, LR}
    
    BL I2C_GenerateSTART
    
    ; 发送设备地址(写模式)
    LSL R3, R0, #1           ; 左移1位，最低位为0(写)
    MOV R0, R3
    BL I2C_SendAddress
    
    ; 发送寄存器地址
    MOV R0, R1
    BL I2C_SendData
    
    ; 发送数据
    MOV R0, R2
    BL I2C_SendData
    
    BL I2C_GenerateSTOP
    
    POP {R3-R5, PC}
    ENDP

; I2C读多字节
I2C_ReadBytes PROC
    ; R0: I2C地址, R1: 寄存器地址, R2: 数据缓冲区, R3: 字节数
    PUSH {R4-R7, LR}
    LDR R4, =0x40005C00      ; I2C3基地址
    
    ; 第一阶段：写寄存器地址
    BL I2C_GenerateSTART
    
    ; 发送设备地址(写模式)
    LSL R5, R0, #1           ; 左移1位，最低位为0(写)
    MOV R0, R5
    BL I2C_SendAddress
    
    ; 发送寄存器地址
    MOV R0, R1
    BL I2C_SendData
    
    ; 重新起始条件
    BL I2C_GenerateSTART
    
    ; 发送设备地址(读模式)
    LSL R5, R0, #1
    ORR R5, R5, #1           ; 最低位为1(读)
    MOV R0, R5
    BL I2C_SendAddress
    
    ; 配置ACK
    LDR R5, [R4, #0x00]      ; I2C_CR1
    ORR R5, R5, #0x0400      ; 使能ACK
    STR R5, [R4, #0x00]
    
    ; 读取数据
    SUBS R3, R3, #1
    BEQ read_last_byte
    
read_loop
    ; 等待RXNE
wait_rxne
    LDR R5, [R4, #0x18]      ; I2C_SR1
    TST R5, #0x0040          ; 检查RXNE
    BEQ wait_rxne
    
    LDR R6, [R4, #0x10]      ; 读取数据
    STRB R6, [R2], #1        ; 存储并递增指针
    
    SUBS R3, R3, #1
    BNE read_loop

read_last_byte
    ; 最后一个字节，禁用ACK
    LDR R5, [R4, #0x00]      ; I2C_CR1
    BIC R5, R5, #0x0400      ; 禁用ACK
    STR R5, [R4, #0x00]
    
    ; 等待最后一个字节
wait_last
    LDR R5, [R4, #0x18]      ; I2C_SR1
    TST R5, #0x0040          ; 检查RXNE
    BEQ wait_last
    
    LDR R6, [R4, #0x10]      ; 读取最后一个字节
    STRB R6, [R2], #1
    
    BL I2C_GenerateSTOP
    
    ; 重新使能ACK
    LDR R5, [R4, #0x00]      ; I2C_CR1
    ORR R5, R5, #0x0400      ; 使能ACK
    STR R5, [R4, #0x00]
    
    POP {R4-R7, PC}
    ENDP

    END