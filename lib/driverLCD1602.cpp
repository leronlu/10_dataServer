/*
 * @brief 液晶驱动
 *
 * @note 
 * @author Lijiazhen
 * @date   20161222
 *
 */
#include "driverLCD1602.h"

/*IO*/
#define LCD_CONTROL_PORT    GPIO_PORT_0
#define LCD_DATA_PORT       GPIO_PORT_1
#define LCD_CTR_RS          GPIO_PIN_3
#define LCD_CTR_RW          GPIO_PIN_4
#define LCD_CTR_EN          GPIO_PIN_5
#define LCD_CTR_BackLight   GPIO_PIN_6

/*行地址*/
#define LCD_ADDR_LINE1      0x00
#define LCD_ADDR_LINE2      0x40

/*LCD1602命令表*/
typedef enum
{
    LCD_CMD_ClearDisplay = 0x001,                                               ///清显示
    LCD_CMD_CursorReset  = 0x002,                                               ///光标复位
    LCD_CMD_SetInputMode = 0x004,                                               ///设置输入模式
    LCD_CMD_DisplaySwitch= 0x008,                                               ///显示开/关控制
    LCD_CMD_MoveCursor   = 0x010,                                               ///移动光标或字符
    LCD_CMD_FuncSet      = 0x020,                                               ///功能设置
    LCD_CMD_CharCreatorAddr = 0x040,                                            ///字符发生器地址
    LCD_CMD_DispDataAddr    = 0x080,                                            ///字符显示地址
    LCD_CMD_ReadBusy        = 0x80                                              ///读取忙标志位
    
}LCD_CMD;

/*LCD_CMD_SetInputMode*/
typedef enum
{
    TypeSetInputModeCurDirRight = 0x02,                                         ///输入时光标的移动方向为右
    TypeSetInputModeCurDirLeft  = 0x00,                                         ///输入时光标的移动方向为左
    TypeSetInputModeMoveChar  = 0x01,                                           ///输入时显示字符可移动
    TypeSetInputModeHoldChar  = 0x00                                            ///输入时显示字符不可移动
}LCD_CMD_TYPE_SetInputMode;
/*LCD_CMD_DisplaySwitch*/
typedef enum
{
    TypeDisplaySwitchOn     = 0x04,                                             ///开显示
    TypeDisplaySwitchOff    = 0x00,                                             ///关显示
    TypeDisplaySwitchCursorOn = 0x02,                                           ///显示光标
    TypeDisplaySwitchCursorOff= 0x00,                                           ///不显示光标
    TypeDisplaySwitchCursorShine= 0x01,                                         ///光标闪烁
    TypeDisplaySwitchCursorHold = 0x00                                          ///光标不闪烁
}LCD_CMD_TYPE_DisplaySwitch;
/*LCD_CMD_MoveCursor*/
typedef enum
{
    TypeMoveCursorMoveChar  = 0x08,                                             ///移动显示字符
    TypeMoveCursorMoveCursor= 0x00,                                             ///移动光标
    TypeMoveCursorDirRight  = 0x04,                                             ///显示字符向右移动
    TypeMoveCursorDirLeft   = 0x00                                              ///显示字符向左移动
}LCD_CMD_TYPE_MoveCursor;
/*LCD_CMD_FuncSet*/
typedef enum
{
    TypeFuncSetBusLine_4    = 0x00,                                             ///4总线
    TypeFuncSetBusLine_8    = 0x10,                                             ///8总线
    TypeFuncSetDoubleLine   = 0x08,                                             ///双行显示
    TypeFuncSetSingleLine   = 0x00,                                             ///单行显示
    TypeFuncSetLatticeChar_10  = 0x04,                                          ///显示5*10点阵字符
    TypeFuncSetLatticeChar_7   = 0x00                                           ///显示5*7点阵支付
}LCD_CMD_TYPE_FuncSet;

/**
 * @brief 设置光标位置
 * @in x
 * @in y
 */
void DriverLCD1602::setCusor(int x, int y)
{
    m_px = x;
    m_py = y;
    setPoint(x, y);
}

/**
 * @brief 清除显示
 */
void DriverLCD1602::clearScreen()
{
    writeCmd(LCD_CMD_ClearDisplay, true);
}

/**
 * @brief 开显示
 */    
void DriverLCD1602::displayOn()
{
    writeCmd(LCD_CMD_DisplaySwitch |
                TypeDisplaySwitchOn, true);
}

/**
 * @brief 关显示
 */    
void DriverLCD1602::displayOff()
{
    writeCmd(LCD_CMD_DisplaySwitch |
                TypeDisplaySwitchOff, true);
}

/**
 * @brief 开背光
 */
void DriverLCD1602::backlightOn()
{
    setPins(LCD_CONTROL_PORT, LCD_CTR_BackLight);
}

/**
 * @brief 关背光
 */    
void DriverLCD1602::backlightOff()
{
    resetPins(LCD_CONTROL_PORT, LCD_CTR_BackLight);
}

/**
 * @brief 显示字符,若x==-1||y==-1则以光标定位为坐标
 * @in val 字符
 * @in x   x坐标
 * @in y   y坐标
 */ 
void DriverLCD1602::displayChar(unsigned char val, int x, int y)
{
    x = (x==-1) ? m_px : x;
    y = (y==-1) ? m_py : y;

    setPoint(x, y);

    writeVal(val);
}

/**
 * @brief 显示字符串,若x==-1||y==-1则以光标定位为坐标
 * @in str 字符串
 * @in x   x坐标
 * @in y   y坐标
 */ 
void DriverLCD1602::displayStr(const char *str, int x, int y)
{
    int cnt = 0;
    
    x = (x==-1) ? m_px : x;
    y = (y==-1) ? m_py : y;

    setPoint(x, y);

    while (*str != '\0' && (cnt++ < 16)) {
        writeVal(*str);
        str ++;
    }
}
    
