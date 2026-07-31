/*************************************************
  Copyright (C), 2012- , Beijing Togest Automation System Equitment Co.,Ltd
  File name:        CANProtocol.h
  Author:           李佳臻     
  Version:          1.0       
  Date:             2012-11-26
  Description:      该文件声明ModBus协议模块的各项操作
  Others:           
  Function List:    
    1. 
  History:         
    1. Date:        2012.11.26
       Author:      Li Jiazhen
       Modification:
    2. Date:        2013.03.20
       Author:      Li Jiazhen
       Modification:重新构建配置文件使之能更加的具有通用性,相应的修改程序代码
        使用根据需求数据去处理输入数据的方式来完成
*************************************************/

#ifndef _MODBUSPROTOCOL_H_
#define _MODBUSPROTOCOL_H_

// removed
#include "stl_package.h"
#include "log_manage.h"
#include "timer_manage.h"
#include "date_manage.h"
#include "xml_parser.h"
#include "../mFileManage/filemanage.h"
#include "../mMsgProtocol/msgProtocol.h"
#include "serial_module.h"
#include "../RtuBaseClass.h"
#include "crc_make.h"

//--------------------------------------------------
#define MODBUSMASTERCONFIG  "../conf/ModBusMasterConfig.xml"                     //MBUS各项配置文件

#define mAbs(x, y)  (((x) > (y)) ? ((x) - (y)) : ((y) - (x)))

//--------------------------------------------------
//ModBus协议功能码定义
#define FUNC_RD_COIL            0x01                                            //读取线圈状态
#define FUNC_RD_INPUT           0x02                                            //读取输入状态
#define FUNC_RD_HOLDREG         0x03                                            //读取保持寄存器数据
#define FUNC_RD_INPUTREG        0x04                                            //读取输入寄存器数据
#define FUNC_YK                 0x05                                            //控制单线圈
#define FUNC_WR_SIGREG          0x06                                            //写单个寄存器
#define FUNC_YK_MULT            0x0F                                            //控制多个线圈
#define FUNC_WR_MULTREG         0x10                                            //写多个寄存器
#define FUNC_RD_FILE            0x14                                            //读文件记录
#define FUNC_RD_MIX				0x5A											//混合帧读取
#define FUNC_UPDATE_BOTTOM      0x55                               				//在线升级底板
#define FUNC_UPDATE_ROOF        0x56                                			//在线升级顶板
#define FUNC_RECORD_NUM         0x61                                			//事件记录数目
#define FUNC_RECORD             0x60                                    		//事件记录实体
#define FUNC_RECORD_CLEAN       0x62                                			//清除事件记录
#define FUNC_SOE				0x16											//遥信soe事件记录

#define FILENUMERRREPORT        0x01                                            //故障报告文件号
#define FILENUMWAVEDIR          0x02                                            //录波文件目录召取
#define FILENUMWAVECALL         0x03                                            //录波文件召取的基文件
#define FILENUMSOENUM			0x02											//soe数目读取

#define WAVEFRAMEHEAD           5                                               //接收的录波数据帧帧头  
#define WAVEFRAMECRC            2                                               //接受的录波数据帧CRC校验     
#define TIMEFRAMEADD            5                                               //增加的时间帧(分时日月年毫秒)
//--------------------------------------------------
//ModBus协议异常代码定义
#define EXP_ILLEGAL_FUNC        0x01                                            //不支持该功能码
#define EXP_DATA_ADDR           0x02                                            //无效的寄存器地址
#define EXP_DATA_VALUE          0x03                                            //无效的数据值
#define EXP_SLAVE_FAIL          0x04                                            //从站执行请求的功能时发生错误
#define EXP_ACKNOWLEDGE         0x05                                            //执行长耗时功能时先进行有效应答
#define EXP_SLAVE_BUSY          0x06                                            //从站忙
#define EXP_NEGATIVE_ACK        0x07                                            //从站无法正确执行该功能
#define EXP_MEM_ERROR           0x08                                            //从站读取扩展内存是出错

//--------------------------------------------------
//录波文件结构
//#define WAVE_SMP_POINT  8                                                       //遥测采样点每周波
//#define WAVE_RCD_PERID  40                                                      //每通道录取周波数
//#define WAVE_RCD_CHANL  12                                                      //总共录波通道数
//#define WAVE_EXT_DATA   40                                                      //录波文件额外信息参见CAN协议定义
//#define WAVE_RCD_LEN    (WAVE_RCD_CHANL*WAVE_RCD_PERID*WAVE_SMP_POINT*2)+WAVE_EXT_DATA

//广播地址0xff
#define BOARDCAST_ID    0xff	

//Modbus主备
#define BACKUP_SYN_ID   0                                                       //主备同步ID
#define BACKUP_SYN_FUNC 0x70                                                    //主备同步功能码

#define UPDATE_TYPE_MASK  0x81													
#define UPDATE_TYPE_FILTER 0x7E

