/*************************************************
  Copyright (C), 2012- , Beijing Togest Automation System Equitment Co.,Ltd
  File name:        RtuBaseClass.h
  Author:           李佳臻     
  Version:          1.0       
  Date:             2012-06-26
  Description:      该文件声明下行数据各个子模块的基类,为了使用继承的多态而创建该类
  Others:           
  Function List:    
    1. 
  History:         
    1. Date:        2012.06.26
       Author:      Li Jiazhen
       Modification:
*************************************************/

#ifndef _RtuBaseClass_H_
#define _RtuBaseClass_H_

#include <functional>

// #include "typedef.h" removed
#include "FileManageModule.h"
#include "filemanage.h"
//#include "DoubleBackup.h"

#define NODE_OFFLINE    1                                                       //板卡在线
#define NODE_ONLINE     2                                                       //板卡离线

//----------------------------------------
//事件类型
enum EventType
{
    ykCmdEvent,
    ycSetParamEvent,
    ycErrReportEvent,
    ycWaveFileEvent,
    eventTypeVerson,
    eventTypeUpdate,
    eventTypeICPart,
    ycManualWaveEvent,
    ykCmdNoResponseEvent,
    eventTypeDBUpdate,
    eventTypeDBRecord,
    eventTypeDBSetParam,
    eventTypeModbusRead
};

//----------------------------------------
//事件状态
enum EventState
{
    eventNone = 0,
    eventSpont = 3,
    eventReq = 5,
    eventAct,
    eventActCon,
    eventStopAct,
    eventStopActCon,
    eventActerm,
    eventNoTi = 44,
    eventNoCot,
    eventNoComAddr,
    eventNoInforAddr
};

//事件状态值
enum EventValue
{
    successed,
    actioned = 0x02,
    failed   = 0x40
};

enum canYkStatus                                                            //遥控过程状态
{
    ykidle, 
    ykselect, 
    ykselectACK,
    ykexecute,
    ykexecuteACK
};  

enum canYcParamQpm                                                          //遥测QPM: KPA位值
{
    QPM_setID=0,                                                            //设置ID
    Thv1=1, 
    Thv2, 
    Limit_low, 
    Limit_high, 
    Limit_llow, 
    Limit_hhigh,                                             
    Limit_hhhigh,
    BTNParam=80,
};

enum canNodeType                                                            //CAN节点板卡类型
{
    canNodeYk=1,
    canNodeYx,
    canNodeYc,
    canNodeGx,
    canNodeEnd
};
enum DBNodeType                                                            //Modbus节点板卡类型
{
    MBottom = 0,
    MRoof,
    MSerial0,
    MSerial1,
    MSerial2,
    MSerial3,
    MSerial4,
    MSerial5,
    MSerial6,
    MSerial7,
    MSerial = 0xFF,
    MReset_Bottom = 0x80,                                                       //复位底板
    MReset_Roof = 0x81,                                                         //复位顶板
    MSetParam = 0x40,                                                           //通用参数设置
    MSetSerialSpeed = 0x20,                                                     //设置串口速率
    MEnd
};

enum ModbusNodeType                                                            //Modbus节点板卡类型
{
    MBType = canNodeEnd,
};

enum Rise3501NodeType                                                            //Modbus节点板卡类型
{
    Rise3501Type = 9,
};

enum ykType
{
    ykTypeDouble,
    ykTypeSingle
};

enum baseAddrOffsetType
{
    BASEADDR_TYPE_RELATE,                                                   //相对
    BASEADDR_TYPE_ABSOLUTE                                                  //绝对
};

/*数据类型*/
enum virtualDataType
{
    virtualDataSingleYx,                                                        //单点遥信
    virtualDataDoubleYx,                                                        //双点遥信
    virtualDataYc,                                                              //遥测
    virtualDataYm,
    virtualDataFPYc
};


enum YcGroupYtInfo
{
    YcGroupYtInfoReadValue      = 241,                                          ///读取组数据
    YcGroupYtInfoWriteValuePre  = 249,                                          ///修改数据预置
    YcGroupYtInfoWriteValueExe  = 250,                                          ///修改数据执行
    YcGroupYtInfoWriteValueAbort= 251                                           ///修改数据撤销
};

