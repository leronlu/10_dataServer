/*
 * @brief GPIO驱动
 *
 * @note 
 * @author Lijiazhen
 * @date   20161222
 *
 */
#ifndef DRIVERGPIO_H
#define DRIVERGPIO_H

#include "log_manage.h"
extern "C"
{
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
}
#ifdef OK6410
#define DEV_GPIO 	"/dev/i2c/0"					                            ///GPIO设备
#else
#define DEV_GPIO	"/dev/i2c-2"
#endif
/*定义GPIO*/								
typedef enum
{
	GPIO_PORT_0,
	GPIO_PORT_1,
	GPIO_PORT_NUM
}DR_GPIO_PORT;
typedef enum
{
    GPIO_PIN_0 = 0x01,
    GPIO_PIN_1 = 0x02,
    GPIO_PIN_2 = 0x04,
    GPIO_PIN_3 = 0x08,
    GPIO_PIN_4 = 0x10,
    GPIO_PIN_5 = 0x20,
    GPIO_PIN_6 = 0x40,
    GPIO_PIN_7 = 0x80
}DR_GPIO_PIN;

/*方向*/
typedef enum
{
    GPIO_DIR_OUT,
    GPIO_DIR_IN    
}DR_GPIO_DIR;

/*应考虑多个模块调用,应添加互斥锁*/
class DriverGPIO
{
public:
    DriverGPIO() 
    {
        if (gpioFd == -1)
            init();
    }
    ~DriverGPIO()
    {
    }
    void setDirs(DR_GPIO_PORT port, int pins);
    void resetDirs(DR_GPIO_PORT port, int pins);
    void setPins(DR_GPIO_PORT port, int pins);                                  ///设置管脚输出高
    void togglePins(DR_GPIO_PORT port, int pins);                               ///
    void resetPins(DR_GPIO_PORT port, int pins);                                ///设置管脚输出低
    void setPinsDirect(DR_GPIO_PORT port, int dir);                             ///设置端口管脚方向
    void writePins(DR_GPIO_PORT port, int pins, int value);                     ///写端口的多个管脚
    int  writePort(DR_GPIO_PORT port, int value);                               ///写端口
    int  readPort(DR_GPIO_PORT port);                                           ///读端口
    int  readPortCache(DR_GPIO_PORT port) {                                      ///读端口缓存值
        return ioData[port];
    }
    int writeValues(int port0Val, int port1Val);
    
private:    
	static void init();
    static int writeDev(int reg, int value);                                    ///写设备
    static int readDev(int reg);                                                ///读设备
    
private:
	static int gpioFd;                                                          ///设备描述符
	static struct i2c_rdwr_ioctl_data   ioMsg;                                  ///全局发送缓存区
    static int ioData[GPIO_PORT_NUM];                                           ///io各个管脚数据
    static int ioDir[GPIO_PORT_NUM];                                            ///io各个管脚方向
};
#endif
