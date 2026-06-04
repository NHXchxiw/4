#include "stm32f4xx.h"                  // Device header
#include "Delay.h"

void MYI2C_W_SCL(uint8_t BitValue){
	GPIO_WriteBit(GPIOB,GPIO_Pin_8,(BitAction)BitValue);
	Delay_us(10);
}

void MYI2C_W_SDA(uint8_t BitValue){
	GPIO_WriteBit(GPIOB,GPIO_Pin_9,(BitAction)BitValue);
	Delay_us(10);
}

uint8_t MYI2C_R_SDA(void){
	uint8_t BitValue;
	BitValue=GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9);
	Delay_us(10);
	return BitValue;
}

void MYI2C_Init(void){
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_I2S2CLKSource_PLLI2S, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	GPIO_SetBits(GPIOB,GPIO_Pin_8 | GPIO_Pin_9);
}

void MYI2C_Start(void){
	MYI2C_W_SDA(1);
	MYI2C_W_SCL(1);
	MYI2C_W_SDA(0);
	MYI2C_W_SCL(0);
}

void MYI2C_Stop(void){
	MYI2C_W_SDA(0);
	MYI2C_W_SCL(1);
	MYI2C_W_SDA(1);
}

void MYI2C_SendByte(uint8_t Byte){
	for(uint8_t i=0;i<8;i++){
		MYI2C_W_SDA(Byte & (0x80 >> i));//Byte最高位
		MYI2C_W_SCL(1);
		MYI2C_W_SCL(0);
	}
}

uint8_t MYI2C_ReceiveByte(void){
	uint8_t Byte=0x00;
	MYI2C_W_SDA(1);
	for(uint8_t i=0;i<8;i++){
		MYI2C_W_SCL(1);
		if(MYI2C_R_SDA()==1)Byte |= (0x80 >> i);
		MYI2C_W_SCL(0);
	}
	return Byte;
}

void MYI2C_SendAck(uint8_t AckBit){
	MYI2C_W_SDA(AckBit);
	MYI2C_W_SCL(1);
	MYI2C_W_SCL(0);
}

uint8_t MYI2C_ReceiveAck(void){
	uint8_t AckBit;
	MYI2C_W_SDA(1);
	MYI2C_W_SCL(1);
	AckBit = MYI2C_R_SDA();
	MYI2C_W_SCL(0);
	return AckBit;
}