enum YcGroupYtDataType
{
    YcGroupYtDataTypeU8 = 1,                                                    ///数据类型无符号8位
    YcGroupYtDataTypeI8,                                                        ///        有符号8位
    YcGroupYtDataTypeU16,
    YcGroupYtDataTypeI16,
    YcGroupYtDataTypeU32,
    YcGroupYtDataTypeI32,
    YcGroupYtDataTypeF32                                                        ///         浮点32位
};

enum YcGroupYtValueIndex
{
    /*YcGroupYtValueIdxCurOverI,
    YcGroupYtValueIdxCurOverII,    
    YcGroupYtValueIdxCurOverIII,
    YcGroupYtValueIdxVolUnderI,
    YcGroupYtValueIdxVolUnderII,
    YcGroupYtValueIdxVolOverI,
    YcGroupYtValueIdxVolOverII,
    YcGroupYtValueIdxEnd*/
    YcGroupYtValueIdxIThv1,
	YcGroupYtValueIdxIThv2,
	YcGroupYtValueIdxIh,
	YcGroupYtValueIdxIhh,
	YcGroupYtValueIdxIhhh,
	YcGroupYtValueIdxIhCheckingTime,
	YcGroupYtValueIdxIhhCheckingTime,
	YcGroupYtValueIdxIhhhCheckingTime,
	YcGroupYtValueIdxVThv1,
	YcGroupYtValueIdxVThv2,
	YcGroupYtValueIdxUd,
	YcGroupYtValueIdxUdd,
	YcGroupYtValueIdxUh,
	YcGroupYtValueIdxUhh,
	YcGroupYtValueIdxUdCheckingTime,
	YcGroupYtValueIdxUddCheckingTime,
	YcGroupYtValueIdxUhCheckingTime,
	YcGroupYtValueIdxUhhCheckingTime,
	YcGroupYtValueIdxEnd
};

enum ZoneIndex
{
	GINZoneIndex = 2,
	NGDZoneNum = 4
};

enum ParamQpm													//遥测参数新增
{
	Limit_IThv1 =1,
    Limit_IThv2,
    Limit_Ih,
    Limit_Ihh,
    Limit_Ihhh,
    Limit_IhCheckingTime,
    Limit_IhhCheckingTime,
    Limit_IhhhCheckingTime,
    Limit_VThv1,
    Limit_VThv2,
    Limit_Ud,
    Limit_Udd,
    Limit_Uh,
    Limit_Uhh,
    Limit_UdCheckingTime,
    Limit_UddCheckingTime,
    Limit_UhCheckingTime,
    Limit_UhhCheckingTime,
    Limit_ZIThv1,
    Limit_ZIThv2,
    Limit_ZIh,
    Limit_ZIhcheckingTime,
    Limit_ZUThv1,
    Limit_ZUThv2,
    Limit_ZUh,
    Limit_ZUhcheckingTime,

	QPM_Modbus = 32,
};

enum canYcPhaseType
{
	canYcIPhaseA,
	canYcIPhaseB,
	canYcIPhaseC,
	canYcUPhaseA,
	canYcUPhaseB,
	canYcUPhaseC,
	canYcIPhaseZ,
	canYcUPhaseZ,
	canYcPhaseNum
};


/**************************************************************************
@ Description:      定义数据结构表
***************************************************************************/
struct DataINT8U                                                                //8位数据
{
	DataINT8U(){
		change = false;
		value = 0;
	}
    bool            change;                                                     //变位
    uint8_t           value;                                                      //值
};
struct DataINT8UNew                                                             //8位数据
{
	DataINT8UNew()
	{
		change = false;
		value = 0;
		defaultJudgeEn = false;
		defaultValue = 0;
	}
    bool            change;                                                     //变位
    uint8_t           value;                                                      //值
    bool 			defaultJudgeEn;												//默认值判断使能
    uint16_t          defaultValue;                                               //值
};
struct DataINT16S                                                               //16位数据
{
	DataINT16S()
	{
		change = false;
		value = 0;
	}
    bool            change;
    int16_t          value;
};
struct DataINT16Yt                                                               //16位数据
{
	DataINT16Yt()
	{
		value = 0;
		qpm = 0;
		change = false;
	}
	int16_t          value;
	uint8_t			qpm;
    bool            change;
};

