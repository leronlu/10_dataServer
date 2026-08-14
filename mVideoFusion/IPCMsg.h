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

/*******************************************************************************
@ Enum Name        : VideoFusionYcIndex
@ Description       : RTU IPC 遥测槽位顺序(与 02_videoServer common/qtParam.h 保持同步,
@                      变更必须两端同批发布, 见 Issue #4)
@ Note              : YcIndexCount 为哨兵(必须保持最后), 枚举扩容时 static_assert
@                      强制构建失败, 提醒同步发送端/消费端
*******************************************************************************/
enum VideoFusionYcIndex
{
	YcIndexMaxPressure,															//0  彩虹点最大声压值
	YcIndexMinPressure,															//1  彩虹点最小声压值
	YcIndexSDCapacity,															//2  SD容量
	YCIndexCpuTemp,																//3  CPU温度
	YCIndexDistant,																//4  距离
	YcIndexMaxPressureX,														//5  最强点X坐标
	YcIndexMaxPressureY,														//6  最强点Y坐标
	YcIndexMinPressureX,														//7  最弱点X坐标
	YcIndexMinPressureY,														//8  最弱点Y坐标
	YcIndexPrpdType,															//9  放电类型
	YcIndexConfidence,															//10 放电置信度
	YcIndexLRQ,																	//11 LRQ
	YcIndexLeak,																//12 泄漏量
	YcIndexGasEconomicLoss,														//13 气经济损失
	YcIndexEnergyEconomicLoss,													//14 能量经济损失
	YcIndexCount																//15 遥测数(哨兵)
};

struct RTUMessage
{
	RTUMessage()
	{
		mtype = 0;
		memset(data, 0, sizeof(data));
	}
	long mtype;
	uint16_t data[YcIndexCount];
};

// 协议不变量: RTU 遥测载荷固定 30B (15×uint16)。有意 tripwire——枚举扩容
// 时此断言强制构建失败, 提醒同步 02_videoServer 发送端协议
static_assert(sizeof(RTUMessage::data) == 30, "RTU 遥测载荷必须为 30B (15×uint16)");

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

