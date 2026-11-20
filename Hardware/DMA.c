#include "DMA.h"
#include "MPU6050.h"
#include "MPU6050Rag.h"
#include "MYI2C.h"
#include "delay.h"




#define IMU_DMA_START_TIMEOUT_US 1000u
//I2C错误标志汇总
#define IMU_I2C_ERROR_MASK (I2C_SR1_AF|I2C_SR1_BERR|I2C_SR1_ARLO|I2C_SR1_OVR)
//DMA事件与错误标志汇总
#define IMU_DMA_ALL_FLAGS (DMA_FLAG_FEIF0|DMA_FLAG_DMEIF0|DMA_FLAG_TEIF0|DMA_FLAG_HTIF0|DMA_FLAG_TCIF0)

/* 缓冲区只在这里定义一次 */
uint8_t imu_dma_rx[MPU_DMA_RX_SIZE];

volatile IMU_DMA_State ImuDmaState = IMU_DMA_IDLE;

OS_EVENT *ImuDmaDoneSem = (OS_EVENT *)0;

/* 判断本次启动过程是否超过时间预算 */
static uint8_t IMU_DMA_StartTimedOut(uint32_t start_cycles)
{
    uint32_t timeout_cycles;

    timeout_cycles =
        (SystemCoreClock / 1000000u) *
        IMU_DMA_START_TIMEOUT_US;

    return (uint32_t)(DWT->CYCCNT - start_cycles)
           >= timeout_cycles;
}

/*
 * 等待SR1中的指定标志全部置位。
 * 注意：这里只读取SR1，不读取SR2，
 * 因而不会提前清除ADDR。
 */
static uint8_t IMU_DMA_WaitSR1(uint32_t mask,uint32_t start_cycles)
{
    uint32_t sr1;

    for (;;)
    {
        sr1 = I2C1->SR1;

        if ((sr1 & IMU_I2C_ERROR_MASK) != 0u){
            return 0u;
        }

        if ((sr1 & mask) == mask){
            return 1u;
        }

        if (IMU_DMA_StartTimedOut(start_cycles)){
            return 0u;
        }
    }
}

/*
 * 等待上一次STOP结束、总线空闲。
 * 只在本次START之前使用。
 */
static uint8_t IMU_DMA_WaitBusIdle(uint32_t start_cycles){
    while (((I2C1->CR1 & I2C_CR1_STOP) != 0u) ||
           ((I2C1->SR2 & I2C_SR2_BUSY) != 0u)){
        if (IMU_DMA_StartTimedOut(start_cycles)){
            return 0u;
        }
    }

    return 1u;
}

/* I2C规定：先读SR1，再读SR2，清除ADDR */
static void IMU_DMA_ClearADDR(void){
    volatile uint32_t discard;

    discard = I2C1->SR1;
    discard = I2C1->SR2;

    (void)discard;//消除“变量被赋值但从未使用”警告
}

/*
 * CPU寻址阶段失败的收尾。
 * 本函数只在DMA尚未启动时调用。
 * DMA启动后的错误/超时，需要另外停止DMA再恢复总线。
 */
static void IMU_DMA_StartFailed(void){
    I2C_DMACmd(I2C1, DISABLE);//失能DMA
    I2C_DMALastTransferCmd(I2C1, DISABLE);//清除last位，防止下一次传输意外停止

    (void)MYI2C_Recover();//恢复PB8PB9总线

    ImuDmaState = IMU_DMA_ERROR;
}

void IMU_DMA_Init(void){
    DMA_InitTypeDef dma;
    NVIC_InitTypeDef nvic = {0};

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    DMA_DeInit(DMA1_Stream0);
    DMA_StructInit(&dma);

    dma.DMA_Channel = DMA_Channel_1;

    dma.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DR;
    dma.DMA_Memory0BaseAddr = (uint32_t)imu_dma_rx;

    dma.DMA_DIR = DMA_DIR_PeripheralToMemory;
    dma.DMA_BufferSize = MPU_DMA_RX_SIZE;

    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;

    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;

    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_High;

    dma.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dma.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dma.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    DMA_Init(DMA1_Stream0, &dma);

    DMA_ITConfig(DMA1_Stream0,
                 DMA_IT_TC | DMA_IT_TE | DMA_IT_DME,
                 ENABLE);

    nvic.NVIC_IRQChannel = DMA1_Stream0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 6u;
    nvic.NVIC_IRQChannelSubPriority = 0u;
    nvic.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&nvic);
}