#define UPDATE_SERIAL_0	(0x01 << 1)	
#define UPDATE_SERIAL_1 (0x02 << 1)
#define UPDATE_SERIAL_2 (0x03 << 1)
#define UPDATE_SERIAL_3 (0x04 << 1)
#define UPDATE_SERIAL_4 (0x05 << 1)
#define UPDATE_SERIAL_5 (0x06 << 1)


//记录号类型，由于单点遥信和双点遥信定义在同一个报文内，因此使用此定义
#define MD_SINGLEYX_SOE		0x104
#define MD_DOUBLEYX_SOE		0x103	

//文件号
#define MD_FILE_NUM_SOE_NUM	0x5001
#define MD_FILE_NUM_SOE		0x6001

//主备超时时间
#define ACTIVE_STANDBY_TIME_OUT		60000

//定义应答类型
enum MBusRspType
{
    rspNoReq,                                                                   //无请求
    rspReadReq,                                                                 //读请求
    rspWriteReq,                                                                 //写请求
    rspReadFile,                                                                 //读文件
    rspUpdate,                                                                  //在线升级
    rspReadRecord,                                                      		//召取事件记录
    rspReadSoe,																	//读取soe
    rspNoDeal
};

//定义应答状态
enum MBusRspStat
{
    rspWaiting,                                                                 //等待应答中
    rspFinished                                                                 //应答结束(超时,正确、错误应答)
};

//定义帧类型
enum MBusFrameType
{
    frameNoType,                                                                //无该种帧类型
    frameSingleYx,                                                              //遥信
    frameDoubleYx,                                                              //双点遥信
    frameYc,                                                                    //遥测
    frameFloatYc,                                                               //短浮点遥测值
    frameInt32Yc,                                                               //32位整型遥测值转为float型传输
    frameYm,                                                                    //遥脉
    frameYt,                                                                    //参数量
    frameYk,                                                                    //遥控
    frameWR,                                                                    //写命令帧
    frameMultiYk,                                                               //多路遥控控制
    frameMultiWR,                                                               //预置多个寄存器
    frameYkSBOSelect,                                                           //遥控选择
    frameReadErrReport,                                                         //遥测故障报告
    frameReadDir,                                                               //录波文件目录
    frameReadWave,                                                              //录播文件召取
    frameUpdate,                                                                //在线升级
    frameRecord,                                                                //招取事件记录
    frameMix,																	//混合型参数
    frameYxSoe,																	//遥信soe
    frameYxSoeNum,																//遥信soeNum
    frameSetTime,                                                                //授时
    frameInt8Yc,																//8位整型遥测值转为float型传输
    frameInt16Yc																//16位整型遥测值转为float型传输
};

//定义混分帧的类型的typeCode
enum MBMixFrameTypeCode{
	MixFrameSingleYx = 1,													    //混合帧单点遥信代码
	MixFrameDoubleYx, 														    //混合帧双点遥信代码
	MixFrameYc,																    //混合帧遥测遥信代码
	MixFrameYt,																    //混合帧参数代码
	MixFrameReadErrReport,													    //混合帧故障报告代码
	MixFrameReadDir,														    //混合帧读目录代码
	MixFrameNoType,															    //混合帧无类型
};

//定义混合帧类型
enum MBusMixFrameType
{
	frameMixSingleYxType = 1,	
	frameMixDoubleYxType,
	frameMixYcType,
	frameMixYtType,
	frameMixReadErrReportType,
	frameMixReadDirType,
};

//定义解析的数据类型
enum MBusParseType{
	frameParseDataNoType = 1,
	frameParseDataVersion,													    //版本号类型
};

//定义data格式
struct MBusParseData                                                            
{
	MBusParseData(){
		regIndex = 0;
		THV = 0;
		dataType = frameParseDataNoType;
		compensate = 0;
		factor = 0.0;
	}
    uint8_t   regIndex;                                                           //数据单元在数据流中序号
    uint16_t  THV;                                                                //门限值
    MBusParseType	dataType;													//数据类型
    int16_t  compensate;                                                         //加减型补偿系数
    float  factor;																//比列系数
};
struct MBusParseSingleYx : public MBusParseData                                 //单点遥信
{
	MBusParseSingleYx(){
		bitIndex = 0;
	}
    uint8_t  bitIndex;                                                            //数据单元位序号
};
struct MBusParseErrReport
{
	MBusParseErrReport(){
		cirIndex = 0;
		deviceId = 0;
	}
	uint8_t cirIndex;																//回路顺序
	uint8_t deviceId;																//设备地址
};

struct MBusParseDoubleYx: public MBusParseData                                  //双点遥信
{
	MBusParseDoubleYx(){
		onBitIndex = 0;
		offBitIndex = 0;
		onRegIndex = 0;
		offRegIndex = 0;
	}
    uint8_t  onBitIndex;                                                          //合位序号
    uint8_t  offBitIndex;                                                         //分位序号
    uint8_t  onRegIndex;                                                          //合数据单元序号
    uint8_t  offRegIndex;                                                         //分数据单元序号
};

