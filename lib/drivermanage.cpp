/******************************************************************************
@ Copy Right		: 2011 Beijing Togest Automation System Equipment Co.,Lt 
@ File Name 		: drivermanage.cpp
@ Author     		: He ZongNan
@ Version    		: V1.0.0
@ Last Modify		: 12/15/2011
@ Description		: This File include some driver app service
-------------------------------------------------------------------------------
@ Modified History	:   
*******************************************************************************/
#include "drivermanage.h"
/*******************************************************************************
@ Function Name  : OpenDevice
@ Description    	: open deriver device 
@ Input           	: ENUM_Driver_Type driverType
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
void DriverService::OpenDevice(ENUM_Driver_Type driverType)
{
	switch(driverType)
	{
		case DIP_TYPE:
			m_dipFd = open(DEV_DIP, O_RDWR);
			break;
		case TEMP_TYPE:
			m_tempFd = open(DEV_TEM, O_RDWR | O_NDELAY | O_NOCTTY);
			break;
		case LED_TYPE:
			m_ledFd = open(DEV_LED, O_RDWR);
			break;
		case WTD_TYPE:
			m_wtdFd = open(DEV_WTD, O_RDWR);
			break;
		default:
			break;
	}
}
/*******************************************************************************
@ Function Name  : EnableWtd
@ Description    	: open deriver device 
@ Input           	: None;
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
void DriverService::EnableWtd()
{
	OpenDevice(WTD_TYPE);
	if (m_wtdFd < 0)
	{
		printfs(LOG_ERROR,  "DriverService::EnableWtd error:%s", strerror(errno));
	}
}
/*******************************************************************************
@ Function Name  : EnableWtd
@ Description    	: open deriver device 
@ Input           	: None;
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
void DriverService::FeedWtd()
{
	ioctl(m_wtdFd, WDIOC_KEEPALIVE, 0);
}
/*******************************************************************************
@ Function Name  : EnableLed
@ Description    	: open deriver device 
@ Input           	: None;
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
void DriverService::EnableLed()
{
	OpenDevice(LED_TYPE);
	if (m_ledFd < 0)
	{
		printfs(LOG_ERROR,  "DriverService::EnableLed error:%s", strerror(errno));
	}
}
/*******************************************************************************
@ Function Name  : IoctlLed
@ Description    	: open deriver device 
@ Input           	: int type;
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
void DriverService::RunLed(int type)
{
#ifdef OK6410
	write(m_ledFd, 0 , type);
#else
        ioctl(m_ledFd, type, 0);
#endif    
}
/*******************************************************************************
@ Function Name  : ReadDip
@ Description    	: None; 
@ Input           	: None;
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
unsigned char DriverService::ReadDip()
{
	unsigned char data = 0;
	OpenDevice(DIP_TYPE);

	if (m_dipFd < 0)
	{
		printfs(LOG_ERROR,  "DriverService::ReadDip error:%s\n", strerror(errno));
	}
	read(m_dipFd, &data, 1);
	close(m_dipFd);
	return ~(data);
}

/*******************************************************************************
@ Function Name  : initRdTemp
@ Description    	: 初始化温度读取
@ Input           	: None;
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
void DriverService::initRdTemp()
{
    OpenDevice(TEMP_TYPE);
    if(m_tempFd < 0)
	{
		printfs(LOG_ERROR, "DriverService::ReadTemp error:%d\n", errno);
	}
}

/*******************************************************************************
@ Function Name  : ReadTemp
@ Description    	: read tempeture value 
@ Input           	: None;
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
float DriverService::ReadTemp()
{
	unsigned char buf[2];
	//unsigned char ret = 0;
	float result,Tem;

	//ret = read(m_tempFd, buf, 1);
	read(m_tempFd, buf, 1);
	result = (float)buf[0];
	result /= 16;
	result += ((float)buf[1] * 16);
	Tem = result*100;					//温度变比为0.01

	return Tem;
}
/*******************************************************************************
@ Function Name  : IoctlLedWtd
@ Description    	: read tempeture value 
@ Input           	: None;
@ Output         	: None;
@ Return         	: None;
*******************************************************************************/
void DriverService::IoctlLedWtd()
{
	static int type = 0;
	FeedWtd();
	if (RUN_LED_ON == type)
	{
		RunLed(RUN_LED_ON);
		type = RUN_LED_OFF;
	}
	else 
	{
		RunLed(RUN_LED_OFF);
		type = RUN_LED_ON;
	}
}
