; main.s - GY-86驱动主程序 (STM32F401RET6)
    AREA    MAIN_PROGRAM, CODE, READONLY
    EXPORT  __main
    IMPORT  SystemClock_Init
    IMPORT  I2C1_Init
    IMPORT  I2C_WriteByte
    IMPORT  I2C_ReadBytes
    THUMB

; 常量定义
MPU6050_ADDR     EQU  0x68
HMC5883L_ADDR    EQU  0x1E
MS5611_ADDR      EQU  0x77

; MPU6050寄存器
MPU6050_WHO_AM_I     EQU  0x75
MPU6050_PWR_MGMT_1   EQU  0x6B
MPU6050_SMPLRT_DIV   EQU  0x19
MPU6050_CONFIG       EQU  0x1A
MPU6050_GYRO_CONFIG  EQU  0x1B
MPU6050_ACCEL_CONFIG EQU  0x1C
MPU6050_INT_ENABLE   EQU  0x38
MPU6050_ACCEL_XOUT_H EQU  0x3B
MPU6050_TEMP_OUT_H   EQU  0x41
MPU6050_GYRO_XOUT_H  EQU  0x43

; HMC5883L寄存器
HMC5883L_CONFIG_A    EQU  0x00
HMC5883L_CONFIG_B    EQU  0x01
HMC5883L_MODE        EQU  0x02
HMC5883L_DATA_X_MSB  EQU  0x03
HMC5883L_DATA_X_LSB  EQU  0x04
HMC5883L_DATA_Z_MSB  EQU  0x05
HMC5883L_DATA_Z_LSB  EQU  0x06
HMC5883L_DATA_Y_MSB  EQU  0x07
HMC5883L_DATA_Y_LSB  EQU  0x08

__main PROC
    BL SystemClock_Init
    BL I2C1_Init
    BL GY86_Init
    
    ; 验证MPU6050连接
    BL Check_MPU6050_Connection
    
main_loop
    ; 读取MPU6050数据
    BL Read_MPU6050_All
    
    ; 读取HMC5883L数据
    BL Read_HMC5883L_All
    
    ; 数据处理和转换
    BL Process_Sensor_Data
    
    ; 延时约100ms
    BL Delay_100ms
    
    B main_loop
    ENDP

; 检查MPU6050连接
Check_MPU6050_Connection PROC
    PUSH {R0-R2, LR}
    
    ; 读取WHO_AM_I寄存器 (应该返回0x68)
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_WHO_AM_I
    LDR R2, =mpu_whoami
    MOV R3, #1
    BL I2C_ReadBytes
    
    POP {R0-R2, PC}
    ENDP

; GY-86初始化
GY86_Init PROC
    PUSH {LR}
    
    ; 初始化MPU6050
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_PWR_MGMT_1
    MOV R2, #0x80           ; 设备复位
    BL I2C_WriteByte
    
    BL Delay_Short
    
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_PWR_MGMT_1
    MOV R2, #0x00           ; 退出睡眠模式
    BL I2C_WriteByte
    
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_SMPLRT_DIV
    MOV R2, #0x07           ; 采样率分频
    BL I2C_WriteByte
    
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_CONFIG
    MOV R2, #0x00           ; 配置
    BL I2C_WriteByte
    
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_GYRO_CONFIG
    MOV R2, #0x00           ; ±250°/s范围
    BL I2C_WriteByte
    
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_ACCEL_CONFIG
    MOV R2, #0x00           ; ±2g范围
    BL I2C_WriteByte
    
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_INT_ENABLE
    MOV R2, #0x00           ; 禁用中断
    BL I2C_WriteByte
    
    ; 初始化HMC5883L
    MOV R0, #HMC5883L_ADDR
    MOV R1, #HMC5883L_CONFIG_A
    MOV R2, #0x70           ; 8样本平均，15Hz输出
    BL I2C_WriteByte
    
    MOV R0, #HMC5883L_ADDR
    MOV R1, #HMC5883L_CONFIG_B
    MOV R2, #0xA0           ; 增益=5
    BL I2C_WriteByte
    
    MOV R0, #HMC5883L_ADDR
    MOV R1, #HMC5883L_MODE
    MOV R2, #0x00           ; 连续测量模式
    BL I2C_WriteByte
    
    POP {PC}
    ENDP

; 读取MPU6050所有数据 (加速度计、温度、陀螺仪)
Read_MPU6050_All PROC
    PUSH {R0-R3, LR}
    
    ; 从0x3B开始读取14个字节
    MOV R0, #MPU6050_ADDR
    MOV R1, #MPU6050_ACCEL_XOUT_H
    LDR R2, =mpu6050_raw_data
    MOV R3, #14
    BL I2C_ReadBytes
    
    ; 转换原始数据为有符号整数
    BL Convert_MPU6050_Data
    
    POP {R0-R3, PC}
    ENDP