struct MbusParseSoeType
{	
	MbusParseSoeType(){
		singleYx = 0;
		doubleYx = 0;
	}
	uint16_t   singleYx;
	uint16_t   doubleYx;
};


struct MBusYxSoeBaseAddr
{
	MBusYxSoeBaseAddr(){
		singleYxBaseAddr = 0;
		doubleYxBaseAddr = 0;
	}
	uint16_t					singleYxBaseAddr;
	uint16_t					doubleYxBaseAddr;
};


struct MBusYxSoeNum
{
	MBusYxSoeNum()
	{
		singleYxNum = 0;
		doubleYxNum = 0;
	}
	uint16_t					singleYxNum;
	uint16_t					doubleYxNum;
};


//定义Frame帧格式
struct MBusRequest
{
	MBusRequest()
	{
		type = frameNoType;
		id = 0;
		deviceAddr = 0;
		funcode = 0;
		startAddr = 0;
		regNum = 0;
		refType = 0;
		fileNum = 0;
		recordLenth = 0;
		recordNum = 0;
		dataNum = 0;
		dataOffset = 0;
		deviceIndex = 0;
	}
    MBusFrameType           type;                                               //请求帧类型
    uint8_t                   id;                                                 //帧序列号
    uint8_t                   deviceAddr;                                         //设备地址
    uint8_t                   funcode;                                            //请求帧功能码
    uint16_t                  startAddr;                                          //数据地址
    uint16_t                  regNum;                                             //长度
    uint8_t                   refType;                                            //文件引用类型
    uint16_t                  fileNum;                                            //文件号 
    uint16_t                  recordLenth;                                        //记录长度
    uint16_t                  recordNum;                                          //记录号数目

	MBusYxSoeBaseAddr		soeBaseAddr;										//soe基址
	MBusYxSoeNum			soeNum;												//soe数目
	std::vector<MBusRequest>		subMBusRequest;										//混合帧格式
	
    uint16_t                  dataNum;                                            //总data数目
    uint16_t                  dataOffset;                                         //data在数据表和数据解析表中的起始序列号
    uint8_t                   deviceIndex;                                        //设备序号
    bool operator==(const MBusRequest &req)
    {
		type = req.type;                                               
		id = req.id;                                                 
		deviceAddr = req.deviceAddr;                                         
		funcode = req.funcode;                                            
		startAddr = req.startAddr;                                          
		regNum = req.regNum;                                             
		refType = req.refType;                                            
		fileNum = req.fileNum;                                            
		recordLenth = req.recordLenth;                                        
		recordNum = req.recordNum;   
		memcpy(&soeBaseAddr, &req.soeBaseAddr, sizeof(soeBaseAddr));
		memcpy(&soeNum, &req.soeNum, sizeof(soeNum));							
												
		subMBusRequest = req.subMBusRequest;										
		dataNum = req.dataNum;                                            
		dataOffset = req.dataOffset;                                         
		deviceIndex = req.deviceIndex;  
		return true;
    }
};

//写命令请求定义
struct MBusYkType
{
	MBusYkType(){
		addr = 0;
		action = 0;
	}
    uint16_t                  addr;
    uint16_t                  action;
};
struct MBusDoubleYk
{
    MBusYkType              ykOn;
    MBusYkType              ykOff;
};

//定义写请求
struct MBusWriteRequest
{
	MBusWriteRequest(){
		deviceAddr = 0;
		funCode = 0;
		num = 0;
		baseAddr = 0;
		readIndex = 0;
		delayReadTime = 0;
	}
    uint8_t                   deviceAddr;                                         //设备地址
    uint8_t                   funCode;                                            //功能码
    uint16_t                  num;                                                //数目
    
    uint16_t                  baseAddr;                                           //基址
    uint16_t                  readIndex;                                          //写操作完成后读取读队列的序号
    uint16_t                  delayReadTime;                                      //写操作完成后发器读取延时时间ms
};

//定义读文件请求
struct MBusReadFileRequest
{
	MBusReadFileRequest(){
		type = frameNoType;
		deviceAddr = 0;
		funCode = 0;
		num = 0;
		baseAddr = 0;
		quoteType = 0;
		fileNum = 0;
		recordNum = 0;
		recordLength = 0;
	}
	MBusFrameType           type;                                               //请求帧类型
    uint8_t                   deviceAddr;                                         //设备地址
	uint8_t                   funCode;                                            //功能码	
	uint16_t					num;												//数目
	uint16_t					baseAddr;											//基址
	MBusYxSoeBaseAddr		soeBaseAddr;										//soe基址
	MBusYxSoeNum			soeNum;												//soe数目
	
	uint8_t					quoteType;											//引用类型
	uint16_t					fileNum;											//文件号
	uint16_t					recordNum;											//记录号
	uint16_t					recordLength;										//记录长度
};

//定义遥控命令
struct MBusWriteYk : public MBusWriteRequest
{
    std::vector<MBusDoubleYk>    doubleYk;
};

