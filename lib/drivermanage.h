/******************************************************************************
@ Copy Right		: 2011 Beijing Togest Automation System Equipment Co.,Lt 
@ File Name 		: drivermanage.h
@ Author     		: He ZongNan
@ Version    		: V1.0.0
@ Last Modify		: 12/15/2011
@ Description		: this file is about some driver app service 
-------------------------------------------------------------------------------
@ Modified History	:   
*******************************************************************************/
#ifndef DRIVERMANAGE_H
#define DRIVERMANAGE_H

#include "log_manage.h"
extern "C"
{
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
}
/**************************************************************************
@ Description:      定义服务使用相关类型与宏
***************************************************************************/
#define DEV_DIP 	"/dev/dips"					//拨码开关设备
#define DEV_TEM 	"/dev/TEM0"					//温度传感器设备
#ifdef OK6410
#define DEV_LED 	"/dev/TICK0"					//LED 灯设备
#else
#define DEV_LED 	"/dev/led"					//LED 灯设备
#endif
#define DEV_WTD 	"/dev/watchdog"				//系统看门狗设备
#define RUN_LED_ON              0x00
#define RUN_LED_OFF             0x01
/**************************************************************************
@ Description:      定义服务使用相关类型与宏
***************************************************************************/								
typedef enum
{
	TEMP_TYPE,
	LED_TYPE,
	DIP_TYPE,
	WTD_TYPE
}ENUM_Driver_Type;

class DriverService
{
public:
    DriverService() 
    {
        m_tempFd = -1;
        m_ledFd = -1;
        m_dipFd = -1;
        m_wtdFd = -1;
    }
    ~DriverService()
    {
        if (m_tempFd >0)
            close(m_tempFd);
    }
	void OpenDevice(ENUM_Driver_Type driverType);
	void EnableWtd();
	void FeedWtd();
	void EnableLed();
	void RunLed(int type);
	unsigned char ReadDip();
	float ReadTemp();
	void IoctlLedWtd();
    void initRdTemp();
    
private:
	int m_tempFd;
	int m_ledFd;
	int m_dipFd;
	int m_wtdFd;
};
#endif
