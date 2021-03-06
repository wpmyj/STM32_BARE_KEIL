#define SPI_FLASH_GLOBAL

#include "tiza_include.h"

//#define SPIFLASH_DEBUG

//指令表
#define W25X_WriteEnable			0x06 
#define W25X_WriteDisable			0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData					0x03 
#define W25X_FastReadData			0x0B 
#define W25X_FastReadDual			0x3B 
#define W25X_PageProgram			0x02 
#define W25X_BlockErase				0xD8 
#define W25X_SectorErase			0x20 
#define W25X_ChipErase				0xC7 
#define W25X_PowerDown				0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID					0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F 


static void SpiFlashWaitBusy(void);           	//等待空闲
static void SpiFlashWriteEnable(void);  		//写使能 
static void SpiFlashWriteDisable(void);			//写保护
static uint8 SpiFlashReadSR(void);        		//读取状态寄存器 
static void SpiFlashWriteSR(uint8 sr);  		//写状态寄存器


//static uint8 SPIFLASHBUFFER[4096];		// 缓存

#define SPI_FLASH_CAPCITY	(8*1024*1024)

static void spidelay_ms(uint16 time)
{ 
  uint32 tmp;// = time * 21;
  while(time--)
  {
    tmp = 21000;
    while(tmp--)
    {
      __NOP();
      __NOP();
      __NOP();
      __NOP();
    }
  }
}


//4Kbytes为一个Sector
//16个扇区为1个Block
//W25Q128
//容量为16M字节,共有128个Block,4096个Sector 


//初始化SPI FLASH
uint8 ExteFlashInit(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  SPI_InitTypeDef  SPI_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  SPI_FRAM_CS_HIGH();

  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;

  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;  ///时钟悬空低
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; ///数据在第1个时钟边沿捕获
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;    ///NSS-片选，由软件管理
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;			//72/4=18MHZ
  ///SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;			///16/2=8MHZ

  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);

  SPI_Cmd(SPI1, ENABLE);
	return 1;
}

//SPI1 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
uint8 SPI1_ReadWriteByte(uint8 TxData)
{		 			 
 
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET){}//等待发送区空  

	SPI_I2S_SendData(SPI1, TxData); //通过外设SPIx发送一个byte  数据

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET){} //等待接收完一个byte  

	return SPI_I2S_ReceiveData(SPI1); //返回通过SPIx最近接收的数据	
}
//读取W25QXX的状态寄存器
//BIT7  6   5   4   3   2   1   0
//SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//SPR:默认0,状态寄存器保护位,配合WP使用
//TB,BP2,BP1,BP0:FLASH区域写保护设置
//WEL:写使能锁定
//BUSY:忙标记位(1,忙;0,空闲)
//默认:0x00
uint8 SpiFlashReadSR(void)   
{  
	uint8 byte=0;   
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_ReadStatusReg);    //发送读取状态寄存器命令    
	byte=SPI1_ReadWriteByte(0Xff);             //读取一个字节  
	SPI_CS_DISABLE;                            //取消片选     
	return byte;   
} 

//写W25QXX状态寄存器
//只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写!!!
void SpiFlashWriteSR(uint8 sr)   
{   
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_WriteStatusReg);   //发送写取状态寄存器命令    
	SPI1_ReadWriteByte(sr);               //写入一个字节  
	SPI_CS_DISABLE;                            //取消片选     	      
}   

//W25QXX写使能	
//将WEL置位   
void SpiFlashWriteEnable(void)   
{
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_WriteEnable);      //发送写使能  
	SPI_CS_DISABLE;                            //取消片选     	      
} 

//W25QXX写禁止	
//将WEL清零  
void SpiFlashWriteDisable(void)   
{  
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_WriteDisable);     //发送写禁止指令    
	SPI_CS_DISABLE;                            //取消片选     	      
} 		