//定义多点遥控命令
struct MBusMultiYkType
{
	MBusMultiYkType(){
		startAddr = 0;
		num = 0;
	}
    uint16_t                  startAddr;
    uint16_t                  num;                                                //func=15:点数,func=16:寄存器数目
    std::vector<uint16_t>          action;
};
struct MBusMultiWriteYk : public MBusWriteRequest
{                                               
    MBusMultiYkType         ykOn;
    MBusMultiYkType         ykOff;
    MBusMultiYkType         ykAbort;
};

//定义写寄存器
struct MBusWriteReg : public MBusWriteRequest
{
    std::vector<uint16_t>          regAddr;                                            //寄存器列表
};

struct MBusYt
{
	MBusYt(){
		ytUintAddrOffset = 0;
		readIndex = 0;
	}
	MBusRequest				request;
	uint16_t					ytUintAddrOffset;
	uint16_t                  readIndex;                                          //参数写完后读取的队列序号
};

//定义数据表
struct MBusData
{
    std::vector<DataINT8U>       singleYx;                                           //单点遥信
    std::vector<DataINT8U>       doubleYx;                                           //双点遥信
    std::vector<DataINT16S>      Yc;                                                 //16位遥测
    std::vector<DataINT32S>      Ym;                                                 //计量值
    std::vector<DataFP32>        floatYc;                                            //短浮点遥测值
    std::vector<DataINT16S>      yt;                                                 //参数量
};

//定义数据解析表
struct MBusDataParse
{
    std::vector<MBusParseSingleYx>   singleYx;
    std::vector<MBusParseDoubleYx>   doubleYx;
    std::vector<MBusParseData>       yc;
    std::vector<MBusParseData>       ym;
    std::vector<MBusParseData>       floatYc;
    std::vector<MBusParseData>       yt;
	std::vector<MBusParseData>		mix;
	std::vector<MBusParseErrReport>  errReport;
};


//定义配置
struct MBusConfig
{
	MBusConfig(){
		pollPeriod = 0;
		norspTime = 0;
		ycCheckPeriod = 0;
		ykNum = 0;
		writeRegs = 0;
		ykTimeOut = 0;
		waveTimeOut = 0;
		timeSetPeriod = 0;
		ackSendTime = 0;
		reqDelaySendTime = 0;
		setYcTHV = 0;
		setTime = 0;
		commStatReport = 0;
		commStatTrytimes = 0;
		deviceNum = 0;
		bocastUpdateOverTime = 0;
		serialPort = 0;
		readFileLengthReduceCounter = 0;
	}	
    uint8_t                   pollPeriod;                                         //轮询周期
    uint8_t                   norspTime;                                          //未响应超时时间
    uint8_t                   ycCheckPeriod;                                      //遥测变位检测周期
    uint16_t                  ykNum;                                              //遥控数目
    uint16_t                  writeRegs;                                          //写寄存器数目
    uint8_t                   ykTimeOut;                                          //遥控超时
    uint8_t					waveTimeOut;										//录波文件超时
    uint16_t                  timeSetPeriod;                                      //授时周期
    uint16_t					ackSendTime;										//从机应答后延时发送时间
    uint8_t                   reqDelaySendTime;                                   //命令帧延时发送时间
    bool                    setYcTHV;                                           //是否添加遥测限值到参数设置列表
    bool                    setTime;                                            //是否进行时间同步
    bool                    commStatReport;                                     //通信状态软遥信上报
    uint16_t                  commStatTrytimes;                                   //通信尝试次数
    uint8_t                   deviceNum;
	uint16_t					bocastUpdateOverTime;								//广播升级超时
	uint8_t					serialPort;											//串口号
	uint8_t					readFileLengthReduceCounter;						//兼容2.99非标准读文件协议帧长度计算	
};

//定义轮询命令
struct MBusReqRecord
{
	MBusReqRecord(){
		pollReqPtr = 0;
		rspStat = rspWaiting;
	}
    uint16_t                  pollReqPtr;
    MBusRspStat             rspStat;                                            //请求帧应答状态
};

//定义请求队列状态记录
struct MBusReqFifo
{
    MBusReqFifo() : 
        reqType(rspNoReq), reqFrameType(frameNoType), reqPtr(0), reqDelaySend(0),
        reqAckDelaySendTime(0)
    {}
    MBusRspType             reqType;                                            //请求帧类型
    MBusFrameType           reqFrameType;                                       //请求帧类型
    uint16_t                  reqPtr;                                             //请求帧在相应帧列表中的位置
    uint16_t                  reqDelaySend;                                       //命令帧延时发送时间
    uint16_t                  reqAckDelaySendTime;                                //命令帧应答延时发送时间
    SerialNetBuf            reqFrame;                                           //命令帧队列
};

enum YcWaveType
{
    waveSMP,                                                                //瞬时采样值
    waveRMS                                                                 //有效值
};


//---------------在线升级添加---------------------------
#define UPDATE_MAX_RETRY_NUM   3               //最大错误计数


