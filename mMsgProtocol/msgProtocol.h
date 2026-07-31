/*************************************************
  Copyright (C), 2012- , Beijing Togest Automation System Equitment Co.,Ltd
  File name:        msgManage.h
  Author:           李佳臻     
  Version:          1.0       
  Date:             2012-06-11
  Description:      该文件声明内部线程通信线程,实现方式为:该线程创建两个消息队列
                    一个消息队列(写)用于其它模块向该模块写入消息,该模块根据配置文件将
                    这些消息重新封装推入另外一个消息队列(读)
  Others:           
  Function List:    
    1. 
  History:         
    1. Date:        2012.06.11
       Author:      Li Jiazhen
       Modification:
*************************************************/

#ifndef _MSGPROTOCOL_H_
#define _MSGPROTOCOL_H_

#include "zmq_types.h"
#include "xml_parser.h"
#include "msg_frame.h"
#include <map>
#include "iec_message.h"

//----------------------------------------------
#define UPSIDEDATACONF   "../conf/UpSideDataConfig.xml"

#define MD_NAME_IEC104  "IEC104"
#define MD_NAME_IEC101  "IEC101"
#define MD_NAME_DOWN    "DOWNSIDE"
#define MD_NAME_TEST    "TestModule"
#define MD_NAME_WEB     "WebServer"
#define MD_NAME_DATABASE "DataBase"
#define MD_NAME_ModBusSev   "ModBusSev"
#define MD_NAME_DOWN_JCW    "JCWLogical"
#define MD_NAME_QT     "QTServer"
#define MD_NAME_INFRARED     "Infrared"
#define MD_NAME_AUDIO     "Audio"
#define MD_NAME_Video_Fusion  "VideoFusion"
#define MD_NAME_JMPG  "JMPGEncode"
#define MD_NAME_H264  "EncodeH264"
#define MD_NAME_Video_WEB     "VideoWeb"
#define MD_NAME_VIDEO_NODE    "VideoNode"
#define MD_NAME_UP_NODE       "UpNode"
#define MD_NAME_V4L2     "V4L2"


// Core IEC 60870-5-104 type identifiers are provided by msg_frame.h.
// Supplementary type identifiers not in msg_frame.h:
#define MSG_M_SP_TA_1               2
#define MSG_M_DP_TA_1               4
#define MSG_M_ST_TA_1               6
#define MSG_M_BO_NA_1               7
#define MSG_M_ME_TA_1              10
#define MSG_M_ME_TB_1              12
#define MSG_M_ME_TC_1              14
#define MSG_M_IT_TA_1              16
#define MSG_M_EP_TA_1              17
#define MSG_M_EP_TB_1              18
#define MSG_M_EP_TC_1              19
#define MSG_M_PS_NA_1              20
#define MSG_M_ME_ND_1              21
#define MSG_M_ST_TB_1              32
#define MSG_M_BO_TB_1              33
#define MSG_M_IT_TB_1              37
#define MSG_M_EP_TD_1              38
#define MSG_M_EP_TE_1              39
#define MSG_M_EP_TF_1              40
#define MSG_C_RC_NA_1              47
#define MSG_C_SE_NA_1              48
#define MSG_C_SE_NB_1              49
#define MSG_C_SE_NC_1              50
#define MSG_C_BO_NA_1              51
#define MSG_C_RC_TA_1              60
#define MSG_C_SE_TA_1              61
#define MSG_C_SE_TB_1              62
#define MSG_C_SE_TC_1              63
#define MSG_C_BO_TA_1              64
#define MSG_M_EI_NA_1              70
#define MSG_C_TS_NA_1             104
#define MSG_C_RP_NA_1             105
#define MSG_C_CD_NA_1             106
#define MSG_C_TS_TA_1             107
#define MSG_P_ME_NA_1             110
#define MSG_P_ME_NC_1             112
#define MSG_F_FR_NA_1             120
#define MSG_F_SR_NA_1             121
#define MSG_F_SC_NA_1             122
#define MSG_F_LS_NA_1             123
#define MSG_F_AF_NA_1             124
#define MSG_F_SG_NA_1             125
#define MSG_F_DR_TA_1             126
#define MSG_F_TM_NA_1             127
#define MSG_C_SE_ND_1             136
#define MSG_M_WA_NA_1             150
#define MSG_P_ME_NA_2             205
#define MSG_M_BD_NA_1             232
#define MSG_M_SM_NC_1             248