struct DataINT32U                                                               //32位数据
{
	DataINT32U()
	{
		change = false;
		value = 0;
	}
    bool            change;
    uint32_t          value;
};
struct DataINT32S                                                               //32位有符号数据
{
	DataINT32S()
	{
		change = false;
		value = 0;
	}
    bool            change;
    int32_t          value;
};   

struct DataFP32
{
	DataFP32()
	{
		change = false;
		value = 0.0;
	}
    bool            change;
    float            value;
};
//Config 配置
/**************************************************************************
@ Description:      定义ValidYxConfig结构体
***************************************************************************/
struct ValidYxConfig
{
	ValidYxConfig()
	{
		id = 0;
		doubleYxNum = 0;
		singleYxNum = 0;
		YmNum = 0;
		doubleYxBaseAddr = 0;
		singleYxBaseAddr = 0;
		YmBaseAddr = 0;
	}
    uint8_t   id;                                                             //遥信子设备ID
    uint8_t   doubleYxNum;                                                    //双点遥信数目
    uint8_t   singleYxNum;                                                    //单点遥信数目
    uint8_t   YmNum;                                              //lx-遥脉个数
    uint16_t  doubleYxBaseAddr;                                               //双点遥信基址    //lx:第几块板的双点地址在总遥信点表中的地址
    uint16_t  singleYxBaseAddr;                                               //单点遥信基址
    uint16_t  YmBaseAddr;                                     //lx-遥脉基址
};

/**************************************************************************
@ Description:      定义ValidYkConfig结构体
***************************************************************************/
struct ValidYkConfig
{
	ValidYkConfig()
	{
		id = 0;
		doubleYkNum = 0;
		singleYkNum = 0;
		doubleYkBaseAddr = 0;
		singleYkBaseAddr = 0;
	}
    uint8_t   id;                                                             //遥控子设备ID
    uint8_t   doubleYkNum;                                                    //双点遥控数目
    uint8_t   singleYkNum;                                                    //单点遥控数目
    uint16_t  doubleYkBaseAddr;                                               //双点遥控基址
    uint16_t  singleYkBaseAddr;                                               //单点遥控基址
};

/*
@brief 用于光纤精准校时存储时间
*/
struct accuTiming
{
	accuTiming()
	{
		enable = false;
	}
    bool    enable;
    timeval  startTime;
    
};

/**************************************************************************
@ Description:      定义ValidGxConfig结构体
***************************************************************************/
struct ValidGxConfig : public ValidYxConfig, public ValidYkConfig
{
	ValidGxConfig(){
		;
	}                                                            //光纤子设备ID
    accuTiming timing;
};

/**************************************************************************
@ Description:      定义ValidYcConfig结构体
***************************************************************************/
struct ValidYcConfig
{
	ValidYcConfig()
	{
		id = 0;
		validYcNum = 0;
		validCircuitNum = 0;
		validYcDcNum = 0;
		validCircuitNumPerBoard = 0;
		validYcNumPerCircuit = 0;
		validYcBaseAddr = 0;
		validYcDcBaseAddr = 0;
		softYxBaseAddr = 0;
	}
    uint8_t   id;                                                             //遥测子设备ID
    uint16_t  validYcNum;                                                     //有效的遥测数目
    uint16_t  validCircuitNum;                                                //有效的回路数目
    uint8_t   validYcDcNum;                                                   //有效的直流数目
    uint16_t  validCircuitNumPerBoard;                                        //每个采集板的有效回路数
    uint16_t  validYcNumPerCircuit;                                           //每个有效回路的遥测量 
    uint16_t  validYcBaseAddr;                                                //每个节点遥测值基址
    uint16_t  validYcDcBaseAddr;                                              //每个节点遥测直流基址
    uint16_t  softYxBaseAddr;                                             //软遥信的基址
};