enum ModbusUpdateStatCode       //在线升级状态码
{
    ModbusUpdateStCodeSuccessed,                                               //升级成功
    ModbusUpdateStCodeBusy,                                                    //正在升级中
    ModbusUpdateStCodeFileParseErr,                                            //升级文件解析错误
    ModbusUpdateStCodeNodeNotReady,                                            //节点未准备好
    ModbusUpdateStCodeFailed,                                                   //升级失败
    ModbusUpdateStCodeNone,
};


//在线升级超时时间
enum ModbusUpdateTimeOut
{
    ModbusUpdateWaitTime = 10,                                                 //在线升级命令下发的响应超时时间
    ModbusUpdateYcTransTime = 180                                              //遥测启动数据传输的回应超时时间
};

//在线升级功能字
enum UpdateCmd
{
    updateCmdError = 0,                         //错误处理
    updateCmdStart = 'S',                       //启动在线升级
    updateCmdErase = 'E',                       //擦除FLASH
    updateCmdWrite = 'W',                       //烧写FLASH
    updateCmdTrans = 'D',                       //传输数据
    updateCmdReset = 'R',                       //复位芯片
    updateCmdNone  = 'N'						//等待
};

//在线升级异常码
enum UpdateExceptionCode
{
    updateErrNormal,                            //正常响应
    updateErrBusy,                              //正在升级中
    updateErrEraseFailed,                       //擦除失败
    updateErrWriteFailed,                       //烧写失败
    updateErrDataNoMatched,                     //接收的数据起始地址和已接收到的数据长度不一致
    updateErrResponseTimeout,                   //接收超时
    updateErrCRCError                           //CRC错误
};

struct ModbusUpdate
{
	ModbusUpdate(){
		type = MBottom;
		node = 0;
		status = ModbusUpdateStCodeSuccessed;
		updateFlag = updateCmdError;
		dataLen = 0;
		d_data = NULL;
		sendCount = 0;
		prevSendCount = 0;
		segNum = 0;
		segRepeat = 0;
	}
    DBNodeType     type;                                                        //在线升级板卡类型
    uint8_t           node;                                                       //节点号
    ModbusUpdateStatCode           status;                                      //在线升级状态
    UpdateCmd       updateFlag;                                                 //在线升级功能字
    uint32_t          dataLen;                                                    //数据长度
    uint8_t          *d_data;                                                     //数据存储缓冲区
    uint32_t          sendCount;                                                  //目前已发送的数据数目
    uint8_t           prevSendCount;                                              //上一次发送的数据数目
    uint32_t          segNum;                                                     //段序号
    uint8_t           segRepeat;                                                  //重复计数
};

struct ModbusSoeCheckPCB
{
	ModbusSoeCheckPCB(){
		soeNumTimeOutCounter = 0;
		soeTimeOutCounter = 0;
		isSoeNumCheck = false;
		isSoeCheck = false;
		SoeNumZeroCounter = 0;
	}
	uint8_t soeNumTimeOutCounter;														//soe数目轮训帧超时次数
	uint8_t soeTimeOutCounter;														//soe轮训帧超时次数
	MbusParseSoeType soeAckNum;														//soe应答数目
	bool isSoeNumCheck;																//soeNumCheck查询状态(1.下行存了多个soe 2.下行存的soe数量与请求的一致)
	TimerService yxSoeCheck;														//Soe数目查询定时器
	bool isSoeCheck;																//soeCheck查询状态

	uint8_t SoeNumZeroCounter;															//响应帧soe数目为零的次数
};


enum ModbusReadRecordStatCode       //招取事件记录状态码
{
    ModbusRecordStCodeSuccessed,                                               //招取成功
    ModbusRecordStCodeBusy,                                                    //招取中
    ModbusRecordStCodeFailed,                                                   //招取失败
    ModbusRecordStCodeNone,
};

//Modbus各个节点板卡的版本信息

struct ModbusVersion
{
	ModbusVersion(){
		nodeVersion = 0;
		stat = 0;
		oldStat = 0;
		offlineCnt = 0;
		deviceId = 0;
		
	}
    uint16_t          nodeVersion;                                                //uint16_t 高8位主版本号, 低8位副版本号
    uint8_t           stat;                                                       //当前在线状态
    uint8_t           oldStat;                                                    //上一次在线状态
    uint8_t           offlineCnt;                                                 //离线全召计数
    uint8_t 			deviceId;													//设备地址
};

struct ModbusRecord
{
    ModbusRecord()
    {
		WaitTime = 2;
		status = ModbusRecordStCodeNone;
		segRepeat = 0;
		node = 0;
		SumCount = 0;
		ReadCount = 0;
		d_data = NULL;
	}
    uint8_t          WaitTime;                    //超时时间
    uint8_t           node;                           //节点号
    ModbusReadRecordStatCode  status;         //状态
    uint8_t           segRepeat;                  //重复计数
    
    uint16_t          SumCount;              //总记录条数
    uint16_t           ReadCount;          //已读取的数目
    uint8_t           *d_data;                        //数据缓冲区
};

