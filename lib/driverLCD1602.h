/*
 * @brief 液晶驱动
 *
 * @note 
 * @author Lijiazhen
 * @date   20161222
 *
 */
#ifndef DRIVELCD1602_H
#define DRIVELCD1602_H

#include "log_manage.h"
#include "driverGpio.h"

#define CHAR_PER_LINE   16                                                      ///每一行最大显示字符

class DriverLCD1602 : public DriverGPIO
{
public:
    DriverLCD1602() 
    {
        initLCD();
    }
    ~DriverLCD1602()
    {
    }

    void    setCusor(int x, int y);
    void    clearScreen();
    void    displayOn();
    void    displayOff();
    void    backlightOn();
    void    backlightOff();
    void    displayChar(unsigned char val, int x=-1, int y=-1);
    void    displayStr(const char *str, int x=-1, int y=-1);
    
private:    
	void    initLCD();
    void    delayMs(int ms);                                                    ///延时ms
    void    delayUs(int us);                                                    ///延时us
    void    writeVal(int val);
    void    setPoint(int x, int y);
    void    writeCmd(unsigned char cmd, bool checkBusy = false);
    unsigned char    readStatusBusy();
    
private:
	int     m_px;                                                                 ///指针x坐标
    int     m_py;                                                                 ///指针y坐标
};
#endif