//读取芯片ID
//返回值如下:				   
//0XEF13,表示芯片型号为W25Q80  
//0XEF14,表示芯片型号为W25Q16    
//0XEF15,表示芯片型号为W25Q32  
//0XEF16,表示芯片型号为W25Q64 
//0XEF17,表示芯片型号为W25Q128 	  
#if 0
uint16 SpiFlashReadID(void)
{
	uint16 Temp = 0;	  
	SPI_CS_ENABLE;				    
	SPI1_ReadWriteByte(W25X_ManufactDeviceID);//发送读取ID命令	    
	SPI1_ReadWriteByte(0x00); 	    
	SPI1_ReadWriteByte(0x00); 	    
	SPI1_ReadWriteByte(0x00); 	 			   
	Temp|=SPI1_ReadWriteByte(0xFF)<<8;  
	Temp|=SPI1_ReadWriteByte(0xFF);	 
	SPI_CS_DISABLE;				    
	return Temp;
}   		    
#endif

//读取SPI FLASH  
//在指定地址开始读取指定长度的数据
//pBuffer:数据存储区
//ReadAddr:开始读取的地址(24bit)
//NumByteToRead:要读取的字节数(最大65535)
uint8 ExteFlashRead(uint32 ReadAddr, uint8* pBuffer, uint16 NumByteToRead)   
{ 
 	uint16 i;   										    
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_ReadData);         //发送读取命令   
	SPI1_ReadWriteByte((uint8)((ReadAddr)>>16));  //发送24bit地址    
	SPI1_ReadWriteByte((uint8)((ReadAddr)>>8));   
	SPI1_ReadWriteByte((uint8)ReadAddr);   
	for(i=0;i<NumByteToRead;i++)
	{ 
			pBuffer[i]=SPI1_ReadWriteByte(0XFF);   //循环读数  
	}
	SPI_CS_DISABLE;  				    	      

	return 1;
}  

//SPI在一页(0~65535)内写入少于256个字节的数据
//在指定地址开始写入最大256字节的数据
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!	 
static void SpiFlashWritePage(uint8* pBuffer,uint32 WriteAddr,uint16 NumByteToWrite)
{
 	uint16 i;  
	SpiFlashWriteEnable();                  //SET WEL 
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_PageProgram);      //发送写页命令   
	SPI1_ReadWriteByte((uint8)((WriteAddr)>>16)); //发送24bit地址    
	SPI1_ReadWriteByte((uint8)((WriteAddr)>>8));   
	SPI1_ReadWriteByte((uint8)WriteAddr);   
	for(i=0;i<NumByteToWrite;i++)SPI1_ReadWriteByte(pBuffer[i]);//循环写数  
	SPI_CS_DISABLE;                            //取消片选 
	SpiFlashWaitBusy();					   //等待写入结束
} 

//无检验写SPI FLASH 
//必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
//具有自动换页功能 
//在指定地址开始写入指定长度的数据,但是要确保地址不越界!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
//CHECK OK
uint8 ExteFlashWrite(uint32 WriteAddr,uint8* pBuffer,uint16 NumByteToWrite)   
{ 			 		 
	uint16 pageremain;	   

	if(WriteAddr + NumByteToWrite >= SPI_FLASH_CAPCITY)
		return 0;
	
	pageremain=256-WriteAddr%256; //单页剩余的字节数		 	    
	
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//不大于256个字节
	while(1)
	{	   
		SpiFlashWritePage(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//写入结束了
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			  //减去已经写入了的字节数
			if(NumByteToWrite>256)pageremain=256; //一次可以写入256个字节
			else pageremain=NumByteToWrite; 	  //不够256个字节了
		}
	};	    

	return 1;
} 