struct ModbusSoe{
	ModbusSoe(){
		soeType = 0;

	}
	DataSoe dataSoe;								//soe数据
	uint8_t	soeType;								//soe类型:单点或者双点
};

class ModBusProtocol : public SerialProtocol
{
public:
    ModBusProtocol(int baseCir = 0);
    ModBusProtocol(const std::string &fileName, int baseCir = 0);
    ~ModBusProtocol();
    
    void    loadConfig();
    BaseDataConfig getConfig();

    void    process();
    void    setBaseCircuitAddr(int baseCircuitAddr);
    virtual bool    recvFrame(const SerialNetBuf &frame);
    virtual bool    sendFrame(SerialNetBuf &frame) {return m_sendFrameFifo.popFront(frame);}
	bool 	getNodeVersion(const ModbusNodeType type, const uint8_t cpuNum, NodeInfo &info);
    bool    getNodeInfoCh(NodeInfo &infoSoe);
    bool    getDoubleYxSoe(DataSoe &soe);                                       //双点遥信
    bool    getSingleYxSoe(DataSoe &soe);                                       //单点遥信
    bool    getValidYcSoe(DataSoe &soe);                                        //AC遥测
    bool    getFP32YcSoe(DataFP32Soe &soe);
    bool    getSingleYxData(const uint16_t addr, uint8_t &value, bool change = false);
    bool    getDoubleYxData(const uint16_t addr, uint8_t &value, bool change = false);
    bool    getValidYcData(const uint16_t addr, uint16_t &value, bool change = false);
    bool    getValidYmData(const uint16_t addr, int32_t &value, bool change = false);
    bool    getFP32YcData(const uint16_t addr, float &value, bool change = false);
    bool    getEvent(SpontEvent &event);
    bool    setDoubleYkCmd(const SpontEvent &event);
    bool    setSingleYkCmd(const SpontEvent &event);
    bool    setYcParam(const SpontEvent &event);
    SpontEvent setParam(const SpontEvent &event); 
    bool    setBroadcastMultiYk(uint8_t nodeId, uint16_t addr, 
                                uint8_t points, uint8_t action);                    //列控专用
    void    setFileManage(FileManage *manage, RtuFileType type);                //录波使用
    void    setCommStat(uint16_t deviceNo, bool stat, char *ip = NULL);
    bool    setUpdate(const SpontEvent &event);                                 //在线升级
    bool    ReadRecord(const SpontEvent &event);                                //招取事件记录
    bool    getPeerUpdatingStat();
    int     getCommRetryTime() {return m_config.commStatTrytimes;}              //获取通信尝试次数
    MBusData & getMBusData() { return m_data;}
    void    setRecvErrPointer(uint16_t *errCount, uint16_t *resetCount) 
    {
        d_recvErrCount = errCount;
        d_resetCount   = resetCount;
    }

private:
    void    init();
    void    initConfig();
    void    initData();
    void    initVector();                                                       //大量数据使用vector时需在此函数中释放掉多余的内存
    bool    sendMBusFrame(SerialNetBuf &frame);
	void 	initVersion();
    
public:                                                                        //接收帧处理
    void    poll();
    //bool    dealMBusFrame();
	bool    dealMBusFrame(char* ipv4 = NULL);
    //bool    protocolOccured();
    bool    protocolOccured(char* ipv4 = NULL);
    void    preocessParamSet();
private:	 
    void    processReadRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd);
    void    processWriteRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd);
    void    processReadFileRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd, char *devIpv4 = NULL);
    void    processYxEvent(const MBusRequest &request, const SerialNetBuf &frame);
    void    processMixYxEvent(const MBusRequest &request, const SerialNetBuf &frame);
    void    processYcEvent(const MBusRequest &request, const SerialNetBuf &frame);
    void    processYmEvent(const MBusRequest &request, const SerialNetBuf &frame);
    void    processB32YcEvent(const MBusRequest &request, const SerialNetBuf &frame);
	void 	processB16YcEvent(const MBusRequest &request, const SerialNetBuf &frame);
	void 	processB8YcEvent(const MBusRequest &request, const SerialNetBuf &frame);


    void    processYtEvent(const MBusRequest &request, const SerialNetBuf &frame);
    void    processYkEvent(const SerialNetBuf &frame);
    void    processYkSelectEvent(const SerialNetBuf &frame);
    void    processWriteEvent(const SerialNetBuf &frame);
    void    processMultiWriteEvent(const SerialNetBuf &frame);
    void    processframeYcErrReportEvent(const MBusRequest &request,const SerialNetBuf &frame);
    void    processframeYcWaveDir(const MBusRequest &request,const SerialNetBuf &frame);
	void 	processframeMix(const MBusRequest &request,const SerialNetBuf &frame);
    void    processframeYcWaveCall(const SerialNetBuf &frame, char *devIpv4 = NULL);
	void 	processframeYxSoeNum(const SerialNetBuf &frame);
	void  	processframeYxSoe(const MBusReadFileRequest &request,const SerialNetBuf &frame);
    void    waveFileConfirm(uint16_t reqptr);
    void    dealWaveFileConfirm(const SerialNetBuf &frame);
    bool    saveYcWaveFile(char *devIpv4,uint8_t deviceAddr);
    void    setTimePeriod();
    
    void    pollSendReq();
    bool    sendWriteYkSelect(MBusFrameType type, uint16_t startaddr, std::vector<uint16_t> &data);
    void    sendWriteYk(MBusFrameType type, uint16_t startaddr, std::vector<uint16_t> &data);
    void    sendMutiWriteReg(MBusFrameType type, uint16_t startaddr, std::vector<uint16_t> &data);
    void    SendReqCmd();
    bool    setFrame(uint16_t reqptr, uint16_t delayTime = 0);
	void 	sendYtReg(MBusFrameType type, uint16_t addrOffset, std::vector<uint16_t> &data);
    void 	sendYtReg(uint8_t deviceID, uint16_t addr, uint16_t value);
	void 	sendReadYxSoeNum(MBusFrameType type,uint16_t yxaddr, uint8_t soeType);
	void 	sendReadSYxSoe(MBusFrameType type, uint16_t soeNum, uint16_t yxaddr, uint8_t soeType);
	bool 	sendReadSoeFile(uint8_t soeNum,uint8_t deviceID);