/**
 * @brief 初始化
 * @return 
 */    
void DriverLCD1602::initLCD()
{
    #if 1
    writePort(LCD_CONTROL_PORT, 0);
    writePort(LCD_DATA_PORT, 0 | LCD_CTR_BackLight);
    /*配置功能8位格式,双行显示 5x7的点阵字符
     * 多次初始化
     */
    writeCmd(LCD_CMD_FuncSet | 
                TypeFuncSetBusLine_8 | 
                TypeFuncSetDoubleLine | 
                TypeFuncSetLatticeChar_7);

    writeCmd(LCD_CMD_FuncSet | 
                TypeFuncSetBusLine_8 | 
                TypeFuncSetDoubleLine | 
                TypeFuncSetLatticeChar_7,
             true);

    writeCmd(LCD_CMD_FuncSet | 
                TypeFuncSetBusLine_8 | 
                TypeFuncSetDoubleLine | 
                TypeFuncSetLatticeChar_7,
             true);

    writeCmd(LCD_CMD_FuncSet | 
                TypeFuncSetBusLine_8 | 
                TypeFuncSetDoubleLine | 
                TypeFuncSetLatticeChar_7,
             true);

    /*清显示*/
    clearScreen();
    
    /*开启显示,无光标,不闪烁*/
    writeCmd(LCD_CMD_DisplaySwitch |
                TypeDisplaySwitchOn |
                TypeDisplaySwitchCursorOff |
                TypeDisplaySwitchCursorHold);
    
    /*配置输入模式,增量,不移位*/
    writeCmd(LCD_CMD_SetInputMode |
                TypeSetInputModeCurDirRight |
                TypeSetInputModeHoldChar);
    #endif
}

/**
 * @brief 延时ms,延时10ms以下准确度不能保证
 * @return 
 */   
void DriverLCD1602::delayMs(int ms)
{
    usleep(1000 * ms);
}

/**
 * @brief 延时us,延时1us不准确
 * @return 
 */    
void DriverLCD1602::delayUs(int us)
{
    usleep(us);
}

/**
 * @brief 写数据
 * @in    val 值
 */    
void DriverLCD1602::writeVal(int val)
{   
    readStatusBusy();
    writePort(LCD_DATA_PORT, val);
    #if 1
    int iodata = readPortCache(LCD_CONTROL_PORT);
    iodata |= LCD_CTR_RS;
    iodata &= ~(LCD_CTR_RW|LCD_CTR_EN);
    writePort(LCD_CONTROL_PORT, iodata);
    #endif
    #if 0
    setPins(LCD_CONTROL_PORT, LCD_CTR_RS);
    resetPins(LCD_CONTROL_PORT, LCD_CTR_RW);
    resetPins(LCD_CONTROL_PORT, LCD_CTR_EN);
    setPins(LCD_CONTROL_PORT, LCD_CTR_EN);
    //delayUs(100);
    #endif
    setPins(LCD_CONTROL_PORT, LCD_CTR_EN);
    resetPins(LCD_CONTROL_PORT, LCD_CTR_EN);
}

/**
 * @brief 设置光标位置
 * @return 
 */ 
void DriverLCD1602::setPoint(int x, int y)
{
    if (y == 1)
        x |= LCD_ADDR_LINE2;
    else
        x |= LCD_ADDR_LINE1;

    writeCmd(LCD_CMD_DispDataAddr |
                (x), true);
}

/**
 * @brief 写命令
 * @in cmd 命令值
 * @int checkBusy 是否检测忙
 * @return 
 */ 
void DriverLCD1602::writeCmd(unsigned char cmd, bool checkBusy)
{
    if (checkBusy) {
        readStatusBusy();
    }
    writePort(LCD_DATA_PORT, cmd);
    #if 1
    int iodata = readPortCache(LCD_CONTROL_PORT);
    iodata &= ~(LCD_CTR_RW|LCD_CTR_EN|LCD_CTR_RS);
    writePort(LCD_CONTROL_PORT, iodata);
    #endif
    #if 0
    resetPins(LCD_CONTROL_PORT, LCD_CTR_RS|LCD_CTR_RW);
    resetPins(LCD_CONTROL_PORT, LCD_CTR_EN);
    #endif
    setPins(LCD_CONTROL_PORT, LCD_CTR_EN);
    resetPins(LCD_CONTROL_PORT, LCD_CTR_EN);
 //   setPins(LCD_CONTROL_PORT, LCD_CTR_EN|LCD_CTR_BackLight);
}

/**
 * @brief 读取忙状态
 * @return 
 */ 
unsigned char DriverLCD1602::readStatusBusy()
{
#if 0
    unsigned char ret = 1;
    resetDirs(LCD_DATA_PORT, 0xFF);
    while (ret) {
    //writePort(LCD_DATA_PORT, 0xFF); 
         
        resetPins(LCD_CONTROL_PORT, LCD_CTR_RS);
        setPins(LCD_CONTROL_PORT, LCD_CTR_RW);

        resetPins(LCD_CONTROL_PORT, LCD_CTR_EN);
        setPins(LCD_CONTROL_PORT, LCD_CTR_EN);

        delayMs(5); //检测忙信号   
        ret = readPort(LCD_DATA_PORT) & LCD_CMD_ReadBusy;
        
        delayMs(5); //检测忙信号
        resetPins(LCD_CONTROL_PORT, LCD_CTR_EN);
        resetPins(LCD_CONTROL_PORT, LCD_CTR_RW);
        
    }
    setDirs(LCD_DATA_PORT, 0xFF);
#else
    //delayMs(10);
#endif
    return 0;
}