; 转换MPU6050原始数据
Convert_MPU6050_Data PROC
    PUSH {R0-R7, LR}
    LDR R0, =mpu6050_raw_data
    LDR R1, =mpu6050_processed_data
    
    ; 加速度计X轴
    LDRB R2, [R0, #0]       ; ACCEL_XOUT_H
    LDRB R3, [R0, #1]       ; ACCEL_XOUT_L
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #0]       ; 存储为有符号16位
    
    ; 加速度计Y轴
    LDRB R2, [R0, #2]       ; ACCEL_YOUT_H
    LDRB R3, [R0, #3]       ; ACCEL_YOUT_L
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #2]
    
    ; 加速度计Z轴
    LDRB R2, [R0, #4]       ; ACCEL_ZOUT_H
    LDRB R3, [R0, #5]       ; ACCEL_ZOUT_L
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #4]
    
    ; 温度
    LDRB R2, [R0, #6]       ; TEMP_OUT_H
    LDRB R3, [R0, #7]       ; TEMP_OUT_L
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #6]
    
    ; 陀螺仪X轴
    LDRB R2, [R0, #8]       ; GYRO_XOUT_H
    LDRB R3, [R0, #9]       ; GYRO_XOUT_L
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #8]
    
    ; 陀螺仪Y轴
    LDRB R2, [R0, #10]      ; GYRO_YOUT_H
    LDRB R3, [R0, #11]      ; GYRO_YOUT_L
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #10]
    
    ; 陀螺仪Z轴
    LDRB R2, [R0, #12]      ; GYRO_ZOUT_H
    LDRB R3, [R0, #13]      ; GYRO_ZOUT_L
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #12]
    
    POP {R0-R7, PC}
    ENDP

; 读取HMC5883L所有数据
Read_HMC5883L_All PROC
    PUSH {R0-R3, LR}
    
    ; 从0x03开始读取6个字节
    MOV R0, #HMC5883L_ADDR
    MOV R1, #HMC5883L_DATA_X_MSB
    LDR R2, =hmc5883l_raw_data
    MOV R3, #6
    BL I2C_ReadBytes
    
    ; 转换原始数据为有符号整数
    BL Convert_HMC5883L_Data
    
    POP {R0-R3, PC}
    ENDP

; 转换HMC5883L原始数据
Convert_HMC5883L_Data PROC
    PUSH {R0-R7, LR}
    LDR R0, =hmc5883l_raw_data
    LDR R1, =hmc5883l_processed_data
    
    ; X轴 (注意：HMC5883L数据顺序是X, Z, Y)
    LDRB R2, [R0, #0]       ; DATA_X_MSB
    LDRB R3, [R0, #1]       ; DATA_X_LSB
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #0]
    
    ; Z轴
    LDRB R2, [R0, #2]       ; DATA_Z_MSB
    LDRB R3, [R0, #3]       ; DATA_Z_LSB
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #2]
    
    ; Y轴
    LDRB R2, [R0, #4]       ; DATA_Y_MSB
    LDRB R3, [R0, #5]       ; DATA_Y_LSB
    LSL R2, R2, #8
    ORR R2, R2, R3
    STRH R2, [R1, #4]
    
    POP {R0-R7, PC}
    ENDP

; 传感器数据处理
Process_Sensor_Data PROC
    PUSH {R0-R5, LR}
    
    ; 这里可以添加数据滤波、校准等处理
    ; 示例：计算加速度计模值
    LDR R0, =mpu6050_processed_data
    LDRSH R1, [R0, #0]      ; accel_x
    LDRSH R2, [R0, #2]      ; accel_y  
    LDRSH R3, [R0, #4]      ; accel_z
    
    ; 计算平方和 (简化版)
    MUL R4, R1, R1
    MUL R5, R2, R2
    ADD R4, R4, R5
    MUL R5, R3, R3
    ADD R4, R4, R5
    
    LDR R0, =sensor_calculations
    STR R4, [R0, #0]        ; 存储加速度模值平方
    
    POP {R0-R5, PC}
    ENDP

; 短延时
Delay_Short PROC
    PUSH {R0}
    MOV R0, #1000
delay_short_loop
    SUBS R0, R0, #1
    BNE delay_short_loop
    POP {R0}
    BX LR
    ENDP

; 100ms延时 (168MHz时钟)
Delay_100ms PROC
    PUSH {R0}
    LDR R0, =1680000        ; 168MHz下约100ms
delay_100ms_loop
    SUBS R0, R0, #1
    BNE delay_100ms_loop
    POP {R0}
    BX LR
    ENDP

; 数据段定义
    AREA    SENSOR_DATA, DATA, READWRITE

; MPU6050原始数据 (14字节)
mpu6050_raw_data     SPACE 14

; MPU6050处理后的数据 (7个16位值)
mpu6050_processed_data SPACE 14

; HMC5883L原始数据 (6字节)
hmc5883l_raw_data    SPACE 6

; HMC5883L处理后的数据 (3个16位值)
hmc5883l_processed_data SPACE 6

; 其他数据
mpu_whoami           SPACE 1
sensor_calculations  SPACE 16

    END
		