//写SPI FLASH  
//在指定地址开始写入指定长度的数据
//该函数带擦除操作!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)						
//NumByteToWrite:要写入的字节数(最大65535)  
#if 0
void SpiFlashWrite(uint32 WriteAddr, uint8* pBuffer, uint16 NumByteToWrite)   
{ 
	uint32 secpos;
	uint16 secoff;
	uint16 secremain;	   
 	uint16 i;    
	uint8 * W25QXX_BUF;	
  
  W25QXX_BUF=SPIFLASHBUFFER;	     
 	secpos=WriteAddr/4096;//扇区地址  
	secoff=WriteAddr%4096;//在扇区内的偏移
	secremain=4096-secoff;//扇区剩余空间大小   
	
 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//测试用
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//不大于4096个字节
	while(1) 
	{	
		SpiFlashRead(secpos*4096, W25QXX_BUF, 4096);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(W25QXX_BUF[secoff+i]!=0XFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			SpiFlashEraseSector(secpos);//擦除这个扇区
			for(i=0;i<secremain;i++)	   //复制
			{
				W25QXX_BUF[i+secoff]=pBuffer[i];	  
			}
			SpiFlashWriteNoCheck(W25QXX_BUF,secpos*4096,4096);//写入整个扇区  

		}else SpiFlashWriteNoCheck(pBuffer,WriteAddr,secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
		if(NumByteToWrite==secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0 	 

			pBuffer+=secremain;  //指针偏移
			WriteAddr+=secremain;//写地址偏移	   
			NumByteToWrite-=secremain;				//字节数递减
			if(NumByteToWrite>4096)secremain=4096;	//下一个扇区还是写不完
			else secremain=NumByteToWrite;			//下一个扇区可以写完了
		}	 
	};	 
}
#endif

//擦除整个芯片		  
//等待时间超长...
uint8 ExteFlashEraseChip(void)   
{                                   
	SpiFlashWriteEnable();                  //SET WEL 
	SpiFlashWaitBusy();   
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_ChipErase);        //发送片擦除命令  
	SPI_CS_DISABLE;                            //取消片选     	      
	SpiFlashWaitBusy();   				   //等待芯片擦除结束

	return 1;
}   

//擦除一个扇区
//Dst_Addr:扇区地址 根据实际容量设置
//擦除一个山区的最少时间:150ms
//0失败 1成功
uint8 ExteFlashEraseSector(uint16 SectorNum)   
{
	uint32 Dst_Addr;
	
//	//监视falsh擦除情况,测试用   
//	#ifdef SPIFLASH_DEBUG
// 	printf("spi flash sector: 0x%X\r\n",SectorNum);	  
//	#endif

	if(SectorNum * SPIFLASH_SECTOR_SIZE > SPI_FLASH_CAPCITY)
		return 0;
	
	Dst_Addr = SectorNum * SPIFLASH_SECTOR_SIZE;			// 4096 bytes per page

	SpiFlashWriteEnable();                  //SET WEL 	 
	SpiFlashWaitBusy();   
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_SectorErase);      //发送扇区擦除指令 
	SPI1_ReadWriteByte((uint8)((Dst_Addr)>>16));  //发送24bit地址    
	SPI1_ReadWriteByte((uint8)((Dst_Addr)>>8));   
	SPI1_ReadWriteByte((uint8)Dst_Addr);  
	SPI_CS_DISABLE;                            //取消片选     	      
	SpiFlashWaitBusy();   				   //等待擦除完成

	return 1;
}  

//等待空闲
void SpiFlashWaitBusy(void)   
{   
	while((SpiFlashReadSR()&0x01)==0x01);   // 等待BUSY位清空
}  

//进入掉电模式
uint8 ExteFlashPowerDown(void)   
{ 
  	SPI_CS_ENABLE;                            //使能器件   
    SPI1_ReadWriteByte(W25X_PowerDown);        //发送掉电命令  
	SPI_CS_DISABLE;                            //取消片选     	      
    spidelay_ms(1);                               //等待TPD  

	return 1;
}   

//唤醒
uint8 ExteFlashWakeUp(void)   
{  
	SPI_CS_ENABLE;                            //使能器件   
	SPI1_ReadWriteByte(W25X_ReleasePowerDown);   //  send W25X_PowerDown command 0xAB    
	SPI_CS_DISABLE;                            //取消片选     	      
	spidelay_ms(1);                               //等待TRES1

	return 1;
}   

