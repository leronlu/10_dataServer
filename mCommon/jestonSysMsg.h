#ifndef _JETSONSYSMSG_H_
#define _JETSONSYSMSG_H_

#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#ifdef Q_OS_LINUX
#include <sys/statvfs.h>
// #include "typedef.h" removed
#include "log_manage.h"
#include "xml_parser.h"
#include "date_manage.h"
#endif

using namespace::std;

struct SdCapMsg {
	SdCapMsg() {

	};
	float totalSize = 0;
	float usedSpace = 0;
	float availableSpace = 0;
	float availablePer = 0.0;
};

class SysMsgInter
{
public:
    virtual ~SysMsgInter() {} 
public:
	virtual std::string getSDCardPath(void) = 0;
	virtual std::string getStoragePath() = 0;
	virtual bool isSDCardInserted() = 0;
	virtual uint16_t getSDcap(void) = 0;
	virtual SdCapMsg getSDCapMsg(void) = 0;
	virtual uint16_t getCPUTemperature(std::string tempFilePath) = 0;
	virtual bool setBrightness(std::string pwmFilePath, int brightness) = 0;
};

#ifdef Q_OS_LINUX
class JestonSysMsg : public SysMsgInter
{
public:
	JestonSysMsg();
	~JestonSysMsg();
public:
	std::string getSDCardPath(void) override;
	std::string getStoragePath() override;
	bool isSDCardInserted() override;
	uint16_t getSDcap(void) override;
	SdCapMsg getSDCapMsg(void) override;
	uint16_t getCPUTemperature(std::string tempFilePath) override;
	bool setBrightness(std::string pwmFilePath, int brightness) override;
};

class RK3588SysMsg : public SysMsgInter
{
public:
	RK3588SysMsg();
	~RK3588SysMsg();
public:
	std::string getSDCardPath(void) override;
	std::string getStoragePath() override;
	bool isSDCardInserted() override;
	uint16_t getSDcap(void) override;
	SdCapMsg getSDCapMsg(void) override;
	uint16_t getCPUTemperature(std::string tempFilePath) override;
	bool setBrightness(std::string pwmFilePath, int brightness) override;
};
#endif

class X86SysMsg : public SysMsgInter
{
public:
	X86SysMsg();
	~X86SysMsg();
public:
	std::string getSDCardPath(void) override;
	std::string getStoragePath() override;
	bool isSDCardInserted() override;
	uint16_t getSDcap(void) override;
	SdCapMsg getSDCapMsg(void) override;
	uint16_t getCPUTemperature(std::string tempFilePath) override;
	bool setBrightness(std::string pwmFilePath, int brightness) override;
};

#endif //

