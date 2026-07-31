#include <unistd.h>
using namespace std;
#include "jestonSysMsg.h"

#ifdef Q_OS_LINUX
JestonSysMsg::JestonSysMsg()
{

}

JestonSysMsg::~JestonSysMsg()
{

}

uint16_t JestonSysMsg::getSDcap(void) 
{
    // 获取SD卡挂载路径
    //std::string sdCardPath = getSDCardPath();
	//SDMsg sdMsg;
	string sdCardPath = getSDCardPath();
    if (sdCardPath.empty()) 
	{
        printfs(LOG_WARNING, "未找到挂载的SD卡\n");
        return 0;
    }

    struct statvfs stats;
    if (statvfs(sdCardPath.c_str(), &stats) == 0) 
	{
        // 获取总容量、已用空间、可用空间
        //unsigned long totalSize = stats.f_blocks * stats.f_frsize;
        //unsigned long usedSpace = (stats.f_blocks - stats.f_bfree) * stats.f_frsize;
        float availableSpace = (stats.f_bavail * stats.f_frsize);

		return (uint16_t)(availableSpace * 100  / (1024 * 1024 * 1024));
    } 
	else 
	{
        printfs(LOG_WARNING,"获取 SD 卡信息失败");
        return 0;
    }
}

SdCapMsg JestonSysMsg::getSDCapMsg(void) {
    SdCapMsg msg;
    return msg;
}

uint16_t JestonSysMsg::getCPUTemperature(string tempFilePath) 
{
    std::ifstream tempFile(tempFilePath);
    if (!tempFile.is_open()) 
	{
        printfs(LOG_ERROR, "无法打开文件：%s" ,tempFilePath);
        return 0;
    }

    int temperature;
    tempFile >> temperature;
    tempFile.close();

    return (temperature/10);
}

bool JestonSysMsg::setBrightness(string pwmFilePath, int brightness) 
{
    // 以超级用户权限运行
    if (geteuid() != 0) 
	{
        printfs(LOG_WARNING,"用户无权限运行设置亮度" );
        return false;
    }

    // 检查亮度值范围
    if (brightness < 0 || brightness > 100) 
	{
        printfs(LOG_WARNING, "值无效,亮度值应在0-100之间" );
        return false;
    }
	brightness = (100-brightness) * 83333.33;
	std::ofstream pwmFile(pwmFilePath);
    if (!pwmFile.is_open()) {
        printfs(LOG_WARNING, "无法打开文件：%s" , pwmFilePath);
        return false;
    }

    pwmFile << brightness;
    pwmFile.close();

    return true;
}

string JestonSysMsg::getSDCardPath(void) 
{
    return "/media/name";
}

string JestonSysMsg::getStoragePath(void) 
{
    return "";
}

bool JestonSysMsg::isSDCardInserted()
{
    return false;
}


RK3588SysMsg::RK3588SysMsg()
{

}

RK3588SysMsg::~RK3588SysMsg()
{

}

uint16_t RK3588SysMsg::getSDcap(void) 
{
    // 获取SD卡挂载路径
    //std::string sdCardPath = getSDCardPath();
	//SDMsg sdMsg;
	string sdCardPath = getStoragePath();
    if (sdCardPath.empty()) 
	{
        printfs(LOG_WARNING, "未找到挂载的SD卡\n");
        return 0;
    }

    struct statvfs stats;
    if (statvfs(sdCardPath.c_str(), &stats) == 0) 
	{
        // 获取总容量、已用空间、可用空间
        //unsigned long totalSize = stats.f_blocks * stats.f_frsize;
        //unsigned long usedSpace = (stats.f_blocks - stats.f_bfree) * stats.f_frsize;
        float availableSpace = (stats.f_bavail * stats.f_frsize);

		return (uint16_t)(availableSpace * 100  / (1024 * 1024 * 1024));
    } 
	else 
	{
        printfs(LOG_WARNING,"获取 SD 卡信息失败");
        return 0;
    }
}

