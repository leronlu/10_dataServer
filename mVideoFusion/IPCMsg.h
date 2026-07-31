#ifndef _IPCMSG_H_
#define _IPCMSG_H_

#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <unistd.h>  
#include <iostream>  
#include <cstring>  

#include <sys/statvfs.h>
#include <stdlib.h>
#include <sys/msg.h>

using namespace::std;

enum class IPCMsgType : uint8_t {
	RTU 	= 60, 
	PRPD1 	= 65, 
	PRPD2 	= 66
};

struct RTUMessage 
{
	RTUMessage()
	{
		mtype = 0;
		memset(data, 0, sizeof(data));
	}
	long mtype;
	uint16_t data[512];
};

struct PrpdMessage 
{
	PrpdMessage()
	{
		mtype = 0;
		memset(data, 0.0, sizeof(data));
	}
	long mtype;
	float data[1920];
};

/*******************************************************************************
@ Function Name     : sndIPCMsg
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int getIPCMsg(int type, const char *pathname);

/*******************************************************************************
@ Function Name     : sndIPCMsg
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int sndIPCMsg(int msgid, std::vector<uint16_t> &data);


/*******************************************************************************
@ Function Name     : sndIPCMsg1
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int sndIPCMsg1(int msgid, std::vector<uint16_t> &data);

/*******************************************************************************
@ Function Name     : sndIPCMsg
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int rcvIPCMsg(int msgid, RTUMessage &message);

/*******************************************************************************
@ Function Name     : sndPrpdMsg1
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int sndPrpdMsg1(int msgid, std::vector<float> &data);

/*******************************************************************************
@ Function Name     : rcvPrpdMsg
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int rcvPrpdMsg(int msgid, PrpdMessage &message);


#endif //_IPCMSG_H_

