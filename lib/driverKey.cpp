/*
 * @brief 按键驱动
 *
 * @note 
 * 
 * @date:20161222
 */
#include "driverKey.h"

int DriverKey::keyFd  = -1;
int DriverKey::change = 0;                                                      ///应考虑多模块调用情况
int DriverKey::keyStat[KeyCodeMaxNum] = {KeyValueOff, KeyValueOff, KeyValueOff};
map<int, int> DriverKey::kernelKeyMapToCode;                                    ///内核键码映射

/**
 * @brief 若keyFd==-1则启动线程
 */
DriverKey::DriverKey()
{
    if (keyFd == -1) {
        kernelKeyMapToCode.insert(std::pair<int, int>(KEY_DOWN, KeyCodeDown));
        kernelKeyMapToCode.insert(std::pair<int, int>(KEY_ENTER, KeyCodeEnter));
        kernelKeyMapToCode.insert(std::pair<int, int>(KEY_DELETE, KeyCodeEsc));
        run();
    }
}

/**
 * @brief 根据键码获取键值
 * @in    code 键码
 * @return 键值
 */
DR_KEY_VALUE DriverKey::getKeyValue(DR_KEY_CODE code)
{
    
    return (DR_KEY_VALUE)(keyStat[code]);
}

/**
 * @brief 获取发生变化的键码和键值并清零变位标志(应考虑多模块调用问题)
 * @in    
 * @return 键值
 */
DR_KEY_STAT DriverKey::getChangedKeyStat()
{
    DR_KEY_STAT stat = {KeyCodeNone, KeyValueOff};
    
    for (int i=0; i<KeyCodeMaxNum; i++) {
        if ((change >> i) & 0x01) {
            change &= ~(1 << i);

            stat.code  = (DR_KEY_CODE)(KeyCodeDown + i);
            stat.value = (DR_KEY_VALUE)keyStat[i];

            return stat;
        }
    }

    return stat;
}

/**
 * @brief 启动按键读取线程
 * @return 
 */
void DriverKey::run()
{
    pthread_t pd = -1;
    
    if (pthread_create(&pd, NULL, readKeyStat, static_cast<void *>(this)) < 0) {
        printfs(LOG_ERROR, "创建按键读取线程失败:%d", errno);
        return ;
    }
    else
        printfs(LOG_INFO, "创建按键读取线程成功!");

    pthread_detach(pd);
}

/**
 * @brief 线程服务函数
 * @return 
 */
void * DriverKey::readKeyStat(void *arg)
{
    struct input_event ev_key;        

    /*以阻塞方式打开输入设备*/
    if ((keyFd = open(DEV_KEY, O_RDWR)) == -1) {
        printfs(LOG_ERROR, "Open %s failed %d", DEV_KEY, errno);
        return NULL;
    }

    while (1) {        
        read(keyFd, &ev_key, sizeof(struct input_event));                
        if (EV_KEY == ev_key.type) {            
            map<int, int>::iterator it = kernelKeyMapToCode.find(ev_key.code);
            if (it != kernelKeyMapToCode.end()) {
                keyStat[it->second] = ev_key.value;
                change |= 0x01 << it->second;
            }

            printfs(LOG_DEBUG, "time:%ld s, %d us \t type:%d  code:%d  value:%d\n", 
                    ev_key.time.tv_sec, ev_key.time.tv_usec,
                    ev_key.type, ev_key.code, ev_key.value);
        }    
    }    

    close(keyFd);
    keyFd = -1;
    return NULL;
}