public:
	bool 	setDataPoll(bool setStop, bool notify = true);
    
private:
    template <typename vectorType> friend void freeVectorSpare(std::vector<vectorType> &data);
    void    setParamConfig(const SpontEvent &event);
    void    setDBParam(const SpontEvent &event);
    void    getParamConfig(const SpontEvent &event);
    bool    saveParamConfig(
                            uint8_t node, 
                            uint8_t offset, 
                            uint8_t qpm, 
                            uint16_t value
                           );
    void    putSoeToFifo(uint16_t addr, int16_t value, STLDeque<DataSoe> &soefifo);
	void 	putSoeToFifo(uint16_t addr, int16_t value, uint8_t type, STLDeque<DataSoe> &soefifo);
	void 	putSoeToFifoWthTime(uint16_t addr, int16_t value, STLDeque<DataSoe> &soefifo, DateType time);
    MBusFrameType getFrameType(const char *type);
	MBusParseType getParseFrameType(const char *parseType);
	MBMixFrameTypeCode getMixFrameCode(const char *type);

private:        //在线升级使用
    bool    composeUpdateStart(uint8_t node, DBNodeType type);    //在线升级开始
    void    composeUpdateErase(uint8_t node, DBNodeType type);    //擦除FLASH
    bool    parseUpdateFile(const char *filename);
    bool    composeUpdateTrans(uint8_t node, DBNodeType type, UpdateExceptionCode errCode);
    void    composeUpdateWrite(uint8_t node, DBNodeType type);    //写FLASH
    void    composeUpdateReset(uint8_t node, DBNodeType type);    //复位
    void    processUpdateRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd);
    void    UpdateStatReport(void);
	void 	simDownAckFrame(UpdateCmd _updateCmd);

private:        //招取事件记录使用
    bool composeReadRecordNum(uint8_t node);                      //组帧  读事件记录数目
    bool composeRecordNumClean(uint8_t node);                     //清除事件记录
    bool composeReadRecord(uint16_t addr, uint8_t num);             //组帧  招取事件记录
    void processRecordRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd, char *devIpv4);     //处理单板响应帧
    void reportErr(bool stat);                                  //上报事件
    void ReSendFrame(void);                                     //重发
    void getYt(const SpontEvent &event);
	void setYt(const SpontEvent &event);
	uint8_t getSerialIndex(uint8_t serialNum);

public:
	//void  setDoubleBackup(DoubleBackup *backup);
	//DoubleBackup *getDoubleBackup(){return d_doubleBackup;}
	uint16_t eqAckMatch(const SerialNetBuf &rcvframe);
	bool  dealFramePre(MBusReqFifo  &reqcmd);

