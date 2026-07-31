#ifndef _SDMSG_H_
#define _SDMSG_H_

#include <stdlib.h>
#include <fstream>

#include <string>

#include "jestonSysMsg.h"

using std::string;

class SDMsg
{
public:
	SDMsg(){
	#ifdef Q_OS_LINUX
		sysMsg = new RK3588SysMsg();
	#else
		sysMsg = new X86SysMsg();
	#endif
	};
	~SDMsg(){
		delete sysMsg;
	};
	std::string getSDCardPath(void) 
	{
		return sysMsg->getStoragePath();
	}
	std::string getSDStoragePath(void) 
	{
		return sysMsg->getSDCardPath();
	}
	bool isSDCardInserted()
    {
        return sysMsg->isSDCardInserted();
    }
	float getSDCapAvailablePer()
    {
		if(!sysMsg->isSDCardInserted()){
			return 0.0;
		}
		SdCapMsg msg = sysMsg->getSDCapMsg();
		return msg.availablePer;
    }
private:
	SysMsgInter *sysMsg = nullptr;
};

#endif //_SDMSG_H_
