/*************************************************
  Copyright (C), 2012- , Beijing Togest Automation System Equitment Co.,Ltd
  File name:        DownSideDataModule.h
  Author:           李佳臻     
  Version:          1.0       
  Date:             2012-06-25
  Description:      该文件声明下行数据配置管理
  Others:           
  Function List:    
    1. 
  History:         
    1. Date:        2012.06.25
       Author:      Li Jiazhen
       Modification:
*************************************************/

#ifndef _DownSideDataModule_H_
#define _DownSideDataModule_H_
#include <string>
#include <list>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include "RtuBaseClass.h"

// NODE_COMM_IEC104 and B32SType moved here from typedef.h
#define NODE_COMM_IEC104    4
typedef union {
    unsigned char bytes[4];
    int           intvalue;
    float         fpvalue;
} B32SType;
#include "log_manage.h"
#include "xml_parser.h"
#include "date_manage.h"
#include "msgProtocol.h"
#include "direct_zmq_msg_manage.h"

#include "MicrophoneModule.h"
#include "ModBusModule.h"
#include "MinmeaModule.h"

#define DownSideConfigFile  "../conf/DownSideDataConfig.xml"
#define YXNUM_PER_FRAME     60                                                  //每帧无时标的遥信个数
#define YCNUM_PER_FRAME     40                                                  //每帧无时标的遥测个数
#define YCNUM_PER_FRAME_FLOAT    24                                       //每帧无时标的遥测浮点型个数
#define YXSOE_PER_FRAME     22                                                  //每帧带时标的遥信个数
#define YCSOE_PER_FRAME     18                                                  //每帧带时标的遥测个数
#define FP32YCSOE_PER_FRAME 16                                                  //每帧带时标的短浮点遥测个数
#define FP32YC_PER_FRAME    30                                                  //每帧无时标的短浮点遥测个数
#define YMNUM_PER_FRAME     30                                                  //每帧无时标的遥脉个数
#define YCSOE_PER_FRAME_FLOAT      15                                         //每帧带时标的遥测浮点型个数
#define NODEINFO_PER_FRAME  24                                                  //每帧节点信息的个数

#define DOWN_MD_NAME_CAN    "CAN"
#define DOWN_MD_NAME_ModBus "ModBus"
#define DOWN_MD_NAME_TUNENL "TunnelLight"
#define DOWN_MD_NAME_UPS    "Ups"
#define DOWN_MD_NAME_VIRTUAL "Virtual"                                          ///虚拟模块该模块不采集物理数据

enum TableType
{
    ykTable,
    yksingleTable,
    paramTable,
    paramGroupTable,
    ycCircuitTable,
    singleYxTable,
    doubleYxTable,
    validYcTable
};

enum YtGroupDataIndex
{
    YtGroupDataIdxType = 0,
    YtGroupDataIdxVSQ,
    YtGroupDataIdxCOT,
    YtGroupDataIdxCommAddr,
    YtGroupDataIdxInfoAddr0,
    YtGroupDataIdxInfoAddr1,
    YtGroupDataIdxCirLow,
    YtGroupDataIdxCirHig,
    YtGroupDataIdxINF,
    YtGroupDataIdxRII,
    YtGroupDataIdxNGD,
    YtGroupDataIdxGINGroup,
    YtGroupDataIdxGINEntry,
    YtGroupDataIdxKOD,
    YtGroupDataIdxDataType,
    YtGroupDataIdxDataSize,
    YtGroupDataIdxDataIndex,
    YtGroupDataIdxGID0,
    YtGroupDataIdxGID1
};

/**************************************************************************
@ Description:      定义下行数据配置
***************************************************************************/
struct DownSideConfig
{
    std::string          moduleName;
    std::string          moduleConfigFile;
    BaseDataConfig  config;
    BaseDataConfig  baseAddr;
};

class DownSideDataModule 
{
public:
    DownSideDataModule();
    ~DownSideDataModule();
public:
    void    initModule();
    void    loadConfig();
    void    run();
    void    stop();                                                             //析构所有子模块,用以动态配置加载
    BaseDataConfig getDataConfig();
    std::vector<DownSideConfig> & getDownSideConfig();                               //返回各个模块配置项                                         //返回主备指针
	void setCallbackFunction(std::function<void(const uint8_t *, uint32_t)> func);
	void setCallbackFunction(std::function<void(std::vector<uint8_t> &)> func);
private:
    static void * processRoutine(void *arg);

private:
    void    processMsg();                                                       //处理消息
    void    processChildSpontEvent();                                           //子模块的突发事件
    void    processSpontEvent();                                                //模块突发事件处理