protected:
    std::string              m_configFile;
    BaseDataConfig      m_baseDataConfig;
    MBusConfig          m_config;
    std::vector<MBusRequest> m_readRequest;                                          //轮询请求列表
    std::vector<MBusReadFileRequest>m_readFileRequest;								//读文件请求
    std::vector<MBusWriteYk> m_writeYK;                                              //遥控
    std::vector<MBusMultiWriteYk> m_writeMultiYK;                                    //多点遥控
    std::map<uint16_t, MBusMultiWriteYk> m_selectYK;                                   //遥控地址映射遥控选择
    std::vector<MBusWriteReg>m_writeReg;                                             //写寄存器
    std::vector<MBusYt>		m_ytReg;												//写遥调
    MBusData            m_data;                                                 //数据表 
    MBusDataParse       m_dataParse;                                            //数据解析表
    MBusReqRecord       m_reqRecord;                                            //应答记录
    canYkStatus         m_ykStatus;                                             //遥控执行状态
    std::vector<uint8_t>       m_deviceRetryCount;                                     //设备重试计数
    std::map<int, int>       m_deviceIdToCommstatAddr;                               //设备序号映射通信addr
    
    STLDeque<MBusReqFifo>   m_reqFifo;                                          //请求帧列表
    STLDeque<SerialNetBuf>  m_sendFrameFifo;                                    //发送帧队列
    STLDeque<SerialNetBuf>  m_recvFrameFifo;                                    //接收帧队列
    STLDeque<DataSoe>       m_singleYxSoeFifo;                                  //单点遥信SOE队列
    STLDeque<DataSoe>       m_doubleYxSoeFifo;                                  //双点遥信SOE队列
    STLDeque<DataSoe>       m_dYxSoeFifo;                                		//遥信SOE队列,用于触发向下发送查询soe数目帧
    STLDeque<DataSoe>       m_validYcSoeFifo;                                   //遥测SOE队列
    STLDeque<DataFP32Soe>   m_FP32YcSoeFifo;                                    //32位遥测SOE队列
    STLDeque<DataSoe>       m_ymSoeFifo;
    STLDeque<NodeInfo>      m_nodeInfoSoeFifo;
	std::vector<ModbusVersion>	m_version;											//版本号控制
	
    SpontEvent              m_ykCmdEvent;                                       //遥控事件
    SpontEvent              m_setParamEvent;                                    //遥测参数设置事件
    SpontEvent              m_updateEvent;                                      //在线升级事件
    SpontEvent              m_readRecord;                                       //招取事件记录事件
    STLDeque<SpontEvent>    m_setParamEventList;                                //缓存参数设置事件
    bool                    m_isParamSetting;                                   //正在设参
    SpontEvent				m_waveEvent;										//录波事件
    
    STLDeque<SpontEvent>m_ycErrReportEvent;                                     //遥测故障报告事件
    std::vector<std::string>      m_waveCause;                                            //录波原因

	TimerService              t_yxSoe;                                          //遥信soe处理
    TimerService              t_rsp;                                            //slave超时定时器
    TimerService              t_poll;                                           //轮询定时器
    TimerService              t_Yk;                                             //遥控超时定时器
    TimerService              t_write;                                          //写定时器
    TimerService              t_delaySend;                                      //延迟发送定时器
    TimerService              t_SetTime;                                        //授时帧帧定时器
    TimerService              m_update;                                         //在线升级响应超时定时器
    TimerService			  m_brocastUpdate;									//广播升级超时
    TimerService              m_Record;                                         //事件记录招取响应超时定时器
    TimerService			  t_wave;											//录波文件超时定时器
    TimerService            t_RcvOver;                                  //字节接收超时

 private:
    //static uint8_t                  ycWaveNum;                                  //目录录波文件数量  
    std::vector<SerialNetBuf>          rcvWaveData;                                  //录波曲线数据帧
    uint32_t                        fileLength;                                   //录波曲线大小
    uint32_t                        waveFrameCounter;                             //录波曲线接收帧数量(整数部分)
    FileManage                    *d_waveFileManage;
    FileManage                    *d_webWaveFile;
    int                           m_baseCir;                                    //遥测回路基址
    uint8_t                         parseMode;                                    //解析模式(光芒为非0，南凯为0)
    bool                          waveWriteSuc;                                 //文件创建成功
    uint32_t                        waveFrameNum;                                 //录波数据帧数量
    uint8_t                         waveTimeOutCounter;                           //录波传输超时计数器
    bool                         dataZero;                                      //数据帧数据为0
    ModbusUpdate      			 m_updateBuff;                         			//在线升级buff
    ModbusRecord      			 m_RecordBuff;                            		//招取事件记录buff
	uint8_t 						 m_waveDeleteId;
	std::map<uint8_t,uint16_t>			 m_ycWaveNum;									//录波数目

	UpdateCmd					 m_bocastUpdateCmd;								//广播在线升级标志位
	static bool					 m_setPoll;										//轮训帧
	bool						 m_tSetTime;									//广播授时使能
	
	ModbusSoeCheckPCB			 m_yxSoeCheck;									//遥信soe查询
private:	
	
	void*       			 	*d_doubleBackup;
	TimerService				 t_doubleBackupModeCheck;						//查询主备在线超时时间
	MBusRequest					 m_requestMatch;								//匹配的请求帧
	bool                         m_firstPoll;                                   //处于主用状态下的第一轮轮询，可以报变位不能报SOE
    bool                         m_peerIsUpdating;                              //对端处于升级状态
    std::map<uint8_t, bool>             m_HighVolDevice;                               //高压回路对应设备
    bool                         m_compatibleYt;                                //遥调兼容标志
    std::deque<uint8_t>                 m_frameRecvBuff;                               //帧接收缓存
    uint16_t                      *d_recvErrCount;                                //接收出错计数
    uint16_t                      *d_resetCount;                                  //连续复位计数
    bool                         m_recviedFlag;                                 //接收到过数据标志
};

#endif