/**************************************************************************
@ Description:      定义SpontEvent结构体,用于子模块的突发事件及需要返回确认的事件
                事件包括:
                        1、遥控的下发、返回及超时
                        2、遥测的下发、返回及超时
                        3、遥测的故障报告突发
                        4、其它可能存在的事件报告
***************************************************************************/

struct SpontEvent
{
    SpontEvent() : availability(false)
    {
        type = ykCmdEvent;
        cmd = 0;
        addr = 0;
        state = eventNone;
        value = successed;
    }
    bool            availability;
    string          srcModuleName;
    EventType       type;
    uint8_t         cmd;
    uint16_t        addr;
    EventState      state;
    EventValue      value;
    vector<uint8_t> data1;
    struct EventData
    {
        EventData() { len = 0; memset(data, 0, sizeof(data)); }
        uint16_t len;
        uint8_t  data[1024*8];
    } data;
};

struct BaseAddrOffset
{
    BaseAddrOffset() { type = BASEADDR_TYPE_RELATE; baseAddr = 0; }
    baseAddrOffsetType type;
    uint16_t           baseAddr;
};

struct BaseDataConfig
{
    BaseDataConfig() : doubleYxnum(0), singleYxnum(0), validYcnum(0), validYcDcNum(0),
        B32YcNum(0), validYtNum(0), validYcCircuitNum(0), validYmnum(0),
        doubleYknum(0), singleYknum(0), yxNodeNum(0), ykNodeNum(0), ycNodeNum(0),
        gxNodeNum(0), modbusDeviceNum(0), modbusSerialIndex(0), rise3501DeviceNum(0) {}
    uint16_t doubleYxnum, singleYxnum, validYcnum, validYcDcNum, B32YcNum;
    uint16_t validYtNum, validYcCircuitNum, validYmnum, doubleYknum, singleYknum;
    uint8_t  yxNodeNum, ykNodeNum, ycNodeNum, gxNodeNum, modbusDeviceNum;
    uint8_t  modbusSerialIndex, rise3501DeviceNum;
    BaseDataConfig &operator+=(const BaseDataConfig &c) {
        doubleYxnum+=c.doubleYxnum; singleYxnum+=c.singleYxnum;
        validYcnum+=c.validYcnum; validYcDcNum+=c.validYcDcNum;
        B32YcNum+=c.B32YcNum; validYtNum+=c.validYtNum;
        validYcCircuitNum+=c.validYcCircuitNum; validYmnum+=c.validYmnum;
        doubleYknum+=c.doubleYknum; singleYknum+=c.singleYknum;
        yxNodeNum+=c.yxNodeNum; ykNodeNum+=c.ykNodeNum;
        ycNodeNum+=c.ycNodeNum; gxNodeNum+=c.gxNodeNum;
        modbusDeviceNum+=c.modbusDeviceNum;
        rise3501DeviceNum+=c.rise3501DeviceNum;
        return *this;
    }
};

struct DataSoe
{
    DataSoe() { addr=0; value=0; type=0; memset(dateTime,0,sizeof(dateTime)); }
    uint16_t addr, value;
    uint8_t  type;
    uint8_t  dateTime[7];
    bool operator==(const DataSoe &s) {
        return (addr==s.addr)&&(value==s.value)&&(!memcmp(dateTime,s.dateTime,7));
    }
};

struct DataFP32Soe
{
    DataFP32Soe() { addr=0; value=0.0f; memset(dateTime,0,sizeof(dateTime)); }
    uint16_t addr;
    float    value;
    uint8_t  dateTime[7];
};

#define CAN_VER_EXT_NUM 3
struct NodeInfo
{
    NodeInfo() { nodeType=0; nodeID=0; nodeLinkStat=0; nodeVersion=0;
        memset(nodeExtInfo,0,sizeof(nodeExtInfo)); }
    uint8_t  nodeType, nodeID, nodeLinkStat;
    uint16_t nodeVersion;
    uint8_t  nodeExtInfo[CAN_VER_EXT_NUM];
};