uint8_t MPU6050_ReadRawDMA_Start(uint8_t *buffer){
    uint32_t start_cycles;
    OS_CPU_SR cpu_sr;

    /* ---------- 1. 检查调用条件 ---------- */

    if ((buffer == (uint8_t *)0) ||
        (ImuDmaDoneSem == (OS_EVENT *)0)){
        return 0u;
    }

    /* 上一次事务尚未完成，不能覆盖其配置 */
    if ((ImuDmaState == IMU_DMA_STARTING) ||
        (ImuDmaState == IMU_DMA_RECEIVING)){
        return 0u;
    }

    /*
     * DMA使能期间不能修改M0AR、NDTR等传输配置。
     * 如果仍在运行，直接拒绝本次启动。
     */
    if (DMA_GetCmdStatus(DMA1_Stream0) != DISABLE){
        return 0u;
    }

    /* 确保DWT计数器已启用 */
    Delay_Init();

    start_cycles = DWT->CYCCNT;

    ImuDmaState = IMU_DMA_STARTING;


    /* ---------- 2. 等待上一次I2C事务结束 ---------- */

    if (!IMU_DMA_WaitBusIdle(start_cycles)){
        goto start_failed;
    }


    /* ---------- 3. 清理上一次传输状态 ---------- */

    I2C_DMACmd(I2C1, DISABLE);
    I2C_DMALastTransferCmd(I2C1, DISABLE);

    DMA_ClearFlag(DMA1_Stream0, IMU_DMA_ALL_FLAGS);
    NVIC_ClearPendingIRQ(DMA1_Stream0_IRQn);

    /*
     * 清除旧完成通知。
     * 前提：调用者已经处理完上一次事务。
     */
    while (OSSemAccept(ImuDmaDoneSem) != 0u){}


    /* ---------- 4. 设置本次DMA目标和数量 ---------- */

    DMA1_Stream0->M0AR = (uint32_t)buffer;//M0AR为DMA内存目标地址

    DMA_SetCurrDataCounter(DMA1_Stream0,MPU_DMA_RX_SIZE);//配置长度


    /* ---------- 5. 恢复正常ACK配置 ---------- */

    /*
     * 清POS：不使用两字节接收的特殊NACK位置。
     * 打开ACK：中间字节正常应答。
     */
    I2C_NACKPositionConfig(I2C1,I2C_NACKPosition_Current);//清掉“下一字节位置 NACK”的特殊配置，防止上一次接受影响本次

    I2C_AcknowledgeConfig(I2C1, ENABLE);


    /* ---------- 6. 产生第一次START ---------- */

    I2C_GenerateSTART(I2C1, ENABLE);

    /* SB=1，表示START已发送 */
    if (!IMU_DMA_WaitSR1(I2C_SR1_SB, start_cycles)){
        goto start_failed;
    }


    /* ---------- 7. 发送MPU写地址 ---------- */

    /*
     * MPU6050_ADDRESS_7BIT = 0x68
     * 标准库这个函数需要传入左移后的地址。
     *
     * 写方向：总线上发送0xD0。
     */
    I2C_Send7bitAddress(
        I2C1,
        (uint8_t)(MPU6050_ADDRESS_7BIT << 1),
        I2C_Direction_Transmitter);

    if (!IMU_DMA_WaitSR1(I2C_SR1_ADDR, start_cycles)){
        goto start_failed;
    }

    /* 写方向的ADDR此时可以清除 */
    IMU_DMA_ClearADDR();


    /* ---------- 8. 发送起始寄存器地址0x3B ---------- */

    if (!IMU_DMA_WaitSR1(I2C_SR1_TXE, start_cycles)){
        goto start_failed;
    }

    I2C_SendData(I2C1, MPU6050_ACCEL_XOUT_H);

    /*
     * 等待BTF，确认0x3B已经发送完成。
     * 仅TXE表示DR空，不代表总线上的字节已发送结束。
     */
    if (!IMU_DMA_WaitSR1(I2C_SR1_BTF, start_cycles)){
        goto start_failed;
    }


    /* ---------- 9. 产生重复START ---------- */

    /*
     * 中间不发送STOP，保留同一笔寄存器读取事务。
     */
    I2C_GenerateSTART(I2C1, ENABLE);
	//SB（Start Bit）置位表示 START 已在总线上发出，此时可以写 DR 发地址
    if (!IMU_DMA_WaitSR1(I2C_SR1_SB, start_cycles)){
		
        goto start_failed;
    }


    /* ---------- 10. 发送MPU读地址 ---------- */

    /*
     * 读方向：总线上发送0xD1。
     * 库函数负责设置方向位。
     */
    I2C_Send7bitAddress(I2C1,(uint8_t)(MPU6050_ADDRESS_7BIT << 1),I2C_Direction_Receiver);

    if (!IMU_DMA_WaitSR1(I2C_SR1_ADDR, start_cycles)){
        goto start_failed;
    }

    /*
     * 关键：
     * 这里先不清ADDR。
     * 先准备DMA，再释放接收流程。
     */


    /* ---------- 11. 将接收阶段交给DMA ---------- */

    OS_ENTER_CRITICAL();

    /*
     * LAST=1：
     * 最后一个DMA接收字节由I2C硬件产生NACK。
     */
    I2C_DMALastTransferCmd(I2C1, ENABLE);

    /*
     * 必须在启用DMA之前更新状态，
     * 避免完成中断先发生、任务随后又覆盖状态。
     */
    ImuDmaState = IMU_DMA_RECEIVING;

    /*
     * 确保前面的内存访问完成，
     * 再允许DMA开始访问目标内存。
     */
    __DMB();

    /* 先准备DMA Stream，再允许I2C发出DMA请求 */
    DMA_Cmd(DMA1_Stream0, ENABLE);
    I2C_DMACmd(I2C1, ENABLE);

    /*
     * 最后清除接收方向ADDR。
     * I2C开始接收，DMA将DR中的数据搬入buffer。
     */
    IMU_DMA_ClearADDR();

    OS_EXIT_CRITICAL();


    /* ---------- 12. 启动成功，立即返回 ---------- */

    return 1u;


start_failed:

    IMU_DMA_StartFailed();

    return 0u;
}