//------------------------------------------
//传送原因 :=CP16{Cause, P/N, T, 源发站地址} 
//  Cause:=UI6[1...6] <0>未定义   <1-47>传送原因 <48-63>预留
//  P/N  :=BS1[7]     <0>肯定确认 <1>否定确认
//  T    :=BS1[8]     <0>未实验   <1>实验
//  源发站地址 :=UI8[9...16] <0>缺省值 <1~25>源发站地址号 
#define MSG_COT_PER                 1                   //周期、循环
#define MSG_COT_BACK                2                   //背景扫描
#define MSG_COT_SPONT               3                   //突发(自发)
#define MSG_COT_INIT                4                   //初始化
#define MSG_COT_REQ                 5                   //请求或被请求
#define MSG_COT_ACT                 6                   //激活
#define MSG_COT_ACTCON              7                   //激活确认
#define MSG_COT_DEACT               8                   //停止激活
#define MSG_COT_DEACTCON            9                   //停止激活确认
#define MSG_COT_ACTTERM             10                  //激活终止
#define MSG_COT_RETREM              11                  //远方命令引起的返送信息
#define MSG_COT_RETLOC              12                  //当地命令引起的返送信息
#define MSG_COT_FILE                13                  //文件传输
/*<14~19>*/
#define MSG_COT_INTROGEN            20                  //响应站召唤
#define MSG_COT_INRO1               21                  //响应第1组召唤
#define MSG_COT_INRO2               22                  //响应第2组召唤
#define MSG_COT_INRO3               23                  //响应第3组召唤
#define MSG_COT_INRO4               24                  //响应第4组召唤
#define MSG_COT_INRO5               25                  //响应第5组召唤
#define MSG_COT_INRO6               26                  //响应第6组召唤
#define MSG_COT_INRO7               27                  //响应第7组召唤
#define MSG_COT_INRO8               28                  //响应第8组召唤
#define MSG_COT_INRO9               29                  //响应第9组召唤
#define MSG_COT_INRO10              30                  //响应第10组召唤
#define MSG_COT_INRO11              31                  //响应第11组召唤
#define MSG_COT_INRO12              32                  //响应第12组召唤
#define MSG_COT_INRO13              33                  //响应第13组召唤
#define MSG_COT_INRO14              34                  //响应第14组召唤
#define MSG_COT_INRO15              35                  //响应第15组召唤
#define MSG_COT_INRO16              36                  //响应第16组召唤
#define MSG_COT_REQCOGEN            37                  //响应计数量(累计量)站(总)召唤
#define MSG_COT_REQCO1              38                  //响应第1组计数量(累计量)召唤
#define MSG_COT_REQCO2              39                  //响应第2组计数量(累计量)召唤
#define MSG_COT_REQCO3              40                  //响应第3组计数量(累计量)召唤
#define MSG_COT_REQCO4              41                  //响应第4组计数量(累计量)召唤
/*<42~43>*/
#define MSG_COT_UNTI                44                  //未知的类型标识
#define MSG_COT_UNCOT               45                  //未知的传送原因
#define MSG_COT_UNCOMADDR           46                  //未知的应用服务数据单元公共地址
#define MSG_COT_UNINFOADDR          47                  //未知的信息对象地址
/*<48~63>专用范围*/

#define MAX_STRING_LEN  30                      //模块名称最大字节数
#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE    256                     //消息最大长度
#endif

//#define	UpSideDataMapEn

//----------------------------------------------
//MSG_C_ST_NA_1 字符串传递类型标志相关
//通信链路类型
enum LinkStatType
{
    LinkTypeDefault,                                    //缺省为0
    LinkTypeYk,                                         //遥控
    LinkTypeYx,                                         //遥信
    LinkTypeYc,                                         //遥测
    LinkTypeGx,                                         //光纤板
    LinkTypeModbus,										//modbusClient
    LinkTypeIEC104,                                     // 104
    LinkTypeIEC101,                                     // 101
    LinkTypeModbusServer,                                //modbusserver
    LinkRise3501                               			//modbusserver
};
//各模块地址(信息体地址低8位)
enum ModuleAddr
{
    MAddrdefault,
    MAddrIEC104,                                        //上行 104模块
    MAddrIEC101,                                        //     101模块
    MAddrModbusServer,                                  //     modbus模块
    MAddrWeb,                                           //     web模块
    MAddrDataBase,                                      //     数据库模块
    MAddrDownSide,                                      //下行 数据管理模块
    MAddrDownCan,                                       //     CAN模块
    MAddrDownMBus,                                      //     modbus模块
    MAddrDownSMS,                                       //     SMS短信模块
    MAddrDownJCW                                        //     接触网逻辑模块
};
//字符串类型(信息体地址高8位)
enum StrModuleType
{
    StrMTypeLink,                                       //通信链路状态
    StrMTypeYkStr,                                      //遥控控制信息
    StrMTypeSMSSoe,                                     //短信模块SOE字符串信息
    StrMTypeCheckYxSOE
};
//模块级别命令次类型标识
enum ModuleCMDTi
{
    ModuleCMDTiDoubleBackup,                             ///双机主备相关命令
    ModuleCMDSyncTime,                                   ///接收主站授时后，同步主备时间
    ModuleModbusDoubleBackupActive,						 ///modbus模块主激活
    ModuleModbusDoubleBackupCancel,						 ///modbus模块主取消
    ModuleCMDTiMBDoubleBackup,							 ///modbus模块双机主备相关命令
    ModuleCMDTi101DoubleBackupStandBy,					 //IEC101模块待命可链接
    ModuleCMDTi101DoubleBackupCancel,					 //IEC101模块主取消
    ModuleCMDTi101DoubleBackupReq,					 	 //IEC101模块获取
};
//----------------------------------------------
//消息格式