SdCapMsg RK3588SysMsg::getSDCapMsg(void) {
    SdCapMsg msg;
    // 获取SD卡挂载路径
    //std::string sdCardPath = getSDCardPath();
	//SDMsg sdMsg;
	string sdCardPath = getSDCardPath();
    if (sdCardPath.empty()) 
	{
        printfs(LOG_WARNING, "未找到挂载的SD卡\n");
        return msg;
    }

    struct statvfs stats;
    if (statvfs(sdCardPath.c_str(), &stats) == 0) 
	{
        // 获取总容量、已用空间、可用空间
        msg.totalSize = stats.f_blocks * stats.f_frsize;
        msg.usedSpace = (stats.f_blocks - stats.f_bfree) * stats.f_frsize;
        msg.availableSpace = (stats.f_bavail * stats.f_frsize);
        msg.availablePer = (msg.availableSpace / msg.totalSize) * 100;
    } 
	else 
	{
        printfs(LOG_WARNING,"获取 SD 卡信息失败");
    }
    return msg;
}

uint16_t RK3588SysMsg::getCPUTemperature(string tempFilePath) 
{
    std::ifstream tempFile(tempFilePath);
    if (!tempFile.is_open()) 
	{
        printfs(LOG_ERROR, "无法打开文件：%s" ,tempFilePath);
        return 0;
    }

    int temperature;
    tempFile >> temperature;
    tempFile.close();

    return (temperature/10);
}

bool RK3588SysMsg::setBrightness(string pwmFilePath, int brightness) 
{
    // 以超级用户权限运行
    if (geteuid() != 0) 
	{
        printfs(LOG_WARNING,"用户无权限运行设置亮度" );
        return false;
    }

    // 检查亮度值范围
    if (brightness < 0 || brightness > 100) 
	{
        printfs(LOG_WARNING, "值无效,亮度值应在0-100之间" );
        return false;
    }
	brightness = (brightness) * 255 / 100;
	std::ofstream pwmFile(pwmFilePath);
    if (!pwmFile.is_open()) {
        printfs(LOG_WARNING, "无法打开文件：%s" , pwmFilePath);
        return false;
    }

    pwmFile << brightness;
    pwmFile.close();

    return true;
}

string RK3588SysMsg::getSDCardPath(void) 
{
    DIR* dir = opendir("/run/media/mmcblk1p1");
    if (dir == nullptr) 
    {
        return "";
    } else {
        closedir(dir);
		return "/run/media/mmcblk1p1";
    }	
}

string RK3588SysMsg::getStoragePath(void) 
{
    return "/userdata";
}

bool RK3588SysMsg::isSDCardInserted()
{
    std::ifstream lsblk_output("/proc/mounts");
    if (!lsblk_output.is_open())
    {
        return false;
    }

    std::string line;
    while (std::getline(lsblk_output, line))
    {
        if (line.find("/dev/mmcblk1p1") != std::string::npos && line.find("/run/media/mmcblk1p1") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}
#endif

X86SysMsg::X86SysMsg()
{

}

X86SysMsg::~X86SysMsg()
{

}

uint16_t X86SysMsg::getSDcap(void) 
{
    return 0;
}

SdCapMsg X86SysMsg::getSDCapMsg(void) {
    SdCapMsg msg;
    msg.totalSize = 100.0;
    msg.availableSpace = 80.0;
    msg.usedSpace = 20.0;
    msg.availablePer = (msg.availableSpace / msg.totalSize) * 100;
    return msg;
}

uint16_t X86SysMsg::getCPUTemperature(string tempFilePath) 
{
    return 0;
}

bool X86SysMsg::setBrightness(string pwmFilePath, int brightness) 
{
    return true;
}

string X86SysMsg::getSDCardPath(void) 
{
    return "E:/02_projectArchive/88_AcousticCamera/01_SRC/72_StreamServerLite/ffmpegRtspServer/src/sdTest";
}

string X86SysMsg::getStoragePath(void) 
{
    return "E:/02_projectArchive/88_AcousticCamera/01_SRC/72_StreamServerLite/ffmpegRtspServer/src/mQtView";
}

bool X86SysMsg::isSDCardInserted()
{
    return true;
}