    bool    sendMsg(MsgData &msg) {
        return m_sendMsgFifo.pushBack(msg);
    }          //发送消息
    void    sendMsg();                                                          //发送消息至消息管理模块
    int8_t   getTableNo(uint16_t addr, TableType type);                            //获取该消息的目标子模块序号
    int8_t   getTableNo(uint8_t cpuNum, canNodeType type);                         //获取该消息的目标子模块序号
    
private:                                                                        //消息处理子函数
    void    callAll(MsgData &msg, const IecMessage &iecMsg);                                              //全召
    void 	callAll(MsgData &msg);
    void    callAllStart(MsgData &msg, uint8_t QOI);
	void 	callAllStart(MsgData &msg);
    void    callAllEnd(MsgData &msg, uint8_t QOI);
	void 	callAllEnd(MsgData &msg);
    void    callDoubleYx(MsgData &msg);
    void    callSingleYx(MsgData &msg);
    void    callYc(MsgData &msg);
    void    callYcDc(MsgData &msg);
    void    callFP32Yc(MsgData &msg);
    void    setYkCmd(MsgData &msg, const IecMessage &iecMsg, TableType type=ykTable);
    void    setParam(MsgData &msg, const IecMessage &iecMsg);
    void    setYt(MsgData &msg, const IecMessage &iecMsg);
    void    setGroupYt(const MsgData &msg, MsgData &ytMsg);
    void    setNodeUpdate(MsgData &msg, const IecMessage &iecMsg);                        //读取版本号
	void 	setDoman(MsgData &msg, const IecMessage &iecMsg);	

    void    callPartAll(MsgData &msg, const IecMessage &iecMsg);                          //部分指定范围全召
    void    setManaulWave(MsgData &msg, const IecMessage &iecMsg);
    void    errMsg(MsgData &msg);                                               //错误消息处理 
    void    callAllYm(MsgData &msg, const IecMessage &iecMsg);
    void    callAllYmStart(MsgData &msg);
    void    callAllYmEnd(MsgData &msg);
    void    callYm(MsgData &msg);
	void	callYt(MsgData &msg);
    void    YmFrzStart(MsgData &msg, int8_t FRZ);
    bool    YmFrz(const IecMessage &iecMsg);
    //void    exeModuleCMD(MsgData &msg);
private:                                                                        //突发事件处理函数
    void    getDoubleYxSoe(MsgData &msg);                                       //SOE信息处理
    void    getSingleYxSoe(MsgData &msg);
    void    getValidYcSoe(MsgData &msg);
    void    getValidYcDcSoe(MsgData &msg);
    void    getFP32YcSoe(MsgData &msg);
    void    getDoubleYxCh(MsgData &msg);                                        //一级变位信息处理
    void    getSingleYxCh(MsgData &msg);
    void    getValidYcCh(MsgData &msg);
    void    getValidYcDcCh(MsgData &msg);
    void    getFP32YcCh(MsgData &msg);
    void    getNodeVersion(MsgData &msg);
    void    getNodeInfoCh(MsgData &msg);
    void    getSpontEvent();                                                    //突发事件处理

private:                                                                        //子函数
    void    getDoubleYxData(MsgData &msg, uint8_t msgCOT);
    void    getSingleYxData(MsgData &msg, uint8_t msgCOT);
    void    getValidYcData(MsgData &msg, uint8_t msgCOT);
    void    getValidYcDcData(MsgData &msg, uint8_t msgCOT);
    void    getValidYmData(MsgData &msg, uint8_t msgCOT);
	void 	getValidYtData(MsgData &msg, uint8_t msgCOT);
    void    getFP32YcData(MsgData &msg, uint8_t msgCOT);
    void    getNodeYxData(MsgData &msg);                                        //获取指定CAN节点遥信
    void    getCircYcData(MsgData &msg);                                        //获取指定回路遥测值
    void    sendYkEventMsg(SpontEvent &event, int8_t childNo, TableType type=ykTable);
    void    sendSetParamEventMsg(SpontEvent &event, int8_t childNo);	
    void    sendYcErrReportEventMsg(SpontEvent &event, int8_t childNo);
    void    sendUpdateEventMsg(SpontEvent &event, int8_t childNo);	
    void    sendCallPartEventMsg(SpontEvent &event, int8_t childNo);
    void    sendManaulWaveEventMsg(SpontEvent &event, int8_t childNo);
    void    sendYkStr(const MsgData &rmsg);
    void    setVirtualData(const IecMessage &iecMsg);
    void    setCommYxStat(const uint16_t addrOffset, uint8_t stat);    
private:
    char                m_moduleName[30];                                       //模块名称
    
    std::vector<DownSideConfig>m_confList;                                             //配置
    std::vector<RtuBaseClass *>m_moduleList;                                           //实例
    //JCWLogical          *d_jcwLogical;                                          ///接触网模块

    pthread_t           m_serverPthread;
    
    DirectZmqMsgManage  m_msgManage;
    STLDeque<MsgData>   m_sendMsgFifo;                                          //消息发送队列

    TimerService        m_timerYcPoll;                                          //遥测轮询定时器
    TimerService        m_YmPoll;                                               //遥脉轮询定时器
    /*文件管理*/
    TimerService        m_timerAnalogSave;                                      //模拟值定时存储定时器
    FileManageModule   *d_fileManage;                                           //文件管理
    std::vector<FileManageConfig> *d_manage;
    //DoubleBackup       *d_doubleBackup;                                         //can模块主备模式

    //DoubleBackup		*d_MBDoubleBackup;									    //modbus模块主备模式
    //DoubleBackup       *d_WatthourMeterDoubleBackup;            //电能表主备
    bool                m_nodeCommYxEnable;                                     //节点通信转遥信使能
    //SoftVirtual         *d_softVirtual;                                         //处理额外数据  

    uint8_t               m_IEC104CommStat[NODE_COMM_IEC104];                     // 104通信状态缓存
	std::function<void(const uint8_t *, uint32_t)> spectrumOnFrame;
	std::function<void(std::vector<uint8_t> &)> spectrumOnFrame1;
};

#endif