struct Unit
{
	Unit(){
		addr = 0;
		nodeId = 0xffff;
	};
	uint16_t addr;
	uint16_t nodeId;
	std::string ip;
};

struct AddrMap
{
	AddrMap()
	{
	}
	std::map<std::string, Unit> 		singleYxDown;
	std::map<std::string, uint16_t> 	singleYxUp;
	
	std::map<std::string, Unit> 		doubleYxDown;
	std::map<std::string, uint16_t> 	doubleYxUp;
	
	std::map<std::string, Unit> 		ycDown;
	std::map<std::string, uint16_t> 	ycUp;
	
	std::map<std::string, Unit> 		ykDown;
	std::map<std::string, uint16_t> 	ykUp;
	
	std::map<std::string, Unit> 		ymDown;
	std::map<std::string, uint16_t> 	ymUp;

	std::map<std::string, Unit> 		ytDown;
	std::map<std::string, uint16_t> 	ytUp;

	std::map<std::string, Unit> 		boardCmdDown;
	std::map<std::string, uint16_t> 	boardCmdUp;

	std::map<std::string, Unit> 		localCmdDown;
	std::map<std::string, uint16_t> 	localCmdUp;

	std::map<std::string, Unit> 		cmdDown;
	std::map<std::string, uint16_t> 	cmdUp;
};

#if 0
struct MsgData
{
    uint8_t srcModuleName[MAX_STRING_LEN];        //模块名称:104模块 IEC104_0//_0表示线程号
    uint8_t dstModuleName[MAX_STRING_LEN];        //dstModuleName为空时该帧信息向该模块的所有线程发送
    uint8_t data[MAX_MSG_SIZE];                   //消息内容
    uint8_t len;
};
#endif

//数据映射类
class DataManager
{
public:
    DataManager();
    ~DataManager();
public:
    void    loadConfig();
	uint16_t 	getUpMapAddr(uint8_t type, std::string srcIp, uint16_t infoAddr);
	Unit 	getDownMapAddr(uint8_t type, uint8_t infoAddr);
	uint16_t  getSingleYxNum(){return addrMap.singleYxUp.size();}
	uint16_t  getDoubleYxNum(){return addrMap.doubleYxUp.size();}
	uint16_t  getYcNum(){return addrMap.ycUp.size();}
	uint16_t  getYtNum(){return addrMap.ytUp.size();}
	uint16_t  getYmNum(){return addrMap.ymUp.size();}
private:
	void 	AddrMapInit(uint16_t addr, uint16_t mapAddr, uint16_t num, Unit unit, std::string type);
private:
	static AddrMap addrMap;
};

//消息帧类
//该类根据IEC60870-101篇协议的ASDU部分创建的,用于解析和生成线程间的消息
//即MsgData.data部分
//其中
//      VSQ中的SQ=0,即不使用顺序传输
//      传输原因长度限定为1个字节
//      公共地址长度限定为1个字节
//      信息对象地址长度限定为2个字节
/*
消息格式: 最大256字节
            TYP:        1字节
            VSQ:        1字节
            TOC:        1字节
            CommAddr:   1字节
            UnitAddr:   2字节
            UnitData:   可变
            UnitAddr:   2字节
            UnitData:   可变
            .
            .
            .
*/
uint16_t getMapUnitAddr(const IecMessage &msg, size_t idx, const std::string &srcIp);
Unit     setMapUnitAddr(IecMessage &msg, size_t idx, uint16_t rawAddr);

class DirectZmqMsgManage;
void initDirectZmqMsgManage(DirectZmqMsgManage &mgr);

#endif

