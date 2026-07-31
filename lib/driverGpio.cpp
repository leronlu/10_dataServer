/*
 * @brief GPIO驱动
 *
 * @note 
 * @author Lijiazhen
 * @date   20161222
 *
 */
#include "driverGpio.h"

#define GPIO_MSG_NUM        2                                                   ///一条消息
#define GPIO_MSG_DATA_NUM   3                                                   ///消息数据内容大小
#define GPIO_I2C_SALVE_ADDR 0x20                                                ///I2C从设备地址

int DriverGPIO::gpioFd = -1;                                                    ///设备描述符
struct i2c_rdwr_ioctl_data DriverGPIO::ioMsg = {NULL, GPIO_MSG_NUM};            ///全局发送缓存区
int DriverGPIO::ioData[GPIO_PORT_NUM] = {0, 0};                                 ///io各个管脚数据
int DriverGPIO::ioDir[GPIO_PORT_NUM] = {0x07, 0x00};                            ///io各个管脚数据

/*I2C 读写标志*/
enum DEV_GPIO_FLAGS
{
    GPIO_FLAG_WR = 0,
    GPIO_FLAG_RD
};

/*I2C Expand IO chip register*/
enum DEV_GPIO_REG
{
	GPIO_REG_INPUT_0	    = 0,
	GPIO_REG_INPUT_1	    = 1,
	GPIO_REG_OUTPUT_0	    = 2,
	GPIO_REG_OUTPUT_1	    = 3,
	GPIO_REG_INVERT_0	    = 4,
	GPIO_REG_INVERT_1	    = 5,
	GPIO_REG_DIRECTION_0	= 6,
	GPIO_REG_DIRECTION_1	= 7,
};

#define _DEBUG_GPIO_ 0
#if _DEBUG_GPIO_
#define debugPrintDir() printfs(LOG_ERROR, "write dir port: dir:%#02lx dir[0]=%#02lx dir[1]=%#02lx", \
                 value, ioDir[0], ioDir[1])
#define debugPrintVal(str) printfs(LOG_ERROR, "%s port: value:%#02x dat[0]=%#02x dat[1]=%#02x", \
                str,  value, ioData[0], ioData[1])                
#else
#define debugPrintDir() {}
#define debugPrintVal(str) {}
#endif

/**
 * @brief 置高多个管脚
 * @in port 端口
 * @in pins 管脚序号GPIO_PIN_0|GPIO_PIN_1|...
 */
void DriverGPIO::setDirs(DR_GPIO_PORT port, int pins)
{
    int value = ioDir[port] & (~pins);
    
    if (writeDev(port + GPIO_REG_DIRECTION_0, value))
        ioDir[port] = value;
    debugPrintDir();
}
/**
 * @brief 置高多个管脚
 * @in port 端口
 * @in pins 管脚序号GPIO_PIN_0|GPIO_PIN_1|...
 */
void DriverGPIO::resetDirs(DR_GPIO_PORT port, int pins)
{
    int value = ioDir[port] | pins;

    if (writeDev(port + GPIO_REG_DIRECTION_0, value))
        ioDir[port] = value;
    debugPrintDir();
}

/**
 * @brief 置高多个管脚
 * @in port 端口
 * @in pins 管脚序号GPIO_PIN_0|GPIO_PIN_1|...
 */
void DriverGPIO::setPins(DR_GPIO_PORT port, int pins)
{
    int value = ioData[port] | pins;

    writePort(port, value);
}

/**
 * @brief 清零多个管脚
 * @in port 端口
 * @in pins 管脚序号GPIO_PIN_0|GPIO_PIN_1|...
 */
void DriverGPIO::resetPins(DR_GPIO_PORT port, int pins)
{
    int value = ioData[port] & ~pins;

    writePort(port, value);
}

/**
 * @brief 切换管脚值
 * @in port 端口
 * @in pins 管脚序号GPIO_PIN_0|GPIO_PIN_1|...
 */
void DriverGPIO::togglePins(DR_GPIO_PORT port, int pins)
{
    int value = ioData[port] & pins;
    int rval  = (ioData[port] & ~pins) | (~value);

    writePort(port, rval);
}

/**
 * @brief 设置端口管脚的方向
 * @in port 端口
 * @in dir  端口所有管脚的方向值
 */
void DriverGPIO::setPinsDirect(DR_GPIO_PORT port, int dir)
{
    writeDev(port + GPIO_REG_DIRECTION_0, dir);
}

/**
 * @brief 写端口的多个管脚
 * @in port  端口
 * @in pins  端口管脚
 * @in value 管脚值
 */
void DriverGPIO::writePins(DR_GPIO_PORT port, int pins, int value)
{
    int val = (ioData[port] & ~pins) | value;

    writePort(port, val);
}

/**
 * @brief 写端口数据
 * @in port 端口
 * @in value 数据
 */