class RtuBaseClass
{
public:
    RtuBaseClass() { m_moduleIndex = 0; }
    virtual ~RtuBaseClass() {}
    void setConfig(const BaseDataConfig &c) { m_config = c; }
    void setModuleInfo(const string &name, const uint8_t idx) { m_moduleName=name; m_moduleIndex=idx; }
    const string &getModuleName() { return m_moduleName; }
    virtual void setBaseAddr(const BaseDataConfig &a) { m_baseAddr = a; }
    virtual BaseDataConfig getBaseAddr() { return m_baseAddr; }
    virtual void loadConfig() = 0;
    virtual BaseDataConfig getConfig() = 0;
    virtual void run() = 0;
    virtual void initModule() = 0;
    virtual void setFileManage(FileManage *m=NULL, RtuFileType t=RtuFileTypeNormal) {}
    virtual void callAll(uint16_t i=100) {}
    virtual bool getDoubleYxSoe(DataSoe &s) { return false; }
    virtual bool getSingleYxSoe(DataSoe &s) { return false; }
    virtual bool getValidYcSoe(DataSoe &s) { return false; }
    virtual bool getValidYcDcSoe(DataSoe &s) { return false; }
    virtual bool getFP32YcSoe(DataFP32Soe &s) { return false; }
    virtual bool getDoubleYxData(const uint16_t a, uint8_t &v, bool c=false) { return false; }
    virtual bool getSingleYxData(const uint16_t a, uint8_t &v, bool c=false) { return false; }
    virtual bool getValidYcData(const uint16_t a, uint16_t &v, bool c=false) { return false; }
    virtual bool getValidYcDcData(const uint16_t a, uint16_t &v, bool c=false) { return false; }
    virtual bool getFP32YcData(const uint16_t a, float &v, bool c=false) { return false; }
    virtual bool getValidYmData(const uint16_t a, int32_t &v, bool c=false) { return false; }
    virtual bool getValidYtData(const uint16_t a, uint16_t &v, bool c=false) { return false; }
    virtual bool getNodeVersion(const canNodeType t, const uint8_t n, NodeInfo &i) { return false; }
    virtual bool getNodeVersion(const ModbusNodeType t, const uint8_t n, NodeInfo &i) { return false; }
    virtual bool getNodeVersion(const Rise3501NodeType t, const uint8_t n, NodeInfo &i) { return false; }
    virtual bool getNodeInfoCh(NodeInfo &s) { return false; }
    virtual bool readEvent(SpontEvent &e) { return false; }
    virtual bool getYcFloatEnable(bool &e) { return false; }
    virtual bool setYmFreezeConfirm(bool f) { return false; }
    virtual bool setYmCallEnd(bool f) { return false; }
    virtual bool setSingleYkCmd(const SpontEvent &e) { return false; }
    virtual bool setDoubleYkCmd(const SpontEvent &e) { return false; }
    virtual bool setYcParam(const SpontEvent &e) { return false; }
    virtual bool setUpdate(const SpontEvent &e) { return false; }
    virtual bool setDBRecord(const SpontEvent &e) { return false; }
    virtual bool callPart(const SpontEvent &e) { return false; }
    virtual SpontEvent setYt(const SpontEvent &e) { return e; }
    virtual SpontEvent setGroupYt(const SpontEvent &e) { return e; }
    virtual void saveParam() {}
    virtual bool manualWave(const SpontEvent &e) { return false; }
    virtual bool FreezeYm(const uint8_t RQT, const uint8_t FRZ) { return false; }
    virtual bool setSingleYxStat(const uint16_t a, uint8_t s) { return false; }
    virtual bool setDoubleYxStat(const uint16_t a, uint8_t s) { return false; }
    virtual bool setValidYcValue(const uint16_t a, uint16_t v) { return false; }
    virtual bool setDataPoll(bool s, bool n=true) { return false; }
    virtual bool easyCmd(const SpontEvent &e) { return false; }
protected:
    BaseDataConfig m_config;
    BaseDataConfig m_baseAddr;
    string         m_moduleName;
    uint8_t        m_moduleIndex;
};

#endif // _RtuBaseClass_H_
