/*
 * @brief 按键驱动
 *
 * @note 
 * 
 *
 */
#ifndef DRIVERKEY_H
#define DRIVERKEY_H

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
#include <linux/input.h>
#include <fcntl.h>
}
/**
 * @brief	
 */
 #ifdef OK6410
#define DEV_KEY 	"/dev/input/event1"                         				///按键输入驱动
#else
#define DEV_KEY         "/dev/input/event0"
#endif

/*按键键码*/
typedef enum
{
    KeyCodeNone = -1,
    KeyCodeDown = 0,                                                            ///下方向键
    KeyCodeEnter,                                                               ///确认键
    KeyCodeEsc,                                                                 ///撤销键
    KeyCodeMaxNum
}DR_KEY_CODE;
/*按键状态值*/
typedef enum
{
    KeyValueOff,                                                                ///按键释放
    KeyValueOn                                                                  ///按键按下
}DR_KEY_VALUE;
/*按键值*/
struct DR_KEY_STAT
{
    DR_KEY_CODE     code;
    DR_KEY_VALUE    value;
};

class DriverKey
{
public:
    DriverKey();
    ~DriverKey()
    {
    }
    bool    checkKeyChanged() {return change != 0;}                             ///应考虑多个模块调用的情况
    DR_KEY_VALUE    getKeyValue(DR_KEY_CODE code);
    DR_KEY_STAT     getChangedKeyStat();
private:    
	void    run();
    static void *readKeyStat(void *arg);
    
private:
	static int  keyFd;
    static int  change;                                                         ///应考虑多模块调用情况
    static int  keyStat[KeyCodeMaxNum];
    static std::map<int, int> kernelKeyMapToCode;                                    ///内核键码映射
};
#endif