//判断接收成功或失败、结束传输，并通过信号量唤醒采集任
void DMA1_Steam0_IRQHandler(void){
	uint8_t transfer_complete;
    uint8_t dma_error;
    uint8_t receive_ok;
    uint32_t i2c_error;
	
	OSIntEnter();
	
	/*
	 * 1.读取中断原因
	 * TC：传输完成
	 * TE：传输错误
	 * DME：直接模式错误
	 */
	
	transfer_complete =
        (DMA_GetITStatus(DMA1_Stream0,
                         DMA_IT_TCIF0) != RESET);

    dma_error =
        (DMA_GetITStatus(DMA1_Stream0,
                         DMA_IT_TEIF0) != RESET) ||
        (DMA_GetITStatus(DMA1_Stream0,
                         DMA_IT_DMEIF0) != RESET);
						 
	if(transfer_complete||dma_error){
		if(ImuDmaState==IMU_DMA_RECEIVING){//只有正在接受的事务在这里结束
			i2c_error=I2C1->SR1&IMU_I2C_ERROR_MASK;//检查I2C错误，但是不清除错误
			
			/*
             * 成功必须同时满足：
             * - 收到了完成中断
             * - 没有DMA错误
             * - 没有I2C错误
             * - 剩余传输数量为0
             */
			receive_ok=
				transfer_complete&&
				!dma_error&&
				(i2c_error==0)&&
				(DMA_GetCurrDataCounter(DMA1_Stream0)==0);
			
			//结束本次I2C接收
			I2C_AcknowledgeConfig(I2C1,DISABLE);
			
			//主机模式才能请求STOP
			if((I2C1->SR2&I2C_SR2_MSL)!=0){
				I2C_GenerateSTOP(I2C1,ENABLE);
			}
			
			I2C_DMACmd(I2C1,DISABLE);//禁止I2C发送新dma请求
			I2C_DMALastTransferCmd(I2C1,DISABLE);//清除last配置
			DMA_Cmd(DMA1_Stream0,DISABLE);//关闭dma
			DMA_ClearFlag(DMA1_Stream0,IMU_DMA_ALL_FLAGS);
			
			__DMB();//保证屏障之前的内存访问在屏障之后的内存访问之前
			
			if(receive_ok){
				ImuDmaState=IMU_DMA_DONE;
			}else{
				ImuDmaState=IMU_DMA_ERROR;
			}
			
			//通知采集任务结果
			if(ImuDmaDoneSem!=((OS_EVENT*)0)){
				(void)OSSemPost(ImuDmaDoneSem);//void显示丢弃返回值
			}else{
				DMA_ClearFlag(DMA1_Stream0,IMU_DMA_ALL_FLAGS);
			}
			
			OSIntExit();
			
	
		}
	}
					

}