int DriverGPIO::writePort(DR_GPIO_PORT port, int value)
{
    int ret = writeDev(port + GPIO_REG_OUTPUT_0, value);

    if (ret > 0)
        ioData[port] = value;
    debugPrintVal("write");
    return ret;
}

/**
 * @brief 读取端口数据
 * @in port 端口
 * @return 数据
 */    
int DriverGPIO::readPort(DR_GPIO_PORT port)
{
    int value = readDev(port + GPIO_REG_INPUT_0);

    ioData[port] = value;
    debugPrintVal("read");
    return value;
}

/**
 * @brief 写端口数据
 * @in port0Val 端口0数据
 * @in port1Val 端口1数据
 * @return 数据
 */    
int DriverGPIO::writeValues(int port0Val, int port1Val)
{
    if (!ioMsg.msgs || !ioMsg.msgs[0].buf) {
        printfs(LOG_ERROR, "msgs == NULL!");
        return -1;
    }

    ioMsg.nmsgs         = 1;
    ioMsg.msgs[0].len   = 3;
    ioMsg.msgs[0].addr  = GPIO_I2C_SALVE_ADDR;
    ioMsg.msgs[0].flags = GPIO_FLAG_WR;
    ioMsg.msgs[0].buf[0]= GPIO_REG_OUTPUT_0;
    ioMsg.msgs[0].buf[1]= port0Val;
    ioMsg.msgs[0].buf[2]= port1Val;

    if (ioctl(gpioFd, I2C_RDWR, (unsigned long)(&ioMsg)) < 0) {
        printfs(LOG_ERROR, "WriteDev err:%d", errno);
        return -1;
    }

    ioData[GPIO_PORT_0] = port0Val;
    ioData[GPIO_PORT_1] = port1Val;

    return GPIO_MSG_DATA_NUM;
}

/**
 * @brief 初始化函数
 * @return 
 */    
void DriverGPIO::init()
{
    if (gpioFd == -1) {
        gpioFd = open(DEV_GPIO, O_RDWR);
        if (gpioFd == -1) {
            printfs(LOG_ERROR, "Open %s error:%d", DEV_GPIO, errno);
            return ;
        }
        ioMsg.msgs = (struct i2c_msg *)malloc(ioMsg.nmsgs * sizeof(struct i2c_msg));
        for (size_t i=0; i<ioMsg.nmsgs; i++) {
            ioMsg.msgs[i].buf = NULL;
            ioMsg.msgs[i].buf = (unsigned char*)malloc(GPIO_MSG_DATA_NUM);
        }
        ioctl(gpioFd, I2C_TIMEOUT, 1);                                          ///I2C超时时间
        ioctl(gpioFd, I2C_RETRIES, 2);                                          ///重发次数

        printfs(LOG_INFO, "Open %s successed!", DEV_GPIO);
    }
}

/**
 * @brief 向设备寄存器写入数据
 * @in reg 寄存器
 * @in value 值
 * @return 
 */ 
int DriverGPIO::writeDev(int reg, int value)
{
    if (!ioMsg.msgs || !ioMsg.msgs[0].buf) {
        printfs(LOG_ERROR, "msgs == NULL!");
        return -1;
    }

    ioMsg.nmsgs         = 1;
    ioMsg.msgs[0].len   = 2;
    ioMsg.msgs[0].addr  = GPIO_I2C_SALVE_ADDR;
    ioMsg.msgs[0].flags = GPIO_FLAG_WR;
    ioMsg.msgs[0].buf[0]= reg;
    ioMsg.msgs[0].buf[1]= value;

    if (ioctl(gpioFd, I2C_RDWR, (unsigned long)(&ioMsg)) < 0) {
        printfs(LOG_ERROR, "WriteDev err:%d", errno);
        return -1;
    }

    return GPIO_MSG_DATA_NUM;
}

/**
 * @brief 读取设备寄存器
 * @in reg 寄存器
 * @return 
 */ 
int DriverGPIO::readDev(int reg)
{
    if (!ioMsg.msgs || !ioMsg.msgs[0].buf || !ioMsg.msgs[1].buf) {
        printfs(LOG_ERROR, "msgs == NULL!");
        return -1;
    }

    ioMsg.nmsgs         = 2;
    ioMsg.msgs[0].len   = 1;
    ioMsg.msgs[0].addr  = GPIO_I2C_SALVE_ADDR;
    ioMsg.msgs[0].flags = GPIO_FLAG_WR;
    ioMsg.msgs[0].buf[0]= reg;
    ioMsg.msgs[1].len   = 1;
    ioMsg.msgs[1].addr  = GPIO_I2C_SALVE_ADDR;
    ioMsg.msgs[1].flags = GPIO_FLAG_RD;
    ioMsg.msgs[1].buf[0]= 0;

    if (ioctl(gpioFd, I2C_RDWR, (unsigned long)(&ioMsg)) < 0) {
        printfs(LOG_ERROR, "WriteDev err:%d", errno);
        return -1;
    }

    return ioMsg.msgs[1].buf[0];
}

