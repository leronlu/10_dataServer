using namespace std;
#include "DownSideDataModule.h"
/************************************************************
  Copyright (C), Beijing Togest Automation System Equitment Co.,Ltd
  FileName:     ModBusProtocol.cpp
  Author:       李佳臻
  Version :     1.0
  Date:         2012-11-26
  Description:  ModBus协议处理
                该文件根据协议的修订而修订
  Version:      1.0
  Function List:   
    1. 
  History:         
      <author>  <time>       <version >   <desc>
      李佳臻    2012/11/26     1.0        修改  
***********************************************************/

#include "crc_make.h"
#include "ModBusProtocol.h"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#define Bottom_UPDATE_FILE  "/mnt/TG04B_Bottom-App.bin"
#define Roof_UPDATE_FILE       "/mnt/TG04B_Roof-App.bin"
//--------------------------------------------------
//全局静态变量
#define modbusPrint(level, format, arg...) DebugLog::Print(level, __LINE__, __FILE__, \
                  NULL, 0, "%s_%d: "#format,  "ModBusModule", m_config.serialPort-2, ##arg)
#define modbusPrintBuf(level, format, buf, len, arg...) DebugLog::Print(level, __LINE__, \
                  __FILE__, buf, len, "%s_%d: "#format, "ModBusModule", \
                  m_config.serialPort-2, ##arg)

bool ModBusProtocol::m_setPoll = false;

/*******************************************************************************
@ Function Name     : ModBusProtocol
@ Description       : 获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
ModBusProtocol::ModBusProtocol(int baseCir)
    : m_reqFifo(160), m_sendFrameFifo(10), m_recvFrameFifo(256), m_singleYxSoeFifo(64), 
      m_doubleYxSoeFifo(64),  m_dYxSoeFifo(64), m_validYcSoeFifo(64), m_FP32YcSoeFifo(64), 
      m_ymSoeFifo(0), d_doubleBackup(NULL), m_peerIsUpdating(false), m_compatibleYt(false)
{
    m_configFile = MODBUSMASTERCONFIG;
	m_baseCir = baseCir;
    m_firstPoll = false;
	fileLength = 0;
	parseMode = 0;
	m_waveDeleteId = 0;
	
    init();
}

/*******************************************************************************
@ Function Name     : ModBusProtocol
@ Description       : 获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
ModBusProtocol::ModBusProtocol(const string &fileName, int baseCir)
    : m_reqFifo(160), m_sendFrameFifo(10), m_recvFrameFifo(256), m_singleYxSoeFifo(64), 
      m_doubleYxSoeFifo(64),  m_dYxSoeFifo(64), m_validYcSoeFifo(64), m_FP32YcSoeFifo(64), 
      m_ymSoeFifo(0), d_doubleBackup(NULL), m_peerIsUpdating(false), m_compatibleYt(false)
{
    m_configFile = fileName;
	m_baseCir = baseCir;
    m_firstPoll = false;
	fileLength = 0;
	parseMode = 0;
	m_waveDeleteId = 0;
    init();
}
/*******************************************************************************
@ Function Name     : ~ModBusProtocol
@ Description       : 获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
ModBusProtocol::~ModBusProtocol()
{
}
/*******************************************************************************
@ Function Name     : init
@ Description       : 初始化
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::init()
{
	d_recvErrCount = NULL;
	d_resetCount   = NULL;
	m_recviedFlag  = false;
    t_rsp.SetTimer(1);                                                          //总线响应超时定时器
    t_rsp.EndTimer();
    t_RcvOver.SetMsTimer(200);

	t_yxSoe.SetTimer(2);														//下行设备上传的数据
	t_yxSoe.StartTimer();

	m_yxSoeCheck.yxSoeCheck.SetTimer(1);
	m_yxSoeCheck.yxSoeCheck.StartTimer();
	
    t_poll.SetMsTimer(100);
    t_Yk.SetTimer(3);
    t_Yk.EndTimer();

    t_write.SetTimer(1);
    t_delaySend.SetMsTimer(200);

	t_SetTime.SetTimer(60);
	m_tSetTime = true;

	//t_doubleBackupModeCheck.SetTimer(60);
	//t_doubleBackupModeCheck.StartTimer();
	
    m_reqRecord.pollReqPtr          = 0;
    m_reqRecord.rspStat             = rspFinished;

    m_config.setYcTHV               = true;
    m_isParamSetting                = false;
    m_setParamEventList.clear();
    m_setParamEventList.setMaxsize(1024);

	t_wave.SetTimer(3);
	t_wave.EndTimer();
    
    m_ykStatus                      = ykidle;
    m_ykCmdEvent.availability       = false;                                    //遥控突发事件
    m_ykCmdEvent.type               = ykCmdEvent;
    m_setParamEvent.availability    = false;                                    //遥测突发事件
    m_setParamEvent.type            = ycSetParamEvent;
    m_updateEvent.availability = false;                                          //在线升级事件
    m_updateEvent.type  = eventTypeDBUpdate;
    m_update.SetTimer(ModbusUpdateWaitTime);
	m_updateBuff.dataLen            = 0;
    m_updateBuff.d_data             = NULL;
    m_updateBuff.status             = ModbusUpdateStCodeNone;
	m_waveEvent.type				= ycWaveFileEvent;
	m_waveEvent.availability		=false;
	//m_brocastUpdate.SetMsTimer(100);
    m_readRecord.type               =  eventTypeDBRecord;               //招取事件记录事件
    m_readRecord.availability       = false;
    m_Record.SetTimer(m_RecordBuff.WaitTime);

    bzero(&m_config, sizeof(m_config));

    m_waveCause.push_back("一级过压产生");
    m_waveCause.push_back("一级过压恢复");
    m_waveCause.push_back("二级过压产生");
    m_waveCause.push_back("二级过压恢复");
    m_waveCause.push_back("欠压产生");
    m_waveCause.push_back("欠压恢复");
    m_waveCause.push_back("失压产生");
    m_waveCause.push_back("失压恢复");
    m_waveCause.push_back("一段过流产生");
    m_waveCause.push_back("一段过流恢复");
    m_waveCause.push_back("二段过流产生");
    m_waveCause.push_back("二段过流恢复");
    m_waveCause.push_back("三段过流产生");
    m_waveCause.push_back("三段过流恢复");
    m_waveCause.push_back("手动录波");
    m_waveCause.push_back("遥信触发录波");

	d_waveFileManage = NULL;
    d_webWaveFile    = NULL;
	waveFrameCounter = 1;
	waveWriteSuc	 = false;
	waveFrameNum	 = 0;
	waveTimeOutCounter = 0;
	dataZero         = false;
	m_bocastUpdateCmd = updateCmdNone;

	//setDataPoll(false);
	m_setPoll = true;

	m_yxSoeCheck.isSoeNumCheck = true;
	m_yxSoeCheck.isSoeCheck = false;
	m_yxSoeCheck.soeAckNum.singleYx = 0;
	m_yxSoeCheck.soeAckNum.doubleYx = 0;
	m_yxSoeCheck.soeNumTimeOutCounter = 0;
	m_yxSoeCheck.soeTimeOutCounter = 0;
	m_yxSoeCheck.SoeNumZeroCounter = 0;
}

/*******************************************************************************
@ Function Name     : loadConfig
@ Description       : 获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::loadConfig()
{
    modbusPrint(LOG_INFO, "加载ModBus配置文件...\n");
    initConfig();
    initData();
    initVector();
}

/*******************************************************************************
@ Function Name     : initVersion
@ Description       : 初始化版本号表
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::initVersion()
{
    ModbusVersion ver;
	ver.stat = NODE_OFFLINE;
	ver.oldStat = NODE_OFFLINE;
    
    m_version.resize(m_config.deviceNum, ver);
}

/*******************************************************************************
@ Function Name     : process
@ Description       : 协议处理函数公开接口
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::process()
{
    poll();
    dealMBusFrame();
    protocolOccured();
    preocessParamSet();	
}

/*******************************************************************************
@ Function Name     : initConfig
@ Description       : 读取配置文件获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::initConfig()
{
    XmlNodeParser f_XmlNodeParser((int8_t *)m_configFile.c_str(), (int8_t *)"/ModBusMasterConfig");
    int32_t value        = 0;
    int8_t  nodePath[120] = "";
    int8_t  text[20]     = "";
    uint16_t childCount   = 0;
    uint16_t doubleYxAddr = 0;
    uint16_t singleYxAddr = 0;
	uint16_t singleYxSoeNum = 0;
	uint16_t singleYxSoeBase = 0;
	uint16_t doubleYxSoeNum = 0;
	uint16_t doubleYxSoeBase = 0;
    uint16_t ykAddr       = 0;				         //0地址默认为一键启动
    //uint16_t writeAddr    = 0;
    uint16_t ytAddr       = 0;
     uint16_t ymAddr       = 0;
    uint16_t fpYcAddr     = 0;
    uint16_t ycAddr       = 0;
	uint16_t YcErrReportAddr =0;	
	uint16_t YcDir =0;
	uint16_t YcWave =0;
    uint16_t  requestNum   = 0;
	float	FPValue = 0;

	f_XmlNodeParser.FindNode((int8_t *)"/ModBusMasterConfig/Serial");
	if (f_XmlNodeParser.GetChildContent((int8_t *)"Port", value))
    {
    	modbusPrint(LOG_INFO, "串口号: %d", value);  
		m_config.serialPort = getSerialIndex(value);
    }
	
	f_XmlNodeParser.FindNode((int8_t *)"/ModBusMasterConfig");
    if (f_XmlNodeParser.GetChildContent((int8_t *)"ReceiveOverTime", value))
    {
        m_config.norspTime = value;
        t_rsp.SetTimer(value);
        //t_rsp.setObjName("响应超时定时器");
        modbusPrint(LOG_INFO, "响应超时时间: %d", m_config.norspTime);
    }

    if (f_XmlNodeParser.GetChildContent((int8_t *)"YcCheckDelay", value))
    {
        m_config.ycCheckPeriod = value;
    }
    if (f_XmlNodeParser.GetChildContent((int8_t *)"PollingTime", value))
    {
        m_config.pollPeriod = value;
        t_poll.SetMsTimer(value);
        t_poll.StartMsTimer();
        modbusPrint(LOG_INFO, "轮询周期时间(ms): %d", value);
    }
	if (f_XmlNodeParser.GetChildContent((int8_t *)"TimeSetPeriod", value))
    {
        m_config.timeSetPeriod = value;
        if (m_config.timeSetPeriod) {
            t_SetTime.SetTimer(value);
            t_SetTime.StartTimer();
        }
		else{
			m_tSetTime = false;
		}
        
        //t_rsp.setObjName("响应超时定时器");
        modbusPrint(LOG_INFO, "授时周期时间(s): %d", m_config.timeSetPeriod);
    }
	if (f_XmlNodeParser.GetChildContent((int8_t *)"AckSendTime", value))
    {
        m_config.ackSendTime = value;
        if (m_config.ackSendTime) {
            t_delaySend.SetMsTimer(value);
        }
        
        //t_rsp.setObjName("响应超时定时器");
        modbusPrint(LOG_INFO, "从机应答后延时发送时间(ms): %d", m_config.ackSendTime);
    }
    if (f_XmlNodeParser.GetChildContent((int8_t*)"YkOverTime", value))
    {
        m_config.ykTimeOut = value;
        t_Yk.SetTimer(value);
        modbusPrint(LOG_INFO, "遥控响应超时时间: %d", m_config.ykTimeOut);
    }

	if (f_XmlNodeParser.GetChildContent((int8_t*)"WaveOverTime", value))
    {
        m_config.waveTimeOut = value;
    }
	else{
		 m_config.waveTimeOut = 3;
	}
	modbusPrint(LOG_INFO, "录波文件传输响应超时时间: %d", m_config.waveTimeOut);
	t_wave.SetTimer(value);
	
	if (f_XmlNodeParser.GetChildContent((int8_t*)"BocastUpdateOverTime", value))
    {
        m_config.bocastUpdateOverTime = value;
        modbusPrint(LOG_INFO, "广播升级每一帧超时时间: \t%d", m_config.bocastUpdateOverTime);
    }
	else{
		m_config.bocastUpdateOverTime = 300;
	}
	
	m_brocastUpdate.SetMsTimer(m_config.bocastUpdateOverTime);

    if (f_XmlNodeParser.GetChildContent((int8_t *)"DelaySend", value))
    {
        m_config.reqDelaySendTime = value;
    }

    if (f_XmlNodeParser.GetChildContent((int8_t *)"SetYcTHV", text)) 
    {
        m_config.setYcTHV = f_XmlNodeParser.StrToBoolean(text);
    }
    if (f_XmlNodeParser.GetChildProperty((int8_t *)"CommunicateStatReport", (int8_t*)"enable", text)) {
        m_config.commStatReport = f_XmlNodeParser.StrToBoolean(text);
    }
    else {
        m_config.commStatReport = false;
    }
    if (f_XmlNodeParser.GetChildProperty((int8_t *)"CommunicateStatReport", (int8_t*)"trytimes", value)) {
        m_config.commStatTrytimes = value;
    }
    else {
        m_config.commStatTrytimes = 3;
    }
    
	if (f_XmlNodeParser.GetChildContent((int8_t *)"parseMode", value))
    {
        parseMode	= value;
    }
	if (f_XmlNodeParser.GetChildContent((int8_t *)"ReadFileLengthReduce", value))
    {
        m_config.readFileLengthReduceCounter = value;
    }
	else{
		m_config.readFileLengthReduceCounter = 0;
	}

	
    childCount = f_XmlNodeParser.GetChildCounter("Device");
    m_config.deviceNum = childCount;
	initVersion();
    /*
    *设备分为两种帧类型:ReadRequest(读请求帧),WriteRequest(写请求帧)
        ReadRequest: 
            Modbus协议部分: funCode:功能码, startAddr:起始地址, regNum:寄存器数目
                     data: THV：用以限制SOE的产生
            数据解析部分: 均将Modbus协议的应答帧的数据部分以字节流进行解析
                type: 帧类型, 标示每个解析中的regIndex取数的意义，即解析的时候先将Modbus数据按对应帧类型的数据类型
                      生成该类型的数据数组，而regIndex为该数据数组的下标+1
                 
                    singleYx: (数据类型：unsigned char 1个字节)，regIndex:数据单元数据流中序号, bitIndex:数据单元中位序号
                    doubleYx: (数据类型：unsigned char 1个字节)，onRegIndex:合数据单元在数据流中序号, onBitIndex:合位在数据单元中位序号
                              offRegIndex:分数据单元在数据流中序号, offBitIndex:分位在数据单元中位序号
                    yc: (数据类型：short 2个字节)，regIndex:数据单元在数据流中序号
                    ym: (数据类型：int 4个字节)，regIndex:数据单元在数据流中序号
                    param: (数据类型：short 2个字节)，regIndex:数据单元在数据流中序号
                    floatYc: (数据类型：float 4个字节)，regIndex:数据单元在数据流中序号
                    int32Yc: (数据类型：int 4个字节)，regIndex:数据单元在数据流中序号
        WriteRequest: 在下发了写请求帧时需要立即读取操作的结果 
                      readReqIdx:该读请求列表下的请求帧序号
                      
            yk: 只支持双点遥控 onAddr:合闸寄存器地址 onAction:合闸动作 offAddr:分闸寄存器地址 offAction:分闸动作
            write: addr: 写寄存器地址, readDataIdx:该地址对应在读请求帧数据列表中的数据序号
    */
    modbusPrint(LOG_INFO, "总设备数目: %d", childCount);
	uint16_t  cirIndex		 = 0;									//故障报告回路索引号
	
    for (uint16_t i=0; i<childCount; i++)
    {
        uint16_t deviceAddr    = 0;
        uint16_t frameNum     = 0;
        
        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]", i+1);
        f_XmlNodeParser.FindNode(nodePath);

        if (f_XmlNodeParser.GetChildContent((int8_t *)"DeviceId", value))
        {
            deviceAddr = value;
			m_version[i].deviceId = value;	    
        }
		
		m_ycWaveNum.insert(pair<uint8_t, uint16_t>(deviceAddr,0));
        
        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest", i+1);
        if (f_XmlNodeParser.FindNode(nodePath)) {
            
        frameNum   = f_XmlNodeParser.GetChildCounter("frame");
        requestNum = frameNum;
        for (uint16_t id=0; id<frameNum; id++)
        {
            MBusRequest req;
			MBusYt      ytReg;
            uint16_t		dataNum = 0;
			uint16_t		subFrame =0;
            MBusFrameType frameType = frameNoType;
			MBusFrameType subFrameType = frameNoType;
			MBusParseType parseFrameType = frameParseDataNoType;
			memset(text, 0, sizeof(text)); 			
			
            req.deviceIndex = i;
            sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]",
                i+1, id+1);
            f_XmlNodeParser.FindNode(nodePath);
            f_XmlNodeParser.GetProperty((int8_t*)"type", text);
            frameType = getFrameType((char *)text);

            req.type       = frameType;
            req.deviceAddr = deviceAddr; 
            f_XmlNodeParser.GetProperty((int8_t*)"funCode", value);
            req.funcode    = value;
			
			if(frameType != frameMix){
	            f_XmlNodeParser.GetProperty((int8_t*)"startAddr", value);
	            req.startAddr  = value;
	            f_XmlNodeParser.GetProperty((int8_t*)"regNum", value);
	            req.regNum     = value;
            	dataNum = f_XmlNodeParser.GetChildCounter("data");
			}
			else{
				subFrame = f_XmlNodeParser.GetChildCounter("subFrame");
			}
            switch (frameType)
            {
            	case frameMix:
				{
					for(uint8_t subFrameId=0; subFrameId<subFrame;subFrameId++){
						MBusRequest subReq;
 						//doubleYx parse
						sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/subFrame[%d]",
                            i+1, id+1, subFrameId+1);

						subReq.deviceAddr = deviceAddr;
						f_XmlNodeParser.FindNode(nodePath);
            			f_XmlNodeParser.GetProperty((int8_t*)"type", text);
            			subFrameType = getFrameType((char *)text);
						subReq.type = subFrameType;
						f_XmlNodeParser.GetProperty((int8_t*)"funCode", value);
            			subReq.funcode    = value;
						f_XmlNodeParser.GetProperty((int8_t*)"startAddr", value);
			            subReq.startAddr  = value;
			            f_XmlNodeParser.GetProperty((int8_t*)"regNum", value);
			            subReq.regNum     = value;
						
		            	dataNum = f_XmlNodeParser.GetChildCounter("data");
		
						switch(subFrameType)
						{
							case frameDoubleYx:{
								DataINT8U yxdata;                        
                    
			                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
			                    {
			                        MBusParseDoubleYx dbyxParse;
			                        
			                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
			                            "ReadRequest/frame[%d]/subFrame[%d]/data[%d]",
			                            i+1, id+1, subFrameId+1,dataId+1);
			                        f_XmlNodeParser.FindNode(nodePath);
			                        f_XmlNodeParser.GetProperty((int8_t *)"onBitIndex", value);
			                        dbyxParse.onBitIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"onRegIndex", value);
			                        dbyxParse.onRegIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"offBitIndex", value);
			                        dbyxParse.offBitIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"offRegIndex", value);
			                        dbyxParse.offRegIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
			                        dbyxParse.THV         = value;

			                        m_dataParse.doubleYx.push_back(dbyxParse);
			                        m_data.doubleYx.push_back(yxdata);
			                    }

			                    subReq.dataOffset = doubleYxAddr;
								doubleYxSoeBase = doubleYxAddr;
			                    subReq.dataNum    = dataNum;
								doubleYxSoeNum = dataNum;
			                    doubleYxAddr  += dataNum;

			                    break;
							}
							case frameSingleYx: 
			                {
			                    DataINT8U yxdata;

			                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
			                    {
			                        MBusParseSingleYx sgyxParse;
			                        
			                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
			                            "ReadRequest/frame[%d]/subFrame[%d]/data[%d]",
			                            i+1, id+1, subFrameId+1, dataId+1);
			                        f_XmlNodeParser.FindNode(nodePath);
			                        f_XmlNodeParser.GetProperty((int8_t *)"bitIndex", value);
			                        sgyxParse.bitIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
			                        sgyxParse.regIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
			                        sgyxParse.THV         = value;

			                        m_dataParse.singleYx.push_back(sgyxParse);
			                        m_data.singleYx.push_back(yxdata);
			                    }

			                    subReq.dataOffset = singleYxAddr;
								singleYxSoeBase = singleYxAddr;
			                    subReq.dataNum    = dataNum;
								singleYxSoeNum = dataNum;
			                    singleYxAddr  += dataNum;
			                    break;
			                }
							case frameYc:
			                {
			                    DataINT16S ycdata;

			                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
			                    {
			                        MBusParseData ycparse;
			                        
			                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
			                            "ReadRequest/frame[%d]/subFrame[%d]/data[%d]",
			                            i+1, id+1, subFrameId+1, dataId+1);
			                        f_XmlNodeParser.FindNode(nodePath);
			                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
			                        ycparse.regIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
			                        ycparse.THV      = value;
                                    
                                    ycparse.compensate = 0;
                                    if (f_XmlNodeParser.GetPropertyDefault((int8_t *)"compensate", value, 0)) {
                                        ycparse.compensate = value;
                                    }
			                        m_dataParse.yc.push_back(ycparse);
			                        m_data.Yc.push_back(ycdata);
			                    }

			                    subReq.dataOffset = ycAddr;
			                    subReq.dataNum    = dataNum;
			                    ycAddr        += dataNum;
			                    
			                    break;
			                }
							case frameYm:
			                {
			                    DataINT32S ymdata;

			                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
			                    {
			                        MBusParseData ymparse;
			                        
			                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
			                            "ReadRequest/frame[%d]/subFrame[%d]/data[%d]",
			                            i+1, id+1,  subFrameId+1, dataId+1);
			                        f_XmlNodeParser.FindNode(nodePath);
			                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
			                        ymparse.regIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
			                        ymparse.THV      = value;

			                        m_dataParse.ym.push_back(ymparse);
			                        m_data.Ym.push_back(ymdata);
			                    }

			                    subReq.dataOffset = ymAddr;
			                    subReq.dataNum    = dataNum;
			                    ymAddr        += dataNum;
			                    
			                    break;
			                }
							
							case frameYt:
			                {
			                    DataINT16S ytdata;

			                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
			                    {
			                        MBusParseData ytparse;
			                        
			                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
			                            "ReadRequest/frame[%d]/subFrame[%d]/data[%d]",
			                            i+1, id+1, subFrameId+1, dataId+1);
			                        f_XmlNodeParser.FindNode(nodePath);
			                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
			                        ytparse.regIndex = value - 1;
			                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
			                        ytparse.THV      = 0;
									f_XmlNodeParser.GetProperty((int8_t*)"type", text);
									parseFrameType = getParseFrameType((char *)text);
			                        ytparse.dataType      = parseFrameType;

			                        m_dataParse.yt.push_back(ytparse);
			                        m_data.yt.push_back(ytdata);

									//memcpy(&ytReg.request ,&subReq ,sizeof(MBusRequest));
									//ytReg.request.subMBusRequest = subReq.subMBusRequest;
									ytReg.request = subReq;
									ytReg.ytUintAddrOffset = ytparse.regIndex;
                                    ytReg.readIndex        = m_readRequest.size();
                                    
									m_ytReg.push_back(ytReg);
			                    }

			                    subReq.dataOffset = ytAddr;
			                    subReq.dataNum    = dataNum;
			                    ytAddr        += dataNum;

								//ytReg.request = &subReq;
								
								
			                    break;
			                }
							case frameReadErrReport:
			                {
					           	f_XmlNodeParser.GetProperty((int8_t*)"type", value);
					            subReq.refType    = value;
					            f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
					            subReq.refType    = value;
					            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
					            subReq.fileNum	   = value;
								f_XmlNodeParser.GetProperty((int8_t*)"recordLegth", value);
								subReq.recordLenth= value;
								f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
								subReq.recordNum= value;
								sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]/subFrame[%d]",
								i+1, id+1, subFrameId+1);

								subReq.dataOffset = YcErrReportAddr;
					            subReq.dataNum    = dataNum;
								YcErrReportAddr += dataNum;
			                    break;
			                }
							case frameReadDir:
			                {
					           
					            f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
					            subReq.refType    = value;
					            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
					            subReq.fileNum	   = value;
								f_XmlNodeParser.GetProperty((int8_t*)"recordLegth", value);
								subReq.recordLenth= value;
								f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
								subReq.recordNum= value;
								sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]/subFrame[%d]",
								i+1, id+1, subFrameId+1);

								subReq.dataOffset = YcDir;
					            subReq.dataNum    = dataNum;
								YcDir += dataNum;
			                    break;
			                }
							case frameYxSoe:
			                {
					           	f_XmlNodeParser.GetProperty((int8_t*)"type", value);
					            subReq.refType    = value;
					            f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
					            subReq.refType    = value;
					            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
					            subReq.fileNum	   = value;
								f_XmlNodeParser.GetProperty((int8_t*)"recordLegth", value);
								subReq.recordLenth= value;
								f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
								subReq.recordNum= value;
								sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]/subFrame[%d]",
								i+1, id+1, subFrameId+1);
								
								subReq.soeNum.singleYxNum = singleYxSoeNum;
								subReq.soeBaseAddr.singleYxBaseAddr = singleYxSoeBase;
								subReq.soeNum.doubleYxNum = doubleYxSoeNum;
								subReq.soeBaseAddr.doubleYxBaseAddr = doubleYxSoeBase;
								
			                    break;
			                }
							case frameYxSoeNum:
			                {
					           	f_XmlNodeParser.GetProperty((int8_t*)"type", value);
					            subReq.refType    = value;
					            f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
					            subReq.refType    = value;
					            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
					            subReq.fileNum	   = value;
								f_XmlNodeParser.GetProperty((int8_t*)"recordLegth", value);
								subReq.recordLenth= value;
								f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
								subReq.recordNum= value;
								sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]/subFrame[%d]",
								i+1, id+1, subFrameId+1);
								
								subReq.soeNum.singleYxNum = singleYxSoeNum;
								subReq.soeBaseAddr.singleYxBaseAddr = singleYxSoeBase;
								subReq.soeNum.doubleYxNum = doubleYxSoeNum;
								subReq.soeBaseAddr.doubleYxBaseAddr = doubleYxSoeBase;
								
			                    break;
			                }
							default:break;
						}
						req.subMBusRequest.push_back(subReq);
					}
					break;
				}
                case frameDoubleYx: 
                {
                    DataINT8U yxdata;                        
                    
                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
                    {
                        MBusParseDoubleYx dbyxParse;
                        
                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/data[%d]",
                            i+1, id+1, dataId+1);
                        f_XmlNodeParser.FindNode(nodePath);
                        f_XmlNodeParser.GetProperty((int8_t *)"onBitIndex", value);
                        dbyxParse.onBitIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"onRegIndex", value);
                        dbyxParse.onRegIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"offBitIndex", value);
                        dbyxParse.offBitIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"offRegIndex", value);
                        dbyxParse.offRegIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
                        dbyxParse.THV         = value;

                        m_dataParse.doubleYx.push_back(dbyxParse);
                        m_data.doubleYx.push_back(yxdata);
                    }
                    
                    req.dataOffset = doubleYxAddr;
					doubleYxSoeBase = doubleYxAddr;
                    req.dataNum    = dataNum;
					doubleYxSoeNum = dataNum;
                    doubleYxAddr  += dataNum;

                    break;
                }
                case frameSingleYx: 
                {
                    DataINT8U yxdata;

                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
                    {
                        MBusParseSingleYx sgyxParse;
                        
                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/data[%d]",
                            i+1, id+1, dataId+1);
                        f_XmlNodeParser.FindNode(nodePath);
                        f_XmlNodeParser.GetProperty((int8_t *)"bitIndex", value);
                        sgyxParse.bitIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
                        sgyxParse.regIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
                        sgyxParse.THV         = value;

                        m_dataParse.singleYx.push_back(sgyxParse);
                        m_data.singleYx.push_back(yxdata);
                    }

                    req.dataOffset = singleYxAddr;
					singleYxSoeBase = singleYxAddr;
                    req.dataNum    = dataNum;
					singleYxSoeNum = dataNum;
                    singleYxAddr  += dataNum;
                    break;
                }
                case frameYc:
                {
                    DataINT16S ycdata;

                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
                    {
                        MBusParseData ycparse;
                        
                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/data[%d]",
                            i+1, id+1, dataId+1);
                        f_XmlNodeParser.FindNode(nodePath);
                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
                        ycparse.regIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
                        ycparse.THV      = value;

                        ycparse.compensate = 0;
                        if (f_XmlNodeParser.GetPropertyDefault((int8_t *)"compensate", value, 0)) {
                            ycparse.compensate = value;
                        }

                        m_dataParse.yc.push_back(ycparse);
                        m_data.Yc.push_back(ycdata);
                    }

                    req.dataOffset = ycAddr;
                    req.dataNum    = dataNum;
                    ycAddr        += dataNum;
                    
                    break;
                }
                case frameYm:
                {
                    DataINT32S ymdata;

                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
                    {
                        MBusParseData ymparse;
                        
                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/data[%d]",
                            i+1, id+1, dataId+1);
                        f_XmlNodeParser.FindNode(nodePath);
                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
                        ymparse.regIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
                        ymparse.THV      = value;

                        m_dataParse.ym.push_back(ymparse);
                        m_data.Ym.push_back(ymdata);
                    }

                    req.dataOffset = ymAddr;
                    req.dataNum    = dataNum;
                    ymAddr        += dataNum;
                    
                    break;
                }
                case frameInt32Yc:
                case frameFloatYc:
                {
                    DataFP32 ycdata;

                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
                    {
                        MBusParseData ycparse;
                        
                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/data[%d]",
                            i+1, id+1, dataId+1);
                        f_XmlNodeParser.FindNode(nodePath);
                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
                        ycparse.regIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
                        ycparse.THV      = value;

                        ycparse.compensate = 0;
                        if (f_XmlNodeParser.GetProperty((int8_t *)"compensate", value)) {
                            ycparse.compensate = value;
                        }

						ycparse.factor = 1;
                        if (f_XmlNodeParser.GetProperty((int8_t *)"factor", FPValue)) {
                            ycparse.factor = FPValue;
                        }

                        m_dataParse.floatYc.push_back(ycparse);
                        m_data.floatYc.push_back(ycdata);
                    }

                    req.dataOffset = fpYcAddr;
                    req.dataNum    = dataNum;
                    fpYcAddr      += dataNum;
                    
                    break;
                }

				case frameInt16Yc:
                {
                    DataFP32 ycdata;

                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
                    {
                        MBusParseData ycparse;
                        
                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/data[%d]",
                            i+1, id+1, dataId+1);
                        f_XmlNodeParser.FindNode(nodePath);
                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
                        ycparse.regIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
                        ycparse.THV      = value;

                        ycparse.compensate = 0;
                        if (f_XmlNodeParser.GetProperty((int8_t *)"compensate", value)) {
                            ycparse.compensate = value;
                        }

						ycparse.factor = 1;
                        if (f_XmlNodeParser.GetProperty((int8_t *)"factor", FPValue)) {
                            ycparse.factor = FPValue;
                        }

                        m_dataParse.floatYc.push_back(ycparse);
                        m_data.floatYc.push_back(ycdata);
                    }

                    req.dataOffset = fpYcAddr;
                    req.dataNum    = dataNum;
                    fpYcAddr      += dataNum;
                    
                    break;
                }

				case frameInt8Yc:
                {
                    DataFP32 ycdata;

                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
                    {
                        MBusParseData ycparse;
                        
                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/data[%d]",
                            i+1, id+1, dataId+1);
                        f_XmlNodeParser.FindNode(nodePath);
                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
                        ycparse.regIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
                        ycparse.THV      = value;

                        ycparse.compensate = 0;
                        if (f_XmlNodeParser.GetProperty((int8_t *)"compensate", value)) {
                            ycparse.compensate = value;
                        }

						ycparse.factor = 1;
                        if (f_XmlNodeParser.GetProperty((int8_t *)"factor", FPValue)) {
                            ycparse.factor = FPValue;
                        }

                        m_dataParse.floatYc.push_back(ycparse);
                        m_data.floatYc.push_back(ycdata);
                    }

                    req.dataOffset = fpYcAddr;
                    req.dataNum    = dataNum;
                    fpYcAddr      += dataNum;
                    
                    break;
                }
				
                case frameYt:
                {
                    DataINT16S ytdata;

                    for (uint8_t dataId=0; dataId<dataNum; dataId++)
                    {
                        MBusParseData ytparse;
                        
                        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/"
                            "ReadRequest/frame[%d]/data[%d]",
                            i+1, id+1, dataId+1);
                        f_XmlNodeParser.FindNode(nodePath);
                        f_XmlNodeParser.GetProperty((int8_t *)"regIndex", value);
                        ytparse.regIndex = value - 1;
                        f_XmlNodeParser.GetProperty((int8_t *)"THV", value);
                        ytparse.THV      = 0;
						f_XmlNodeParser.GetPropertyDefault((int8_t*)"type", text, (int8_t*)"null");
						parseFrameType = getParseFrameType((char *)text);
                        ytparse.dataType      = parseFrameType;

                        m_dataParse.yt.push_back(ytparse);
                        m_data.yt.push_back(ytdata);
						ytReg.request = req;
						
						ytReg.ytUintAddrOffset = ytparse.regIndex;
                        ytReg.readIndex        = m_readRequest.size();


						m_ytReg.push_back(ytReg);

                    }

                    req.dataOffset = ytAddr;
                    req.dataNum    = dataNum;
                    ytAddr        += dataNum;
					//ytReg.request = &req;

                    
                    break;
                }

				case frameReadErrReport:
                {
		           
		            f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
		            req.refType    = value;
		            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
		            req.fileNum	   = value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordLegth", value);
					req.recordLenth= value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
					req.recordNum= value;
					sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]",
					i+1, id+1);
					req.dataOffset = YcErrReportAddr;
		            req.dataNum    = dataNum;
					YcErrReportAddr += dataNum;
                    break;
                }
				case frameReadDir:
                {
		           
		            f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
		            req.refType    = value;
		            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
		            req.fileNum	   = value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordLegth", value);
					req.recordLenth= value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
					req.recordNum= value;
					sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]",
					i+1, id+1);
					req.dataOffset = YcDir;
		            req.dataNum    = dataNum;
					YcDir += dataNum;
                    break;
                }
				case frameReadWave:
                {
		           
		            f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
		            req.refType    = value;
		            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
		            req.fileNum	   = value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordLegth", value);
					req.recordLenth= value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
					req.recordNum= value;
					sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]",
					i+1, id+1);
					req.dataOffset = YcWave;
		            req.dataNum    = dataNum;
					YcWave += dataNum;
                    break;
                }
				case frameYxSoeNum:
				{
					f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
		            req.refType    = value;
		            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
		            req.fileNum	   = value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordLength", value);
					req.recordLenth= value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
					req.recordNum= value;
					sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]",
					i+1, id+1);

					req.soeNum.singleYxNum = singleYxSoeNum;
					req.soeBaseAddr.singleYxBaseAddr = singleYxSoeBase;
					req.soeNum.doubleYxNum = doubleYxSoeNum;
					req.soeBaseAddr.doubleYxBaseAddr = doubleYxSoeBase;
					
                    break;
				}
				case frameYxSoe:
				{
					f_XmlNodeParser.GetProperty((int8_t*)"refType", value);
		            req.refType    = value;
		            f_XmlNodeParser.GetProperty((int8_t*)"fileNum", value);
		            req.fileNum	   = value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordLength", value);
					req.recordLenth= value;
					f_XmlNodeParser.GetProperty((int8_t*)"recordNum", value);
					req.recordNum= value;
					sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadRequest/frame[%d]",
					i+1, id+1);
					req.soeNum.singleYxNum = singleYxSoeNum;
					req.soeBaseAddr.singleYxBaseAddr = singleYxSoeBase;
					req.soeNum.doubleYxNum = doubleYxSoeNum;
					req.soeBaseAddr.doubleYxBaseAddr = doubleYxSoeBase;
					
                    break;
				}
                default:break;
            }

            m_readRequest.push_back(req);
        }
        }

        /*解析写请求配置*/
        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/WriteRequest", i+1);
        if (f_XmlNodeParser.FindNode(nodePath)) {
            
        uint16_t ykSelectBaseAddr = ykAddr;
        frameNum = f_XmlNodeParser.GetChildCounter("frame");
        for (uint16_t id=0; id<frameNum; id++)
        {            
            sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]",
                            i+1, id+1);
            f_XmlNodeParser.FindNode(nodePath);
            f_XmlNodeParser.GetProperty((int8_t *)"type", text);

            if (!strcasecmp("yk", (char *)text))                                //遥控
            {
                MBusWriteYk  writeYk;

                writeYk.deviceAddr  = deviceAddr;
                f_XmlNodeParser.GetProperty((int8_t *)"funCode", value);
                writeYk.funCode     = value;
                f_XmlNodeParser.GetProperty((int8_t *)"readFrameIdx", value);
                if (value)
                {
                    value -= 1;
                    writeYk.readIndex = m_readRequest.size() - requestNum + value;
                }
                else {
                    writeYk.readIndex = 0xFFFF;
                }
                f_XmlNodeParser.GetPropertyDefault((int8_t *)"delayReadTime", value, 1000);
                writeYk.delayReadTime = value;
                    
                writeYk.num = f_XmlNodeParser.GetChildCounter("reg");
                
                writeYk.baseAddr    = ykAddr;
                ykAddr             += writeYk.num;
                m_config.ykNum     += writeYk.num;
                for (uint8_t regid=0; regid<writeYk.num; regid++)
                {
                    MBusDoubleYk ykConf;
                    
                    sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/reg[%d]",
                            i+1, id+1, regid+1);
                    f_XmlNodeParser.FindNode(nodePath);
                    f_XmlNodeParser.GetProperty((int8_t *)"onAddr", value);
                    ykConf.ykOn.addr    = value;
                    f_XmlNodeParser.GetProperty((int8_t *)"onAction", value);
                    ykConf.ykOn.action = value;

                    f_XmlNodeParser.GetProperty((int8_t *)"offAddr", value);
                    ykConf.ykOff.addr    = value;
                    f_XmlNodeParser.GetProperty((int8_t *)"offAction", value);
                    ykConf.ykOff.action = value;

                    writeYk.doubleYk.push_back(ykConf);
                }

                m_writeYK.push_back(writeYk);
            }
            else
            if (!strcasecmp("settime", (char *)text) ||
                !strcasecmp("write", (char *)text))                             //授时寄存器
            {
                MBusWriteReg writeReg;

                writeReg.deviceAddr  = deviceAddr;
                f_XmlNodeParser.GetProperty((int8_t *)"funCode", value);
                writeReg.funCode     = value;
                f_XmlNodeParser.GetProperty((int8_t *)"readFrameIdx", value);
                if (value)
                {
                    value -= 1;
                    writeReg.readIndex = m_readRequest.size() - requestNum + value;
                }
                else {
                    writeReg.readIndex = 0xFFFF;
                }
                writeReg.baseAddr    = 0;

                writeReg.num         = f_XmlNodeParser.GetChildCounter("reg");

                for (uint8_t regid=0; regid<writeReg.num; regid++)
                {                    
                    sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/reg[%d]",
                            i+1, id+1, regid+1);
                    f_XmlNodeParser.FindNode(nodePath);
                    f_XmlNodeParser.GetProperty((int8_t *)"addr", value);
                    writeReg.regAddr.push_back(value);
                }
                m_writeReg.push_back(writeReg);
                if (!strcasecmp("write", (char *)text)) {
                    m_compatibleYt = true;
                    m_config.writeRegs = writeReg.num;
                }
            }
            else
            if (!strcasecmp("multiYk", (char *)text))                             //写寄存器
            {
                MBusMultiWriteYk myk;

                myk.deviceAddr  = deviceAddr;
                f_XmlNodeParser.GetProperty((int8_t *)"funCode", value);
                myk.funCode     = value;
                f_XmlNodeParser.GetProperty((int8_t *)"readFrameIdx", value);
                if (value)
                {
                    value -= 1;
                    myk.readIndex = m_readRequest.size() - requestNum + value;
                }
                else {
                    myk.readIndex = 0xFFFF;
                }
                myk.baseAddr    = ykAddr;

                myk.num         = 1;

                ykAddr              += myk.num;
                m_config.ykNum      += myk.num;
                sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/on",
                            i+1, id+1);
                f_XmlNodeParser.FindNode(nodePath);
                f_XmlNodeParser.GetProperty((int8_t *)"startAddr", value);
                myk.ykOn.startAddr = value;
                f_XmlNodeParser.GetProperty((int8_t *)"num", value);
                myk.ykOn.num = value;
                int regNum = f_XmlNodeParser.GetChildCounter("reg");
                for (uint8_t regid=0; regid<regNum; regid++)
                {                    
                    sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/on/reg[%d]",
                            i+1, id+1, regid+1);
                    f_XmlNodeParser.FindNode(nodePath);
                    f_XmlNodeParser.GetContent(value);
                    myk.ykOn.action.push_back(value);
                }
                sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/off",
                            i+1, id+1);
                f_XmlNodeParser.FindNode(nodePath);
                f_XmlNodeParser.GetProperty((int8_t *)"startAddr", value);
                myk.ykOff.startAddr = value;
                f_XmlNodeParser.GetProperty((int8_t *)"num", value);
                myk.ykOff.num = value;
                int regOffNum = f_XmlNodeParser.GetChildCounter("reg");
                for (uint8_t regid=0; regid<regOffNum; regid++)
                {                    
                    sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/off/reg[%d]",
                            i+1, id+1, regid+1);
                    f_XmlNodeParser.FindNode(nodePath);
                    f_XmlNodeParser.GetContent(value);
                    myk.ykOff.action.push_back(value);
                }
                m_writeMultiYK.push_back(myk);
            }
            else
            if (!strcasecmp("ykSelect", (char *)text))                             //写寄存器
            {
                MBusMultiWriteYk syk;

                syk.deviceAddr  = deviceAddr;
                f_XmlNodeParser.GetProperty((int8_t *)"funCode", value);
                syk.funCode     = value;
                f_XmlNodeParser.GetProperty((int8_t *)"ykFrameIdx", value);
                if (value)
                {
                    value -= 1;
                    syk.readIndex = ykSelectBaseAddr + value;
                }
                else {
                    syk.readIndex = 0xFFFF;
                }
                if (syk.readIndex == 0xFFFF)
                    continue;
                
                sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/on",
                            i+1, id+1);
                f_XmlNodeParser.FindNode(nodePath);
                f_XmlNodeParser.GetProperty((int8_t *)"startAddr", value);
                syk.ykOn.startAddr = value;
                f_XmlNodeParser.GetProperty((int8_t *)"num", value);
                syk.ykOn.num = value;
                int regNum = f_XmlNodeParser.GetChildCounter("reg");
                for (uint8_t regid=0; regid<regNum; regid++)
                {                    
                    sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/on/reg[%d]",
                            i+1, id+1, regid+1);
                    f_XmlNodeParser.FindNode(nodePath);
                    f_XmlNodeParser.GetContent(value);
                    syk.ykOn.action.push_back(value);
                }
                sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/off",
                            i+1, id+1);
                f_XmlNodeParser.FindNode(nodePath);
                f_XmlNodeParser.GetProperty((int8_t *)"startAddr", value);
                syk.ykOff.startAddr = value;
                f_XmlNodeParser.GetProperty((int8_t *)"num", value);
                syk.ykOff.num = value;
                int regOffNum = f_XmlNodeParser.GetChildCounter("reg");
                for (uint8_t regid=0; regid<regOffNum; regid++)
                {                    
                    sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/off/reg[%d]",
                            i+1, id+1, regid+1);
                    f_XmlNodeParser.FindNode(nodePath);
                    f_XmlNodeParser.GetContent(value);
                    syk.ykOff.action.push_back(value);
                }
                sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/abort",
                            i+1, id+1);
                f_XmlNodeParser.FindNode(nodePath);
                f_XmlNodeParser.GetProperty((int8_t *)"startAddr", value);
                syk.ykAbort.startAddr = value;
                f_XmlNodeParser.GetProperty((int8_t *)"num", value);
                syk.ykAbort.num = value;
                int regAbortNum = f_XmlNodeParser.GetChildCounter("reg");
                for (uint8_t regid=0; regid<regAbortNum; regid++)
                {                    
                    sprintf((char *)nodePath, 
                            "/ModBusMasterConfig/Device[%d]/WriteRequest/frame[%d]/abort/reg[%d]",
                            i+1, id+1, regid+1);
                    f_XmlNodeParser.FindNode(nodePath);
                    f_XmlNodeParser.GetContent(value);
                    syk.ykAbort.action.push_back(value);
                }

                m_selectYK[syk.readIndex] = syk;
            }
        }
        }
        else {
            m_config.commStatReport = false;
        }
		#if 1
		 /*解析读文件请求*/
        sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadFileRequest", i+1);
        if (f_XmlNodeParser.FindNode(nodePath)) {
        frameNum = f_XmlNodeParser.GetChildCounter("frame");
        for (uint16_t id=0; id<frameNum; id++)
        {      
        	MBusReadFileRequest  readFileRequest;
            sprintf((char *)nodePath, "/ModBusMasterConfig/Device[%d]/ReadFileRequest/frame[%d]",
                            i+1, id+1);
            f_XmlNodeParser.FindNode(nodePath);
            f_XmlNodeParser.GetProperty((int8_t *)"type", text);
			readFileRequest.type = getFrameType((char *)text);
			
            if (!strcasecmp("readSoeNum", (char *)text))                                //读soe数目
            {
                readFileRequest.deviceAddr  = deviceAddr;
                f_XmlNodeParser.GetProperty((int8_t *)"funCode", value);
                readFileRequest.funCode     = value;
               	f_XmlNodeParser.GetProperty((int8_t *)"refType", value);
                readFileRequest.quoteType     = value;
				f_XmlNodeParser.GetProperty((int8_t *)"fileNum", value);
                readFileRequest.fileNum     = value;
				f_XmlNodeParser.GetProperty((int8_t *)"recordNum", value);
                readFileRequest.recordNum     = value;
				f_XmlNodeParser.GetProperty((int8_t *)"recordLength", value);
                readFileRequest.recordLength     = value;
				readFileRequest.num			= singleYxSoeNum;
				readFileRequest.baseAddr	= singleYxSoeBase;

				readFileRequest.soeNum.singleYxNum = singleYxSoeNum;
				readFileRequest.soeBaseAddr.singleYxBaseAddr = singleYxSoeBase;
				readFileRequest.soeNum.doubleYxNum = doubleYxSoeNum;
				readFileRequest.soeBaseAddr.doubleYxBaseAddr = doubleYxSoeBase;
                m_readFileRequest.push_back(readFileRequest);
            }
            else
            if (!strcasecmp("readSoe", (char *)text))                             		//读soe信息
            {	
				readFileRequest.deviceAddr  = deviceAddr;
                f_XmlNodeParser.GetProperty((int8_t *)"funCode", value);
                readFileRequest.funCode     = value;
               	f_XmlNodeParser.GetProperty((int8_t *)"refType", value);
                readFileRequest.quoteType     = value;
				f_XmlNodeParser.GetProperty((int8_t *)"fileNum", value);
                readFileRequest.fileNum     = value;
				f_XmlNodeParser.GetProperty((int8_t *)"recordNum", value);
                readFileRequest.recordNum     = value;
				f_XmlNodeParser.GetProperty((int8_t *)"recordLength", value);
                readFileRequest.recordLength     = value;
				readFileRequest.soeNum.singleYxNum = singleYxSoeNum;
				readFileRequest.soeBaseAddr.singleYxBaseAddr = singleYxSoeBase;
				readFileRequest.soeNum.doubleYxNum = doubleYxSoeNum;
				readFileRequest.soeBaseAddr.doubleYxBaseAddr = doubleYxSoeBase;
				
                m_readFileRequest.push_back(readFileRequest);
            }
        }
        }

		#endif
        /*每个设备后追加通信状态软遥信*/
        if (m_config.commStatReport) {
            DataINT8U yxdata;

            {           //处理因增加通讯状态遥信数目后遥信的数目与解析数目不匹配 导致每增加一个modbus设备遥信都会错位的问题
                MBusParseSingleYx sgyxParse;
                memset(&sgyxParse, 1, sizeof(sgyxParse));

                m_dataParse.singleYx.push_back(sgyxParse);
            }
            m_data.singleYx.push_back(yxdata);
            m_deviceIdToCommstatAddr.insert(std::pair<int,int>(i, m_data.singleYx.size() - 1));
            singleYxAddr += 1;
        }
        m_deviceRetryCount.push_back(0);
		//回路索引号与设备ID相关联
		if (deviceAddr != 255){
			MBusParseErrReport errReportParse;
			errReportParse.cirIndex = cirIndex;
			errReportParse.deviceId = deviceAddr;
			
			m_dataParse.errReport.push_back(errReportParse);
			cirIndex++;
		}
    }
    if (f_XmlNodeParser.FindNode((int8_t *)"/ModBusMasterConfig/HighVolDevice")) {
        uint8_t deviceCnt = f_XmlNodeParser.GetChildCounter("DeviceId");
        for (uint8_t i=1; i<=deviceCnt; i++) {
            sprintf((char *)nodePath, "/ModBusMasterConfig/HighVolDevice/DeviceId[%d]", i);
            f_XmlNodeParser.FindNode(nodePath);
            f_XmlNodeParser.GetContent(value);
            m_HighVolDevice[value] = true;
            modbusPrint(LOG_INFO, "设备:%d 配置为高压回路", value);
        }
    }
    
    m_baseDataConfig.validYcCircuitNum = cirIndex;
    modbusPrint(LOG_INFO, "双点遥信数目:   %d", m_data.doubleYx.size());
    modbusPrint(LOG_INFO, "单点遥信数目:   %d", m_data.singleYx.size());
    modbusPrint(LOG_INFO, "遥测数目:       %d", m_data.Yc.size());
    modbusPrint(LOG_INFO, "短浮点遥测数目: %d", m_data.floatYc.size());
    modbusPrint(LOG_INFO, "遥脉数目:       %d", m_data.Ym.size());
    modbusPrint(LOG_INFO, "遥控数目:       %d", m_config.ykNum);
    modbusPrint(LOG_INFO, "参数量数目:     %d", m_data.yt.size());
    modbusPrint(LOG_INFO, "写寄存器数目:   %d", m_config.writeRegs);
    modbusPrint(LOG_INFO, "回路数目:       %d", m_baseDataConfig.validYcCircuitNum);
}

/*******************************************************************************
@ Function Name     : getConfig
@ Description       : 返回各个项配置
@ Input             : None
@ Output            : None;
@ Return            : BaseDataConfig;
*******************************************************************************/
BaseDataConfig ModBusProtocol::getConfig()
{
    BaseDataConfig config;

    config.doubleYxnum = m_data.doubleYx.size();
    config.singleYxnum = m_data.singleYx.size();
    config.doubleYknum = m_config.ykNum;
    config.singleYknum = m_config.ykNum;
    config.validYcnum  = m_data.Yc.size();
    config.validYmnum  = m_data.Ym.size();
    config.B32YcNum    = m_data.floatYc.size();
    config.validYtNum  = (m_data.yt.size() 
                                + ((m_config.setYcTHV) ? (m_data.Yc.size()) : 0));  //遥调数目
    config.modbusDeviceNum = m_config.deviceNum;
	config.modbusSerialIndex = m_config.serialPort;
    config.validYcCircuitNum = m_baseDataConfig.validYcCircuitNum;

    m_baseDataConfig = config;

    DebugPrint("ModbusProtocol config: doubleYxNum=%d, singleYxNum=%d, doubleYkNum=%d"\
                "singleYkNum=%d, validYcNum=%d, valieYtNum=%d, B32YcNum=%d", 
                config.doubleYxnum, config.singleYxnum, config.doubleYknum, 
                config.singleYknum, config.validYcnum, config.validYtNum, config.B32YcNum);  

    return config;
}

/*******************************************************************************
@ Function Name     : initVector
@ Description       : 使用vector.swap()释放多余的内存空间
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::initVector()
{
    freeVectorSpare(m_data.doubleYx);
    freeVectorSpare(m_data.singleYx);
    freeVectorSpare(m_data.Yc);
    freeVectorSpare(m_data.floatYc);
    freeVectorSpare(m_data.Ym);
    freeVectorSpare(m_data.yt);
    freeVectorSpare(m_dataParse.doubleYx);
    freeVectorSpare(m_dataParse.singleYx);
    freeVectorSpare(m_dataParse.yc);
    freeVectorSpare(m_dataParse.floatYc);
    freeVectorSpare(m_dataParse.ym);
    freeVectorSpare(m_dataParse.yt);

	freeVectorSpare(m_readFileRequest);
    freeVectorSpare(m_readRequest);
    freeVectorSpare(m_writeYK);
    freeVectorSpare(m_writeReg);
}

/*******************************************************************************
@ Function Name     : initCanData
@ Description       : 初始化CAN数据
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::initData()
{
    m_updateBuff.status = ModbusUpdateStCodeNone;    
}

/*******************************************************************************
@ Function Name     : recvFrame
@ Description       : 接收处理
@ Input             : baseCircuitAddr
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::recvFrame(const SerialNetBuf &frame) 
{
    if (t_RcvOver.CheckTimeOut() && !m_frameRecvBuff.empty())       //上一帧字节接收超时
    {
        printfs(LOG_DEBUG, "字节接收超时。");
        m_frameRecvBuff.clear();
    }
        
    m_frameRecvBuff.insert(m_frameRecvBuff.end(), frame.Buf, frame.Buf + frame.BufLen);
    t_RcvOver.StartMsTimer();
    
    if (m_frameRecvBuff.size() < 5){
        return false;
    }
    if (m_frameRecvBuff.size() >= 256)
    {
        m_frameRecvBuff.clear();
        t_RcvOver.EndTimer();
        return false;
    }
   
    while(!m_frameRecvBuff.empty())
    {
        uint16_t i = 0;
        for (i=0; i < m_version.size(); i++)
        {
            if ((m_frameRecvBuff.front() == m_version[i].deviceId) || (m_frameRecvBuff.front()==0x00))
                break;
        }        
        if (i >= m_version.size())
            m_frameRecvBuff.pop_front();                //推出不符合配置ID得字节
        else
            break;
    }

    #if 1 
    uint16_t crcLen = CrcMake::ModBusFrameGet(m_frameRecvBuff);
    if (crcLen == 0) 
    {
//        printfs(LOG_INFO,"crc校验暂未通过。fifo目前大小:%d", (uint16_t)m_frameRecvBuff.size());
        return false;
    }
	
    uint16_t len = 0;

    if (crcLen >= len)
        len = crcLen;

    if (m_frameRecvBuff.size() >= len) 
    {
        SerialNetBuf tFrame;

        while (tFrame.BufLen < len && !m_frameRecvBuff.empty()) 
        {
            tFrame.Buf[tFrame.BufLen ++] = m_frameRecvBuff.front();
            m_frameRecvBuff.pop_front();
        }
        if (CrcMake::MakeCrc(tFrame.Buf, tFrame.BufLen) == 0) 
        {
            if (d_recvErrCount && *d_recvErrCount > 0) 
            {
                (*d_recvErrCount) --;
                *d_resetCount   = 0;
            }
            m_recviedFlag = true;
            modbusPrintBuf(LOG_DEBUG, "%sRecv: ", tFrame.Buf, tFrame.BufLen, m_setPoll?" ":"备用监听-");
            return m_recvFrameFifo.pushBack(tFrame);
        }
    }
#endif
return false;
}

/*******************************************************************************
@ Function Name     : poll
@ Description       : 根据当前轮询状态开始轮询
@ Input             : frame: ModBus接收帧
@ Output            : None;
@ Return            : true or false;
*******************************************************************************/
void ModBusProtocol::poll()
{
    if((d_doubleBackup == NULL )|| m_setPoll)
    {
        if (m_reqFifo.isempty() && t_poll.CheckTimeOut()) //m_mectime为timeout 时间
        {
            pollSendReq();
            t_poll.StartMsTimer();
        }
    }
    
    if ((m_reqRecord.rspStat == rspFinished))
    {
        MBusReqFifo reqcmd;
		
        if (m_reqFifo.front(reqcmd))
        {
            if (reqcmd.reqDelaySend) {
        	    t_delaySend.StartMsTimer(reqcmd.reqDelaySend);
            } else {
                t_delaySend.StartMsTimer(m_config.ackSendTime);
            }
            m_reqRecord.rspStat = rspWaiting;
        }
    }
}

/*******************************************************************************
@ Function Name     : pollSendReq
@ Description       : 根据当前轮询状态开始轮询
@ Input             : frame: ModBus接收帧
@ Output            : None;
@ Return            : true or false;
*******************************************************************************/
void ModBusProtocol::pollSendReq()
{
    setFrame(m_reqRecord.pollReqPtr);
    
    if (++m_reqRecord.pollReqPtr >= m_readRequest.size())
    {
        m_reqRecord.pollReqPtr = 0;
        m_firstPoll = false;
    }
}

/*******************************************************************************
@ Function Name     : sendReadSoeFile
@ Description       : 向发送队列插入soe请求帧
@ Input             : @type: 帧类型
                      @startaddr: 起始地址
                      @data: 数据列表
@ Output            : None;
@ Return            : void 
*******************************************************************************/
bool ModBusProtocol::sendReadSoeFile(uint8_t soeNum,uint8_t deviceID)
{
	MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    //uint8_t        deviceID   = 0;
    uint8_t        funcode    = 0;
	uint8_t		 quoteType = 0;
    uint16_t       fileNum    = 0;
    uint16_t       recordNum  = 0;
	uint16_t		 recordLength = 0;
    uint8_t        requestIndex = 0;
	
    MBusReadFileRequest *readFileRequest = NULL;

	for (uint8_t i=0; i<m_readFileRequest.size(); i++) {
		if (m_readFileRequest[i].deviceAddr == deviceID) {
			readFileRequest = &m_readFileRequest[i];
            requestIndex    = i;
		}
	}
	if (readFileRequest == NULL) {
		modbusPrint(LOG_WARNING, "设备:%d 无对应的SOE查询请求", deviceID);
		return false;
	}
	fileNum    = readFileRequest->fileNum;
	if(fileNum == MD_FILE_NUM_SOE){
		//deviceID = readFileRequest->deviceAddr;
        funcode  = readFileRequest->funCode;
		quoteType = readFileRequest->quoteType;
		recordNum = readFileRequest->recordNum;
		recordLength = readFileRequest->recordLength;
	}
	 
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
	frame.Buf[frame.BufLen ++] = 0;										//总字节数
	for(uint8_t soeIndex = 0; soeIndex < soeNum; soeIndex++){
		frame.Buf[frame.BufLen ++] = quoteType;
	    frame.Buf[frame.BufLen ++] = (fileNum >> 8)& 0xff;
	    frame.Buf[frame.BufLen ++] = fileNum & 0xff;
	    frame.Buf[frame.BufLen ++] = (recordNum >> 8)& 0xff;
	    frame.Buf[frame.BufLen ++] = recordNum & 0xff;
		frame.Buf[frame.BufLen ++] = (recordLength >> 8)& 0xff;
	    frame.Buf[frame.BufLen ++] = recordLength & 0xff;
	}
    
	frame.Buf[2] = frame.BufLen - 3;									//减去id和功能码
    
    reqcmd.reqType        = rspReadFile;
	reqcmd.reqPtr		  = requestIndex;
	reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameYxSoe;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_DEBUG, "读文件遥信SOE数目: ID:%d 功能码:%d ", deviceID, funcode);
	return true;
}

/*******************************************************************************
@ Function Name     : sendWriteYkSelect
@ Description       : 向发送队列插入SBO帧
@ Input             : @type: 帧类型
                      @startaddr: 起始地址
                      @data: 数据列表
@ Output            : None;
@ Return            : void 
*******************************************************************************/
bool ModBusProtocol::sendWriteYkSelect(MBusFrameType type, uint16_t startaddr, vector<uint16_t> &data)
{
    map<uint16_t,  MBusMultiWriteYk>::iterator it = m_selectYK.find(startaddr);

    if (it == m_selectYK.end())
        return false;

    MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    uint8_t        deviceID   = 0;
    uint8_t        funcode    = 0;
    uint16_t       startReg   = 0;
    uint16_t       index      = 0;
    uint16_t       delayTime  = 0;
    uint8_t        num        = 0;
    MBusMultiWriteYk &writeRequest = it->second;
    MBusMultiYkType  *ykType = NULL;
    
    deviceID = writeRequest.deviceAddr;
    funcode  = writeRequest.funCode;
    index    = writeRequest.readIndex;
    delayTime= writeRequest.delayReadTime;
    switch (data[0]) {
        case 0x0000: ykType = &writeRequest.ykOff; 
            modbusPrint(LOG_ERROR, "遥控选择分: ID:%d 功能码:%d ", deviceID, funcode);
            break;
        case 0xFF00: ykType = &writeRequest.ykOn; 
            modbusPrint(LOG_ERROR, "遥控选择合: ID:%d 功能码:%d ", deviceID, funcode);
            break;
        case 0xFFFF: ykType = &writeRequest.ykAbort; 
            modbusPrint(LOG_ERROR, "遥控撤销: ID:%d 功能码:%d ", deviceID, funcode);
            break;
        default: break;
    }

    if (!ykType)
        return false;
    
    startReg = ykType->startAddr;
    num      = ykType->num;
    data.clear();
    data.insert(data.begin(), ykType->action.begin(), ykType->action.end());
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = (startReg >> 8) & 0xFF;
    frame.Buf[frame.BufLen ++] = startReg & 0xFF;
    switch (funcode) {
        case FUNC_WR_SIGREG:
        case FUNC_YK: {    
            frame.Buf[frame.BufLen ++] = data[0] >> 8;
            frame.Buf[frame.BufLen ++] = data[0];
            break;
        }
        case FUNC_YK_MULT: {
            frame.Buf[frame.BufLen ++] = num >> 8;
            frame.Buf[frame.BufLen ++] = num;
            frame.Buf[frame.BufLen ++] = num / 2 + (num % 8 ? 1 : 0);
            for (size_t i=0; i<data.size(); i++) {
                frame.Buf[frame.BufLen ++] = data[i];
            }
            break;
        }
        case FUNC_WR_MULTREG: {
            frame.Buf[frame.BufLen ++] = num >> 8;
            frame.Buf[frame.BufLen ++] = num;
            frame.Buf[frame.BufLen ++] = num * 2;
            for (size_t i=0; i<data.size(); i++) {
                frame.Buf[frame.BufLen ++] = data[i] >> 8;
                frame.Buf[frame.BufLen ++] = data[i];
            }
            break;
        }
        default: break;
    }
    
    reqcmd.reqType        = rspWriteReq;
    reqcmd.reqPtr         = index;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = type;
    reqcmd.reqDelaySend   = 0;
    reqcmd.reqAckDelaySendTime = delayTime;

    m_reqFifo.pushBack(reqcmd);

    return true;
}

/*******************************************************************************
@ Function Name     : sendReadYxSoeNum
@ Description       : 向发送队列插入读文件命令
@ Input             : @yxaddr:遥信地址
                      @startaddr: 起始地址
                      @data: 数据列表
@ Output            : None;
@ Return            : void 
*******************************************************************************/
void ModBusProtocol::sendReadYxSoeNum(MBusFrameType type,uint16_t yxaddr, uint8_t soeType)
{
    MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    uint8_t        deviceID   = 0;
    uint8_t        funcode    = 0;
	uint8_t		 quoteType = 0;
    uint16_t       fileNum    = 0;
    uint16_t       recordNum  = 0;
	uint16_t		 recordLength = 0;
	uint16_t       index      = 0;
	bool		 noYx = true;

	
    MBusReadFileRequest *readFileRequest = NULL;
    for (int8_t i=m_readFileRequest.size()-1; i>=0; i--)
    {
        readFileRequest = &m_readFileRequest[i];
		if(soeType == frameSingleYx){
			if ((yxaddr >= readFileRequest->soeBaseAddr.singleYxBaseAddr) && 
				(yxaddr < (readFileRequest->soeBaseAddr.singleYxBaseAddr + readFileRequest->soeNum.singleYxNum)))
	        {
	        	fileNum    = readFileRequest->fileNum;
				if(fileNum == MD_FILE_NUM_SOE_NUM){
					deviceID = readFileRequest->deviceAddr;
		            funcode  = readFileRequest->funCode;
					quoteType = readFileRequest->quoteType;
					recordNum = readFileRequest->recordNum;
					recordLength = readFileRequest->recordLength;
					index = i;
					noYx = false; 
		            break;
				}
	            
	        }
		}
		else{
			if ((yxaddr >= readFileRequest->soeBaseAddr.doubleYxBaseAddr) &&
				(yxaddr < (readFileRequest->soeBaseAddr.doubleYxBaseAddr + readFileRequest->soeNum.doubleYxNum)))
	        {
	        	fileNum    = readFileRequest->fileNum;
				if(fileNum == MD_FILE_NUM_SOE_NUM){
					deviceID = readFileRequest->deviceAddr;
		            funcode  = readFileRequest->funCode;
					quoteType = readFileRequest->quoteType;
					recordNum = readFileRequest->recordNum;
					recordLength = readFileRequest->recordLength;
					index = i;
					noYx = false; 
		            break;
				}
	            
	        }
		}
    }
   	if(noYx){
		return;
	}
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
	frame.Buf[frame.BufLen ++] = 0;
    frame.Buf[frame.BufLen ++] = quoteType;
    frame.Buf[frame.BufLen ++] = (fileNum >> 8)& 0xff;
    frame.Buf[frame.BufLen ++] = fileNum & 0xff;
    frame.Buf[frame.BufLen ++] = (recordNum >> 8)& 0xff;
    frame.Buf[frame.BufLen ++] = recordNum & 0xff;
	frame.Buf[frame.BufLen ++] = (recordLength >> 8)& 0xff;
    frame.Buf[frame.BufLen ++] = recordLength & 0xff;
	frame.Buf[2]			   = frame.BufLen - 3;	
	
    reqcmd.reqType        = rspReadFile;
	reqcmd.reqPtr		  = index;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = type;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_DEBUG, "读文件SOE数目帧: ID:%d 功能码:%d ", deviceID, funcode);
}
/*******************************************************************************
@ Function Name     : sendReadSYxSoe
@ Description       : 向发送队列插入读单点遥信文件命令
@ Input             : @yxaddr:遥信地址
                      @startaddr: 起始地址
                      @data: 数据列表
@ Output            : None;
@ Return            : void 
*******************************************************************************/
void ModBusProtocol::sendReadSYxSoe(MBusFrameType type, uint16_t soeNum, uint16_t yxaddr, uint8_t soeType)
{
    MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    uint8_t        deviceID   = 0;
    uint8_t        funcode    = 0;
	uint8_t		 quoteType = 0;
    uint16_t       fileNum    = 0;
    uint16_t       recordNum  = 0;
	uint16_t		 recordLength = 0;	
	uint16_t       index      = 0;
	bool		 noYx = true;
	
    MBusReadFileRequest *readFileRequest = NULL;
    for (int8_t i=m_readFileRequest.size()-1; i>=0; i--)
    {
        readFileRequest = &m_readFileRequest[i];
		if(soeType == frameSingleYx){
			if ((yxaddr >= readFileRequest->soeBaseAddr.singleYxBaseAddr) && 
				(yxaddr < (readFileRequest->soeBaseAddr.singleYxBaseAddr + readFileRequest->soeNum.singleYxNum)))
	        {
	        	fileNum    = readFileRequest->fileNum;
				if(fileNum == MD_FILE_NUM_SOE){
					deviceID = readFileRequest->deviceAddr;
			        funcode  = readFileRequest->funCode;
					quoteType = readFileRequest->quoteType;
					recordNum = readFileRequest->recordNum;
					recordLength = readFileRequest->recordLength;
					index = i;
					noYx = false;
			        break;
				}
	        }
		}
		else{
			if ((yxaddr >= readFileRequest->soeBaseAddr.doubleYxBaseAddr) && 
				(yxaddr < (readFileRequest->soeBaseAddr.doubleYxBaseAddr + readFileRequest->soeNum.doubleYxNum)))
	        {
	        	fileNum    = readFileRequest->fileNum;
				if(fileNum == MD_FILE_NUM_SOE){
					deviceID = readFileRequest->deviceAddr;
			        funcode  = readFileRequest->funCode;
					quoteType = readFileRequest->quoteType;
					recordNum = readFileRequest->recordNum;
					recordLength = readFileRequest->recordLength;
					index = i;
					noYx = false;
			        break;
				}
	        }
		}
    }
	
    if(noYx){
		return;
	}
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
	frame.Buf[frame.BufLen ++] = 0;										//总字节数
	for(uint8_t soeIndex = 0; soeIndex < soeNum; soeIndex++){
		frame.Buf[frame.BufLen ++] = quoteType;
	    frame.Buf[frame.BufLen ++] = (fileNum >> 8)& 0xff;
	    frame.Buf[frame.BufLen ++] = fileNum & 0xff;
	    frame.Buf[frame.BufLen ++] = (recordNum >> 8)& 0xff;
	    frame.Buf[frame.BufLen ++] = recordNum & 0xff;
		frame.Buf[frame.BufLen ++] = (recordLength >> 8)& 0xff;
	    frame.Buf[frame.BufLen ++] = recordLength & 0xff;
	}
    
	frame.Buf[2] = frame.BufLen - 3;									//减去id和功能码
    
    reqcmd.reqType        = rspReadFile;
	reqcmd.reqPtr		  = index;
	reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = type;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_DEBUG, "读文件遥信SOE数目: ID:%d 功能码:%d ", deviceID, funcode);
}

/*******************************************************************************
@ Function Name     : sendWriteYk
@ Description       : 向发送队列插入写命令帧
@ Input             : @type: 帧类型
                      @startaddr: 起始地址
                      @data: 数据列表
@ Output            : None;
@ Return            : void 
*******************************************************************************/
void ModBusProtocol::sendWriteYk(MBusFrameType type, uint16_t startaddr, vector<uint16_t> &data)
{
    MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    uint8_t        deviceID   = 0;
    uint8_t        funcode    = 0;
    uint16_t       startReg   = 0;
    uint16_t       index      = 0;
    uint16_t       delayTime  = 0;
    uint8_t        num        = 0;

    if (type == frameYk)
    {
        MBusWriteYk *writeRequest = NULL;
        for (int8_t i=m_writeYK.size()-1; i>=0; i--)
        {
            writeRequest = &m_writeYK[i];
            if (startaddr >= writeRequest->baseAddr 
                && startaddr < (writeRequest->baseAddr + writeRequest->num))
            {
                uint8_t ykIndex = startaddr - writeRequest->baseAddr;

                deviceID = writeRequest->deviceAddr;
                funcode  = writeRequest->funCode;
                index    = writeRequest->readIndex;
                delayTime= writeRequest->delayReadTime;
                if (data[0] == 0x0000)
                {
                    startReg = writeRequest->doubleYk[ykIndex].ykOff.addr;
                    data[0]  = writeRequest->doubleYk[ykIndex].ykOff.action;
                }
                else
                {
                    startReg = writeRequest->doubleYk[ykIndex].ykOn.addr;
                    data[0]  = writeRequest->doubleYk[ykIndex].ykOn.action;
                }
                break;
            }
        }
        //遥控顺序排列，若无对应的单路遥控则查询多路遥控
        if (deviceID == 0) {
            MBusMultiWriteYk *writeRequest = NULL;
            for (int8_t i=m_writeMultiYK.size()-1; i>=0; i--)
            {
                writeRequest = &m_writeMultiYK[i];
                if (startaddr >= writeRequest->baseAddr 
                    && startaddr < (writeRequest->baseAddr + writeRequest->num))
                {
                    deviceID = writeRequest->deviceAddr;
                    funcode  = writeRequest->funCode;
                    index    = writeRequest->readIndex;
                    delayTime= writeRequest->delayReadTime;
                    if (data[0] == 0x0000)
                    {
                        startReg = writeRequest->ykOff.startAddr;
                        num      = writeRequest->ykOff.num;
                        data.clear();
                        data.insert(data.begin(), writeRequest->ykOff.action.begin(), writeRequest->ykOff.action.end());
                    }
                    else
                    {
                        startReg = writeRequest->ykOn.startAddr;
                        num      = writeRequest->ykOn.num;
                        data.clear();
                        data.insert(data.begin(), writeRequest->ykOn.action.begin(), writeRequest->ykOn.action.end());
                    }
                    break;
                }
            }
        }
    }
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = (startReg >> 8) & 0xFF;
    frame.Buf[frame.BufLen ++] = startReg & 0xFF;
    switch (funcode) {
        case FUNC_WR_SIGREG:
        case FUNC_YK: {    
            frame.Buf[frame.BufLen ++] = data[0] >> 8;
            frame.Buf[frame.BufLen ++] = data[0];
            break;
        }
        case FUNC_YK_MULT: {
            frame.Buf[frame.BufLen ++] = num >> 8;
            frame.Buf[frame.BufLen ++] = num;
            frame.Buf[frame.BufLen ++] = num / 2 + (num % 8 ? 1 : 0);
            for (size_t i=0; i<data.size(); i++) {
                frame.Buf[frame.BufLen ++] = data[i];
            }
            break;
        }
        case FUNC_WR_MULTREG: {
            frame.Buf[frame.BufLen ++] = num >> 8;
            frame.Buf[frame.BufLen ++] = num;
            frame.Buf[frame.BufLen ++] = num * 2;
            for (size_t i=0; i<data.size(); i++) {
                frame.Buf[frame.BufLen ++] = data[i] >> 8;
                frame.Buf[frame.BufLen ++] = data[i];
            }
            break;
        }
        default: break;
    }
    
    reqcmd.reqType        = rspWriteReq;
    reqcmd.reqPtr         = index;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = type;
    reqcmd.reqDelaySend   = 0;
    reqcmd.reqAckDelaySendTime = delayTime;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_ERROR, "遥控: ID:%d 功能码:%d ", deviceID, funcode);
}

/*******************************************************************************
@ Function Name     : sendYtReg
@ Description       : 向发送队列插入写命令帧
@ Input             : @type: 帧类型
                      @startaddr: 起始地址
                      @data: 数据列表
@ Output            : None;
@ Return            : void 
*******************************************************************************/
void ModBusProtocol::sendYtReg(MBusFrameType type, uint16_t addrOffset, vector<uint16_t> &data)
{
    MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    uint8_t        deviceID   = 0;
    //uint8_t        funcode    = 0;
    uint16_t       startReg   = 0;
    uint16_t        index      = 0;

 	if(type == frameYt){
		MBusYt *writeRequest = NULL;
        writeRequest = &m_ytReg[addrOffset];
        
        deviceID    = writeRequest->request.deviceAddr;
        //funcode     = writeRequest->request.funcode;
        startReg    = writeRequest->request.startAddr + writeRequest->ytUintAddrOffset;
        index       = writeRequest->readIndex;
	}
    //funcode = funcode;
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = 0x06;
    frame.Buf[frame.BufLen ++] = (startReg >> 8) & 0xFF;
    frame.Buf[frame.BufLen ++] = startReg & 0xFF;
    frame.Buf[frame.BufLen ++] = data[0] >> 8;
    frame.Buf[frame.BufLen ++] = data[0];

    reqcmd.reqType        = rspWriteReq;
    reqcmd.reqPtr         = index;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = type;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_INFO, "写数据: ID:%d 功能码:6 ", deviceID);
}

/*******************************************************************************
@ Function Name     : sendWrReg
@ Description       : 向发送队列插入写命令帧
@ Input             : @type: 帧类型
                      @startaddr: 起始地址
                      @data: 数据列表
@ Output            : None;
@ Return            : void 
*******************************************************************************/
void ModBusProtocol::sendYtReg(uint8_t deviceID, uint16_t addr, uint16_t value)
{
    MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = 0x06;
    frame.Buf[frame.BufLen ++] = (addr >> 8) & 0xFF;
    frame.Buf[frame.BufLen ++] = addr & 0xFF;
    frame.Buf[frame.BufLen ++] = value >> 8;
    frame.Buf[frame.BufLen ++] = value;

    reqcmd.reqType        = rspWriteReq;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameYt;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_INFO, "透传写数据: ID:%d addr:%d value:%d ", deviceID, addr, value);
}

/*******************************************************************************
@ Function Name     : sendWriteReg
@ Description       : 向发送队列插入写命令帧
@ Input             : @type: 帧类型
                      @startaddr: 起始地址
                      @data: 数据列表
@ Output            : None;
@ Return            : void 
*******************************************************************************/
void ModBusProtocol::sendMutiWriteReg(MBusFrameType type, uint16_t startaddr, vector<uint16_t> &data)
{
	MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    uint8_t        deviceID   = 0;
    uint8_t        funcode    = 0;
    uint16_t       startReg   = 0;
    uint16_t       index      = 0;
	uint16_t		 regNum		= 0;

    if (type == frameMultiWR)
    {
        MBusWriteReg *writeRequest = NULL;
        for (uint16_t i=0; i<m_writeReg.size(); i++)
        {
            writeRequest = &m_writeReg[i];
            if (startaddr >= writeRequest->baseAddr 
                && startaddr < (writeRequest->baseAddr + writeRequest->num))
            {
                deviceID    = writeRequest->deviceAddr;
                funcode     = writeRequest->funCode;
                startReg    = writeRequest->regAddr[startaddr - writeRequest->baseAddr];
				regNum		= writeRequest->num;
                index       = writeRequest->readIndex;
                break;
            }
        }
    }
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = (startReg >> 8) & 0xFF;
    frame.Buf[frame.BufLen ++] = startReg & 0xFF;
	frame.Buf[frame.BufLen ++] = (regNum >> 8) &0xFF;
	frame.Buf[frame.BufLen ++] = regNum & 0xFF;
	frame.Buf[frame.BufLen ++] = (regNum & 0xFF)*2;
    for (int i=0; i<regNum*2; i++) {
        frame.Buf[frame.BufLen ++] = data[i];
    }
	if (deviceID == BOARDCAST_ID) {
		reqcmd.reqType        = rspNoReq;
	}
	else {
    	reqcmd.reqType        = rspWriteReq;
	}
    reqcmd.reqPtr         = index;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = type;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_INFO, "写多个数据: ID:%d 功能码:%d ", deviceID, funcode);
}
/*******************************************************************************
@ Function Name     : SendReqCmd
@ Description       : 发送帧
@ Input             : frame: ModBus接收帧
@ Output            : None;
@ Return            : true or false;
*******************************************************************************/
void ModBusProtocol::SendReqCmd()
{
    MBusReqFifo reqcmd;

    if (m_reqFifo.front(reqcmd))
    {
        SerialNetBuf frame = reqcmd.reqFrame;

        sendMBusFrame(frame);
        m_reqRecord.rspStat   = rspWaiting;        
		
        if(frame.Buf[1] == FUNC_UPDATE_BOTTOM && frame.Buf[0] == BOARDCAST_ID)
        {	
            m_brocastUpdate.StartTimer();
            m_bocastUpdateCmd = (UpdateCmd)frame.Buf[2];
        }
    }
}
    
/*******************************************************************************
@ Function Name     : dealMBusFrame
@ Description       : 处理ModBus接收帧
@ Input             : frame: ModBus接收帧
@ Output            : None;
@ Return            : true or false;
*******************************************************************************/
bool ModBusProtocol::dealMBusFrame(char *ipv4)
{
    SerialNetBuf frame;        
    if (!m_recvFrameFifo.popFront(frame))
    {
        return false;
    }
    
    MBusReqFifo  reqcmd;    	

    if (!m_reqFifo.popFront(reqcmd))
    {   
        printfs(LOG_INFO, "主备模式:%s, 当前无存储的请求帧", m_setPoll?"工作态":"监听态");
        return false;
    }
    
    t_rsp.EndTimer();
    m_reqRecord.rspStat = rspFinished;
	
    switch (reqcmd.reqType)
    {
        case rspReadReq:
            {
                processReadRequest(frame, reqcmd);
                break;
            }
        case rspWriteReq:
            {                
                processWriteRequest(frame, reqcmd);
                break;
            }
		case rspReadFile:
            {                
                processReadFileRequest(frame, reqcmd,ipv4);
                break;
            }
        case rspUpdate:
            {
                processUpdateRequest(frame, reqcmd);
                break;
            }
        case rspReadRecord:
            {
                processRecordRequest(frame, reqcmd, ipv4);
                break;
            }
        case rspNoDeal:
            {                
                break;
            }
        default:
            {
                modbusPrint(LOG_INFO, "%s:无该请求命令类型:%d\n", ipv4?ipv4:" ", reqcmd.reqType);
                return false;
            }
    }
    return true;
}

/*******************************************************************************
@ Function Name     : protocolOccured
@ Description       : CAN 模块突发事件处理
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::protocolOccured(char *ipv4)
{
    if (t_rsp.CheckTimeOut())
    {
        MBusReqFifo reqcmd;
        t_rsp.EndTimer();  
        m_reqRecord.rspStat = rspFinished;
        
        if (!m_reqFifo.popFront(reqcmd))
            return false;
        
        if (reqcmd.reqType != rspNoReq)
        {
            modbusPrint(LOG_INFO, "%s%s请求超时!",m_setPoll ? " " : "备用监听-", ipv4?ipv4:"");
            
            if (reqcmd.reqType == rspReadFile)				//超时发送读录波文件帧
            {
                if(waveTimeOutCounter >= 3)
                    return false;
                m_reqFifo.pushBack(reqcmd);
                waveTimeOutCounter = 0;
                
                if(reqcmd.reqFrameType == frameYxSoeNum){       //soe请求帧超时计数
                    m_yxSoeCheck.soeNumTimeOutCounter++;
                    m_yxSoeCheck.isSoeNumCheck = true;
                }
                else if(reqcmd.reqFrameType == frameYxSoe){
                    m_yxSoeCheck.soeTimeOutCounter ++;
                    m_yxSoeCheck.isSoeCheck = true;
                    
                    if(m_yxSoeCheck.soeTimeOutCounter > 3){     //soe查询超时3次还原查询数目和soe查询标志位
                        m_yxSoeCheck.isSoeCheck = false;
                        m_yxSoeCheck.isSoeCheck = true;
                        m_yxSoeCheck.soeAckNum.doubleYx = 0;
                        m_yxSoeCheck.soeAckNum.singleYx = 0;
                    }
                }
            }                        
            if (m_recviedFlag) 
            {
                if (!m_frameRecvBuff.empty())
                    m_frameRecvBuff.clear();
                if (d_recvErrCount)
                    (*d_recvErrCount) ++;
            }
        }
		
        if (reqcmd.reqType == rspReadReq) {
            MBusRequest &request       = m_readRequest[reqcmd.reqPtr];
            if ((size_t)request.deviceIndex < m_deviceRetryCount.size() &&
                m_deviceRetryCount[request.deviceIndex] >= m_config.commStatTrytimes) {
                setCommStat(request.deviceIndex, false);
                m_deviceRetryCount[request.deviceIndex] = 0;
            }
        }
    }
    if (t_Yk.CheckTimeOut())
    {
        t_Yk.EndTimer();
        if (m_reqRecord.rspStat == rspWaiting)
        {
            m_reqRecord.rspStat = rspFinished;
        }
		m_reqFifo.clearAll();
        printfs(LOG_ERROR,  "遥控命令:%d 地址:%d 动作:%#x 选择超时!\n", 
                    m_ykCmdEvent.cmd, m_ykCmdEvent.addr, m_ykCmdEvent.data.data[0]);
        m_ykStatus                  = ykidle;
        m_ykCmdEvent.availability   = true;
        m_ykCmdEvent.state          = eventActCon;
        m_ykCmdEvent.value          = failed;

    }

    if (t_wave.CheckTimeOut())
    {
        t_wave.EndTimer();
        if (m_reqRecord.rspStat == rspWaiting)
        {
            m_reqRecord.rspStat = rspFinished;
            m_reqFifo.delFront();
            waveFrameCounter = 1;
            rcvWaveData.clear();
        }
        m_waveEvent.availability   = true;
        m_waveEvent.state          = eventActCon;
        m_waveEvent.value          = failed;
    }
	
    if (t_write.CheckTimeOut())
    {
        t_write.EndTimer();
        #if 0
        m_setParamEvent.availability= true;
        m_setParamEvent.state       = eventActCon;
        m_setParamEvent.value       = failed;
        #endif
        m_reqRecord.rspStat = rspFinished;
        m_reqFifo.delFront();
        m_isParamSetting = false;
		modbusPrint(LOG_INFO, "遥调命令应答超时");
    }
	
    if (t_delaySend.CheckTimeOut())
    {
        t_delaySend.EndTimer();
        SendReqCmd();
    }
    if (m_update.CheckTimeOut())
    {
        m_update.EndTimer();
        modbusPrint(LOG_ERROR, "在线升级超时! 超时时间: %d", m_update.getTime());
        m_updateEvent.availability = true;
        m_updateEvent.state        = eventActCon;
        m_updateEvent.value        = failed;
        UpdateStatReport();
                
    }
    if(m_Record.CheckTimeOut())
    {
        m_Record.EndTimer();
        modbusPrint(LOG_ERROR, "招取事件记录超时，超时时间：%d", m_Record.getTime());
        ReSendFrame();
    }

	//广播升级处理
	if(m_bocastUpdateCmd != updateCmdNone){

		if(m_brocastUpdate.CheckTimeOut()){
			m_brocastUpdate.EndTimer();
			simDownAckFrame(m_bocastUpdateCmd);
			m_bocastUpdateCmd = updateCmdNone;
		}
	}

	//定时授时帧
	if (t_SetTime.CheckTimeOut() && m_tSetTime == true)
	{
        t_SetTime.StartTimer();
		if((d_doubleBackup == NULL ) && 
            m_setPoll){
			setTimePeriod();
		}
	}
    return true;
}
/*******************************************************************************
@ Function Name     : processReadRequest
@ Description       : 处理读请求回复帧
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processReadRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd)
{	
	if(reqcmd.reqPtr >= m_readRequest.size()){
		modbusPrint(LOG_INFO, "解析帧索引号异常,源解析帧容器长度:%d, 检索帧索引序号:%d\n", m_readRequest.size(), frame.Buf[2]);
		return;
	}

	const MBusRequest &request = m_readRequest[reqcmd.reqPtr];  
    const uint8_t  id      = frame.Buf[0];
    uint16_t       rspCnt  = 0;
	/*
	printfs(LOG_DEBUG, "进入数据解析,解析格式:请求的设备地址:%d,请求的功能码:%d,请求的寄存器数目:%d,请求的文件数目%d,请求的文件长度%d,",request.deviceAddr,
		request.funcode, request.regNum, request.fileNum, request.recordLenth);
	*/
    if (frame.Buf[1] & 0x80)
    {
        modbusPrint(LOG_INFO, "从设备应答异常,异常码:%d\n", frame.Buf[2]);
        return ;
    }
    if (id != request.deviceAddr)
    {
        modbusPrint(LOG_INFO, "应答帧设备地址不正确, req=%d, rsp=%d!\n", 
                    request.deviceAddr, id);
        return ;
    }

    if (frame.Buf[1] != request.funcode)
    {
        modbusPrint(LOG_INFO, "应答帧功能不一致, req=%d, rsp=%d!\n",
                    request.funcode, frame.Buf[1]);
        return ;
    }

    if ((size_t)request.deviceIndex < m_deviceRetryCount.size()) {
        m_deviceRetryCount[request.deviceIndex] = 0;
        setCommStat(request.deviceIndex, true);
    }

	switch (request.funcode)
    {
        case FUNC_RD_INPUT:
        case FUNC_RD_COIL: rspCnt = request.regNum/8 + ((request.regNum%8)?1:0);break;
        case FUNC_RD_HOLDREG:
        case FUNC_RD_INPUTREG: rspCnt = request.regNum* 2; break;
		case FUNC_RD_FILE:
		{	
			if(request.fileNum == FILENUMERRREPORT)
			{
				rspCnt = request.recordNum*24;break;
			}
			if(request.fileNum == FILENUMWAVEDIR)			
			{
				rspCnt = request.recordLenth+2;break;
			}
		}
		case FUNC_RD_MIX:{
			for(uint8_t frameIndex = 0; frameIndex < request.subMBusRequest.size();frameIndex++){
				const MBusRequest &subRequest = request.subMBusRequest[frameIndex];
				switch(subRequest.funcode){
					case FUNC_RD_INPUT:
			        case FUNC_RD_COIL: rspCnt += ((subRequest.regNum/8 + ((subRequest.regNum%8)?1:0)) + 1);break;
			        case FUNC_RD_HOLDREG:
			        case FUNC_RD_INPUTREG: rspCnt += ((subRequest.regNum* 2) + 1); break;
					case FUNC_RD_FILE:
					{	
						if(subRequest.fileNum == FILENUMERRREPORT)
						{
							rspCnt += (subRequest.recordLenth + 3);break;
						}
						if(subRequest.fileNum == FILENUMWAVEDIR)			
						{
							rspCnt += (subRequest.recordLenth + 3);break;
						}
					}
					case FUNC_SOE:{
						rspCnt = frame.Buf[2];
						break;
					}
					default: modbusPrint(LOG_INFO, "不支持该功能码:%d\n", request.funcode); break;
				}
				//字节数判断临时处理
				if(d_doubleBackup == NULL){
					rspCnt = frame.Buf[2];
				}
			}
			break;
		}
        default: modbusPrint(LOG_INFO, "不支持该功能码:%d\n", request.funcode); return;
    }
    if (frame.Buf[2] != rspCnt || frame.Buf[2] < frame.BufLen - 5)
    {
        modbusPrint(LOG_INFO, "应答的字节数: %d与请求寄存器数目: %d不相符!\n",
                   frame.Buf[2] ,rspCnt );
		if (request.fileNum == FILENUMWAVECALL)
			rcvWaveData.clear();
        return ;
    }
    switch (request.type)
    {
        case frameSingleYx: 
        case frameDoubleYx: 	processYxEvent(request, frame); break;
        case frameYc:       	processYcEvent(request, frame); break;
        case frameInt32Yc:
        case frameFloatYc:  	processB32YcEvent(request, frame); break;
		case frameInt16Yc:		processB16YcEvent(request, frame);break;
		case frameInt8Yc:		processB8YcEvent(request, frame);break;

        case frameYm:       	processYmEvent(request, frame); break;
        case frameYt:       	processYtEvent(request, frame); break;
		case frameReadErrReport:processframeYcErrReportEvent(request, frame); break;
		case frameReadDir:		processframeYcWaveDir(request, frame); break;
		case frameMix:			processframeMix(request,frame);break;
        default:break;
    }
}

/*******************************************************************************
@ Function Name     : processWriteRequest
@ Description       : 处理写请求应答帧
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processWriteRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd)
{
    const SerialNetBuf   &request = reqcmd.reqFrame;
    uint8_t       id       = frame.Buf[0];

    if (id != request.Buf[0])
    {
        modbusPrint(LOG_INFO, "应答帧设备地址不正确, req=%d, rsp=%d!\n", 
                    request.Buf[0], id);
        return ;
    }

    if (frame.Buf[1] != request.Buf[1])
    {
        modbusPrint(LOG_INFO, "应答帧功能不一致, req=%d, rsp=%d!\n",
                    request.Buf[1], frame.Buf[1]);
        return ;
    }
    if (memcmp(&frame.Buf[2], &request.Buf[2], 4))                  //数据区不一致
    {
        modbusPrint(LOG_INFO, "应答数据与请求数据不一致!\n");
        return ;
    }

    switch (reqcmd.reqFrameType)
    {
        case frameYk:   
        {
            t_Yk.EndTimer(); 
            processYkEvent(frame); 
            setFrame(reqcmd.reqPtr, reqcmd.reqAckDelaySendTime);                //向命令队列推入遥信查询帧
            break;
        }
        case frameMultiYk:      break;
        case frameWR:  
        {
            t_write.EndTimer();
            processWriteEvent(frame);    
            setFrame(reqcmd.reqPtr, reqcmd.reqAckDelaySendTime);
            break;
        }
        case frameYt:
        {
            t_write.EndTimer();
            processWriteEvent(frame);
			setFrame(reqcmd.reqPtr, m_config.reqDelaySendTime);                 //向命令队列推入混合帧查询
            m_isParamSetting = false;
            break;
        }
        case frameMultiWR: 
		{
			t_write.EndTimer();
			processMultiWriteEvent(frame);
			setFrame(reqcmd.reqPtr, reqcmd.reqAckDelaySendTime);
			break;
    	}
        case frameYkSBOSelect:
        {
            processYkSelectEvent(frame);
            break;
        }
        default: 
        {
            modbusPrint(LOG_INFO, "不支持该帧格式:%d\n", 
                reqcmd.reqFrameType);     
            break;
        }
    }
}

/*******************************************************************************
@ Function Name     : processReadFileRequest
@ Description       : 处理读文件应答帧
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processReadFileRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd,char *devIpv4)
{
	#if 1
	const SerialNetBuf   &request = reqcmd.reqFrame;
	const MBusReadFileRequest &fileRequest = m_readFileRequest[reqcmd.reqPtr];
    uint8_t        id       = frame.Buf[0];
	uint16_t		 requestlength  = 0;
	uint16_t		 fileNum = (request.Buf[4] << 8 | request.Buf[5]);
	//uint16_t		 recordNum	= (request.Buf[6] << 8 | request.Buf[7]);

    if (frame.Buf[1] & 0x80)
    {
    	if(devIpv4 != NULL){
			modbusPrint(LOG_INFO, "%s:从设备应答异常,异常码:%d\n",devIpv4, frame.Buf[2]);
		}
		else{
			modbusPrint(LOG_INFO, "从设备应答异常,异常码:%d\n", frame.Buf[2]);
		}
        
        return ;
    }
    if (id != request.Buf[0])
    {
    	if(devIpv4 != NULL){
			modbusPrint(LOG_INFO, "%s:应答帧设备地址不正确, req=%d, rsp=%d!\n", devIpv4,request.Buf[0], id);
		}
		else{
			modbusPrint(LOG_INFO, "应答帧设备地址不正确, req=%d, rsp=%d!\n",request.Buf[0], id);
		}
        
        return ;
    }

    if (frame.Buf[1] != request.Buf[1])
    {
    	if(devIpv4 != NULL){
			modbusPrint(LOG_INFO, "%s:应答帧功能不一致, req=%d, rsp=%d!\n", devIpv4?devIpv4:" ",request.Buf[1], frame.Buf[1]);
		}
        else{
			modbusPrint(LOG_INFO, "应答帧功能不一致, req=%d, rsp=%d!\n",request.Buf[1], frame.Buf[1]);
		}
        return ;
    }

	if(fileNum == MD_FILE_NUM_SOE_NUM || fileNum == MD_FILE_NUM_SOE){
		
	}
	else{
		if(waveFrameCounter >= waveFrameNum)					//最后一帧数据长度
		{
			requestlength = fileLength %request.Buf[9];
		}
		else													//非最后一帧数据长度
		{
			requestlength = request.Buf[9];	
		}
		if ((frame.BufLen - 7) != requestlength)
	    {
	    	if(devIpv4 != NULL){
				modbusPrint(LOG_INFO, "%s:应答的字节数: %d与请求寄存器数目: %d不相符!\n",devIpv4,(frame.BufLen - 7), requestlength );
			}
			else{
				modbusPrint(LOG_INFO, "应答的字节数: %d与请求寄存器数目: %d不相符!\n",(frame.BufLen - 7), requestlength);	
			}
			
			rcvWaveData.clear();
			waveFrameCounter = 1;
            m_ycWaveNum.at(request.Buf[0]) = 0;
	        return ;
	    }
	}
	
    switch(reqcmd.reqFrameType)
    {
        case frameReadWave:   
        {
			processframeYcWaveCall(frame,devIpv4);
			if(dataZero)								//数据帧数据为0；
			{
				break;
				dataZero = false;
			}
            if(waveFrameCounter < waveFrameNum)
        	{
        		t_wave.EndTimer();
        		waveFrameCounter++;
        		setFrame(reqcmd.reqPtr);                 //向命令队列下一个录波召取帧
        	}
			else
			{
				waveFrameCounter = 1;
			}
			
            break;
        }
		case frameYxSoeNum:		processframeYxSoeNum(frame);break;
		case frameYxSoe:		processframeYxSoe(fileRequest,frame);break;
        default: 
            {
                modbusPrint(LOG_INFO, "%s:不支持该帧格式:%d\n",devIpv4, 
                    reqcmd.reqFrameType);     
                break;
            }
    }
		#endif
}
/*******************************************************************************
@ Function Name     : getNodeVersion
@ Description       : 获取指定类型和节点号的软件版本信息
@ Input             : type:     节点类型
                      cpuNum:   节点序号
@ Output            : 
@ Return            : 版本号
*******************************************************************************/
bool ModBusProtocol::getNodeVersion(const ModbusNodeType type, const uint8_t cpuNum, NodeInfo &info)
{       
    uint8_t  nodeSt  = 0;
    uint16_t nodeVer = 0;
	uint8_t deviceId = 0;

	nodeSt = m_version[cpuNum].stat;
	nodeVer= m_version[cpuNum].nodeVersion;
	deviceId = m_version[cpuNum].deviceId;

    info.nodeType = type;
    info.nodeID   = cpuNum;
    info.nodeLinkStat = nodeSt;
    info.nodeVersion  = nodeVer;
	info.nodeExtInfo[0] = deviceId;

    return true;
}

/*******************************************************************************
@ Function Name     : getNodeInfoCh
@ Description       : 获取指定类型和节点号的软件版本信息
@ Input             : type:     节点类型
                      cpuNum:   节点序号
@ Output            : 
@ Return            : 版本号
*******************************************************************************/
bool ModBusProtocol:: getNodeInfoCh(NodeInfo &infoSoe)
{
    return m_nodeInfoSoeFifo.popFront(infoSoe);
}

/*******************************************************************************
@ Function Name     : getDoubleYxSoe
@ Description       : 读取双点遥信SOE
@ Input             : None
@ Output            : soe;
@ Return            : true or false
*******************************************************************************/
bool ModBusProtocol::getDoubleYxSoe(DataSoe &soe)
{
    return m_doubleYxSoeFifo.popFront(soe);
}

/*******************************************************************************
@ Function Name     : getSingleYxSoe
@ Description       : 读取单点遥信SOE
@ Input             : None
@ Output            : soe;
@ Return            : true or false;
*******************************************************************************/
bool ModBusProtocol::getSingleYxSoe(DataSoe &soe)
{
	return m_singleYxSoeFifo.popFront(soe);
}

/*******************************************************************************
@ Function Name     : getValidYcSoe
@ Description       : 获取遥测SOE
@ Input             : None
@ Output            : soe;
@ Return            : true or false;
*******************************************************************************/
bool ModBusProtocol::getValidYcSoe(DataSoe &soe)
{
    return m_validYcSoeFifo.popFront(soe);
}

/*******************************************************************************
@ Function Name     : getFP32YcSoe
@ Description       : 获取短浮点遥测SOE
@ Input             : None
@ Output            : soe;
@ Return            : true or false;
*******************************************************************************/
bool ModBusProtocol::getFP32YcSoe(DataFP32Soe &soe)
{
    return m_FP32YcSoeFifo.popFront(soe);
}

/*******************************************************************************
@ Function Name     : getDoubleYxData
@ Description       : 获取双点遥信数据
@ Input             : addr:双点遥信地址
@ Output            : value 遥信状态值
@ Return            : true or false;
*******************************************************************************/
bool ModBusProtocol::getDoubleYxData(const uint16_t addr, uint8_t &value, bool change)
{
    if (!change)
    {
        if (addr < m_data.doubleYx.size())
        {
            value = m_data.doubleYx[addr].value;
            return true;
        }
    }
    else
    {
        if (addr <m_data.doubleYx.size())
        {
            if (m_data.doubleYx[addr].change)
            {
                value = m_data.doubleYx[addr].value;
				modbusPrint(LOG_INFO, "双点遥信地址:%d 变位\n", value);
                m_data.doubleYx[addr].change = false; 
                return true;
            }
        }
    }

    return false;
}

/*******************************************************************************
@ Function Name     : getSingleYxData
@ Description       : 获取单点遥信数据
@ Input             : addr  目标遥信点地址
@ Output            : value 目标遥信点状态
@ Return            : true or false
*******************************************************************************/
bool ModBusProtocol::getSingleYxData(const uint16_t addr, uint8_t &value, bool change)
{
    if (!change)
    {
        if (addr < m_data.singleYx.size())
        {
            value = m_data.singleYx[addr].value;
            return true;
        }
    }
    else
    {
        if (addr <m_data.singleYx.size())
        {
            if (m_data.singleYx[addr].change)
            {
                value = m_data.singleYx[addr].value;
                m_data.singleYx[addr].change = false;
                return true;
            }
        }
    }

    return false;
}

/*******************************************************************************
@ Function Name     : getValidYcData
@ Description       : 获取遥测值
@ Input             : addr  目标遥测点地址
@ Output            : value 目标遥测点值
@ Return            : ture or false
*******************************************************************************/
bool ModBusProtocol::getValidYcData(const uint16_t addr, uint16_t &value, bool change)
{
    if (!change)
    {
        if (addr < m_data.Yc.size())
        {
            value = m_data.Yc[addr].value;
            return true;
        }
    }
    else
    {
        if (addr <m_data.Yc.size())
        {
            if (m_data.Yc[addr].change)
            {
                value = m_data.Yc[addr].value;
                m_data.Yc[addr].change = false;
                return true;
            }
        }
    }

    return false;
}

/*******************************************************************************
@ Function Name     : getValidYmData
@ Description       : 获取遥脉
@ Input             : addr  目标遥脉点地址
@ Output            : value 目标遥脉点值
@ Return            : ture or false
*******************************************************************************/
bool ModBusProtocol::getValidYmData(const uint16_t addr, int32_t &value, bool change)
{
    if (!change)
    {
        if (addr < m_data.Ym.size())
        {
            value = m_data.Ym[addr].value;
            return true;
        }
    }
    else
    {
        if (addr <m_data.Ym.size())
        {
            if (m_data.Ym[addr].change)
            {
                value = m_data.Ym[addr].value;
                m_data.Ym[addr].change = false;
                return true;
            }
        }
    }

    return false;
}

/*******************************************************************************
@ Function Name     : getFP32YcData
@ Description       : 获取32位浮点遥测值
@ Input             : addr  目标点地址
@ Output            : value 目标点值
@ Return            : ture or false
*******************************************************************************/
bool ModBusProtocol::getFP32YcData(const uint16_t addr, float &value, bool change)
{
    if (!change)
    {
        if (addr < m_data.floatYc.size())
        {
            value = m_data.floatYc[addr].value;
            return true;
        }
    }
    else
    {
        if (addr <m_data.floatYc.size())
        {
            if (m_data.floatYc[addr].change)
            {
                value = m_data.floatYc[addr].value;
                m_data.floatYc[addr].change = false;
                return true;
            }
        }
    }

    return false;
}

/*******************************************************************************
@ Function Name     : getEvent
@ Description       : 获取突发事件
@ Input             : m_event: 事件
@ Output            : event  : 事件输出
@ Return            : true:  新事件
                      false: 无事件
*******************************************************************************/
bool ModBusProtocol::getEvent(SpontEvent &event)
{
    if (m_ykCmdEvent.availability)
    {
        m_ykCmdEvent.availability = false;
        event = m_ykCmdEvent;
        
        return true;
    }
    else if (m_setParamEvent.availability)
    {
        m_setParamEvent.availability = false;
        event = m_setParamEvent;
        m_isParamSetting = false;
        if (m_data.yt.size() == 0)
            return false;
        return true;
    }
	else if (m_ycErrReportEvent.popFront(event))		//故障报告
    {
        //m_ycErrReportEvent.availability = false;
        //event = m_ycErrReportEvent;

        return true;
    }
    else if (m_updateEvent.availability)            //在线升级
    {
        m_updateEvent.availability = false;
        event = m_updateEvent;

        return true;
    }
    else if(m_readRecord.availability)
    {
        m_readRecord.availability = false;
        event = m_readRecord;

        return true;
    }

    return false;
}

/*******************************************************************************
@ Function Name     : setSingleYkCmd
@ Description       : 单点遥控命令
@ Input             : addr  单点遥控地址
@ Output            : sco   单点遥控执行动作
@ Return            : true or false
*******************************************************************************/
bool ModBusProtocol::setSingleYkCmd(const SpontEvent &event)
{
    uint16_t addr = 0;
    uint8_t dco  = 0;
    vector<uint16_t> data;

    addr = event.addr;
    dco  = event.data.data[0];
    
    if (event.state == eventStopAct)
    {
        m_ykCmdEvent              = event;
        m_ykCmdEvent.availability = false;

        data.push_back(0xFFFF);
        if (!sendWriteYkSelect(frameYkSBOSelect, addr, data)) {
            m_ykStatus                = ykidle;
            t_Yk.EndTimer();
            m_ykCmdEvent.availability = true;
            m_ykCmdEvent.state        = eventStopActCon;
            m_ykCmdEvent.value        = successed;
        }

        return true;
    }
 
    if (addr < m_baseDataConfig.singleYknum)
    {
        if (dco & 0x80)      //遥控选择
        {
            if (m_ykStatus == ykidle)
            {  
                t_Yk.StartTimer(20);
                
                m_ykCmdEvent              = event;
                m_ykCmdEvent.availability = false;
                if ((dco & 0x01) == 0x00)
                {
                    modbusPrint(LOG_ERROR, "地址:%d 选择分\n", addr);
                    data.push_back(0x0000);
                }
                else if ((dco & 0x01) == 0x01)
                {
                    modbusPrint(LOG_ERROR, "地址:%d 选择合\n", addr);
                    data.push_back(0xFF00);
                }
                if (!sendWriteYkSelect(frameYkSBOSelect, addr, data))
                {
                    m_ykStatus                  = ykselect;
                    m_ykCmdEvent.availability   = true;
                    m_ykCmdEvent.state          = eventActCon;
                    m_ykCmdEvent.value          = successed;
                }
                
                return true;
            }
        }
        else
        {
            //dealMBusFrame();
            if (m_ykStatus == ykselect)
            {
                if ((addr == m_ykCmdEvent.addr) && ((dco&0x01) == (m_ykCmdEvent.data.data[0]&0x01)))
                {
                    m_ykCmdEvent              = event;
                    m_ykCmdEvent.availability = false;
                    if ((dco & 0x01) == 0x00)
                    {
                        modbusPrint(LOG_ERROR, "地址:%d 控分\n", addr);
                        data.push_back(0x0000);
                    }
                    else if ((dco & 0x01) == 0x01)
                    {
                        modbusPrint(LOG_ERROR, "地址:%d 控合\n", addr);
                        data.push_back(0xFF00);
                    }
                    sendWriteYk(frameYk, addr, data);
                    t_Yk.StartTimer(m_config.ykTimeOut);
                    m_ykStatus = ykexecute;
                    
                    return true;
                }
                else
                {
                    t_Yk.EndTimer();
                    m_ykStatus = ykidle;
                    modbusPrint(LOG_ERROR, 
                        "遥控执行与选择不一致, 地址:sel=%d exe=%d 类型:sel=%d exe=%d\n",
                            m_ykCmdEvent.addr, addr, m_ykCmdEvent.data.data[0]&0x03, dco&0x03);
                }
            }
        }
    }
    
    m_ykCmdEvent                = event;
    m_ykCmdEvent.availability   = true;
    m_ykCmdEvent.state          = eventActCon;
    m_ykCmdEvent.value          = failed;
    
    return false;
}

/*******************************************************************************
@ Function Name     : setDoubleYkCmd
@ Description       : 双点遥控命令
@ Input             : addr  双点遥控地址
@ Output            : dco   双点遥控命令控制命令
@ Return            : true or false
*******************************************************************************/
bool ModBusProtocol::setDoubleYkCmd(const SpontEvent &event)
{
    uint16_t addr = 0;
    uint8_t dco  = 0;
    vector<uint16_t> data;

    addr = event.addr;
    dco  = event.data.data[0];
    
    if (event.state == eventStopAct)
    {
        m_ykCmdEvent              = event;
        m_ykCmdEvent.availability = false;

        data.push_back(0xFFFF);
        if (!sendWriteYkSelect(frameYkSBOSelect, addr, data)) {
            m_ykStatus                = ykidle;
            t_Yk.EndTimer();
            m_ykCmdEvent.availability = true;
            m_ykCmdEvent.state        = eventStopActCon;
            m_ykCmdEvent.value        = successed;
        }

        return true;
    }
 
    if (addr < m_baseDataConfig.doubleYknum && ((d_doubleBackup == NULL ) ||m_setPoll))
    {
        if (dco & 0x80)      //遥控选择
        {
            if (m_ykStatus == ykidle)
            {  
                t_Yk.StartTimer(20);
                
                m_ykCmdEvent              = event;
                m_ykCmdEvent.availability = false;
                if ((dco & 0x03) == 0x01)
                {
                    modbusPrint(LOG_ERROR, "地址:%d 选择分\n", addr);
                    data.push_back(0x0000);
                }
                else if ((dco & 0x03) == 0x02)
                {
                    modbusPrint(LOG_ERROR, "地址:%d 选择合\n", addr);
                    data.push_back(0xFF00);
                }
                if (!sendWriteYkSelect(frameYkSBOSelect, addr, data))
                {
                    m_ykStatus                  = ykselect;
                    m_ykCmdEvent.availability   = true;
                    m_ykCmdEvent.state          = eventActCon;
                    m_ykCmdEvent.value          = successed;
                }

                return true;
            }
        }
        else
        {
            if (m_ykStatus == ykselect)
            {
                if ((addr == m_ykCmdEvent.addr) && ((dco&0x03) == (m_ykCmdEvent.data.data[0]&0x03)))
                {
                    m_ykCmdEvent              = event;
                    m_ykCmdEvent.availability = false;
                    if ((dco & 0x03) == 0x01)
                    {
                        modbusPrint(LOG_ERROR, "地址:%d 控分\n", addr);
                        data.push_back(0x0000);
                    }
                    else if ((dco & 0x03) == 0x02)
                    {
                        modbusPrint(LOG_ERROR, "地址:%d 控合\n", addr);
                        data.push_back(0xFF00);
                    }
                    sendWriteYk(frameYk, addr, data);
                    t_Yk.StartTimer(m_config.ykTimeOut);
                    m_ykStatus = ykexecute;
                    
                    return true;
                }
                else
                {
                    t_Yk.EndTimer();
                    m_ykStatus = ykidle;
                    modbusPrint(LOG_ERROR, 
                        "遥控执行与选择不一致, 地址:sel=%d exe=%d 类型:sel=%d exe=%d\n",
                            m_ykCmdEvent.addr, addr, m_ykCmdEvent.data.data[0]&0x03, dco&0x03);
                }
            }
        }
    }
    m_ykCmdEvent                = event;
    m_ykCmdEvent.availability   = true;
    m_ykCmdEvent.state          = eventActCon;
    m_ykCmdEvent.value          = failed;
    return false;
}

/*******************************************************************************
@ Function Name     : setYcParam
@ Description       : 设置遥测参数
@ Input             : qpm   参数类型
                      addr  目标参数地址(遥测回路编号)
                      value 参数值
@ Output            : None;
@ Return            : true or false
*******************************************************************************/
bool ModBusProtocol::setYcParam(const SpontEvent &event)
{
    m_setParamEventList.pushBack(event);
    modbusPrint(LOG_DEBUG,"insert Param frame, size=%d, setFlag=%d", (int)(m_setParamEventList.size()), 
        m_isParamSetting?1:0);

    return true;
}

/*******************************************************************************
@ Function Name     : setParam
@ Description       : 设置遥测参数
@ Input             : event: 事件
@ Output            : None;
@ Return            : 返回事件
*******************************************************************************/
SpontEvent ModBusProtocol::setParam(const SpontEvent &event)
{
    SpontEvent revt = event;
    if (event.state == eventReq)                                     			//读取参数
    {
        if (m_config.setYcTHV && event.addr < m_data.Yc.size())             	//读取门限值
        {
            getParamConfig(event);
        }
        else
        {
		    getYt(event);
        }
    }
    else if (event.state == eventAct)                                			//设置参数
    {
        if (event.type == eventTypeDBSetParam)
        {
            modbusPrint(LOG_INFO, "setDBParam");
            setDBParam(event);
        }
        else
        {
            if (m_config.setYcTHV && event.addr < m_data.Yc.size())         	//设置门限值
            {
                setParamConfig(event);
            }
            else
            {
                setYt(event);
            }
        }
    }
    revt = m_setParamEvent;
    revt.availability = true;

    return revt;
}

/*******************************************************************************
@ Function Name     : setBroadcastMultiYk
@ Description       : 设置多路遥控只支持8路以下
@ Input             : nodeID
                      addr   寄存器地址
                      points 遥控点数
                      action 参数值
@ Output            : None;
@ Return            : true or false
*******************************************************************************/
bool ModBusProtocol::setBroadcastMultiYk(uint8_t nodeId, uint16_t addr, 
                                         uint8_t points, uint8_t action)
{
    MBusReqFifo  reqcmd;
    SerialNetBuf frame;
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = nodeId;
    frame.Buf[frame.BufLen ++] = FUNC_YK_MULT;
    frame.Buf[frame.BufLen ++] = (addr >> 8) & 0xFF;
    frame.Buf[frame.BufLen ++] = addr & 0xFF;
    frame.Buf[frame.BufLen ++] = 0;
    frame.Buf[frame.BufLen ++] = points;
    frame.Buf[frame.BufLen ++] = 1;
    frame.Buf[frame.BufLen ++] = action;
    
    reqcmd.reqType        = rspNoReq;
    reqcmd.reqFrame       = frame;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_ERROR, "遥控: ID:%d 功能码:%d ", nodeId, FUNC_YK_MULT);

    return true;
}

/*******************************************************************************
@ Function Name     : processYxEvent
@ Description       : 解析遥信接收帧
@ Input             : frame  CAN接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processYxEvent(const MBusRequest &request, const SerialNetBuf &frame)
{
    uint8_t THV   = 0;
    uint8_t yxSt  = 0;
    vector<uint16_t>     rdYx;
    vector<DataINT8U> *yxdata = NULL;
    //STLDeque<DataSoe> *yxsoefifo = NULL;
    char   info[32] = "";
    uint16_t addr    = 0;
    //printfs(LOG_DEBUG, "进入遥信数据解析");
	
    if (request.type == frameDoubleYx)
    {
        yxdata     = &m_data.doubleYx;
        //yxsoefifo  = &m_doubleYxSoeFifo;
        sprintf(info, "双点");
    }
    else if (request.type == frameSingleYx)
    {
        yxdata     = &m_data.singleYx;
        //yxsoefifo  = &m_singleYxSoeFifo;
        sprintf(info, "单点");
    }
	
    for (uint16_t i=0; i<frame.Buf[2]; )                                           //组合解析数据源
    {
        if (request.funcode == FUNC_RD_COIL || request.funcode == FUNC_RD_INPUT)
        {
            rdYx.push_back(frame.Buf[3 + i]);
            
            i ++;
        }
        else
        {
            rdYx.push_back((frame.Buf[3 + i] << 8) | frame.Buf[4 + i]);
            i += 2;
        }
    }
        
    for (uint16_t i=0; i<request.dataNum; i++)                               
    {
        addr = request.dataOffset + i;
        
        if (request.type == frameSingleYx)
        {
            yxSt = (rdYx[m_dataParse.singleYx[addr].regIndex] >> 
                        m_dataParse.singleYx[addr].bitIndex) & 0x01;
            THV  = m_dataParse.singleYx[addr].THV;
        }
        else if (request.type == frameDoubleYx)
        {
            yxSt = (((rdYx[m_dataParse.doubleYx[addr].onRegIndex] 
                     >> m_dataParse.doubleYx[addr].onBitIndex) & 0x01) << 1) | 
                   ((rdYx[m_dataParse.doubleYx[addr].offRegIndex]
                     >> m_dataParse.doubleYx[addr].offBitIndex) & 0x01);
            THV  = m_dataParse.doubleYx[addr].THV;
        }

        if (mAbs(yxSt, yxdata->at(addr).value) > THV)
        {
            if((!m_firstPoll) && ((d_doubleBackup == NULL) || m_setPoll))
            {
                if(request.type == frameDoubleYx) {
                    putSoeToFifo(addr, yxSt, m_doubleYxSoeFifo);                //二级SOE
                }
                else{
                    putSoeToFifo(addr, yxSt, m_singleYxSoeFifo);
                }
            }
            yxdata->at(addr).change = true;                                 //一级变位
        }
        yxdata->at(addr).value = yxSt;
        if (yxdata->at(addr).change) {
            modbusPrint(LOG_WARNING, "%s遥信变位: 地址=%d 状态=%d\n", 
                            info, addr, yxSt);
        }
    }
}
/*******************************************************************************
@ Function Name     : processYxEvent
@ Description       : 解析遥信接收帧
@ Input             : frame  CAN接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processMixYxEvent(const MBusRequest &request, const SerialNetBuf &frame)
{
    uint8_t THV   = 0;
    uint8_t yxSt  = 0;
    vector<uint16_t>     rdYx;
    vector<DataINT8U> *yxdata = NULL;
    char   info[32] = "";
    uint16_t addr    = 0;
    if (request.type == frameDoubleYx)
    {
        yxdata     = &m_data.doubleYx;
        sprintf(info, "双点");
    }
    else if (request.type == frameSingleYx)
    {
        yxdata     = &m_data.singleYx;
        sprintf(info, "单点");
    }
    for (uint16_t i=0; i<frame.Buf[2]; )                                           //组合解析数据源
    {
        if (request.funcode == FUNC_RD_COIL || request.funcode == FUNC_RD_INPUT)
        {
            rdYx.push_back(frame.Buf[3 + i]);
            i ++;
        }
        else
        {
            rdYx.push_back((frame.Buf[3 + i] << 8) | frame.Buf[4 + i]);
            i += 2;
        }
    }
    for (uint16_t i=0; i<request.dataNum; i++)                               
    {
        addr = request.dataOffset + i;
        if (request.type == frameSingleYx)
        {
            yxSt = (rdYx[m_dataParse.singleYx[addr].regIndex] >> 
                        m_dataParse.singleYx[addr].bitIndex) & 0x01;
            THV  = m_dataParse.singleYx[addr].THV;
        }
        else if (request.type == frameDoubleYx)
        {
            yxSt = (((rdYx[m_dataParse.doubleYx[addr].onRegIndex] 
                     >> m_dataParse.doubleYx[addr].onBitIndex) & 0x01) << 1) | 
                   ((rdYx[m_dataParse.doubleYx[addr].offRegIndex]
                     >> m_dataParse.doubleYx[addr].offBitIndex) & 0x01);
            THV  = m_dataParse.doubleYx[addr].THV;
        }
        if (mAbs(yxSt, yxdata->at(addr).value) > THV)
        {
    		if(d_doubleBackup == NULL){
	            yxdata->at(addr).change = true;                                     //一级变位
            }
        }
        yxdata->at(addr).value = yxSt;
        if (yxdata->at(addr).change)
        {
            modbusPrint(LOG_WARNING, "%s遥信变位: 地址=%d 状态=%d\n", 
                            info, addr, yxSt);
        }
    }
}

/*******************************************************************************
@ Function Name     : processYcEvent
@ Description       : 解析CAN遥测数据帧
@ Input             : frame: CAN接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processYcEvent(const MBusRequest &request, const SerialNetBuf &frame)
{
    vector<int16_t> rdYc;
    uint16_t addr     = 0;
    int16_t ycValue  = 0;
    const MBusParseData *data = NULL;

    for (uint8_t i=0; i<frame.Buf[2]; i+=2)                                       //组合解析数据源
    {
        rdYc.push_back((frame.Buf[3+i] << 8) | frame.Buf[4+i]);
    }
    for (uint8_t i=0; i<request.dataNum; i++)
    {
        addr    = request.dataOffset + i;
        data    = &m_dataParse.yc[addr];
        ycValue = rdYc[data->regIndex] + data->compensate;
        
        if (mAbs(ycValue, m_data.Yc[addr].value) > data->THV)
        {
			if(!m_firstPoll && m_setPoll){
				putSoeToFifo(addr, ycValue, m_validYcSoeFifo);                      //SOE
	            m_data.Yc[addr].change = true;                                      //无时标
                modbusPrint(LOG_WARNING, "遥测变位: 地址=%d 值=%d 历史值:%d THV=%d", 
                    addr, ycValue, m_data.Yc[addr].value, data->THV);
			}
            m_data.Yc[addr].value      = ycValue;
        }
    }
}
/*******************************************************************************
@ Function Name     : processYmEvent
@ Description       : 解析遥脉数据帧
@ Input             : frame: CAN接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processYmEvent(const MBusRequest &request, const SerialNetBuf &frame)
{
    vector<int32_t> rdYm;
    uint16_t addr     = 0;
    int32_t ymValue  = 0;
    const MBusParseData *data = NULL;

    for (uint8_t i=0; i<frame.Buf[2]; i+=4)                                       //组合解析数据源
    {
        rdYm.push_back((frame.Buf[3+i] << 24) | frame.Buf[4+i] << 16 | 
                       frame.Buf[5+i] << 8    | frame.Buf[6+i]);
    }
    for (uint8_t i=0; i<request.dataNum; i++)
    {
        addr    = request.dataOffset + i;
        data    = &m_dataParse.ym[addr];

        ymValue = rdYm[data->regIndex];

	 if (mAbs(ymValue, m_data.Ym[addr].value) > data->THV)
	{
		m_data.Ym[addr].change = true;
	}
	 m_data.Ym[addr].value      = ymValue;
    }

}

/*******************************************************************************
@ Function Name     : processB32YcEvent
@ Description       : 解析32位遥测数据帧
@ Input             : frame: CAN接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processB32YcEvent(const MBusRequest &request, const SerialNetBuf &frame)
{
    vector<B32SType> rdYc;
    uint16_t addr     = 0;
    float   ycValue  = 0.0;

    for (uint8_t i=0; i<frame.Buf[2]; i+=4)                                       //组合解析数据源
    {
        B32SType value;

        value.bytes[0] = frame.Buf[6+i];
        value.bytes[1] = frame.Buf[5+i];
        value.bytes[2] = frame.Buf[4+i];
        value.bytes[3] = frame.Buf[3+i];
        
        rdYc.push_back(value);
    }
    for (uint8_t i=0; i<request.dataNum; i++)
    {
        addr    = request.dataOffset + i;
        MBusParseData &data = m_dataParse.floatYc[addr];

        if (request.type == frameInt32Yc)
        {
            ycValue = (float)(rdYc[data.regIndex].intvalue) + data.compensate;
        }
        else
        if (request.type == frameFloatYc)
        {
            ycValue = rdYc[data.regIndex].fpvalue + data.compensate;
        }
		
		ycValue = ycValue*data.factor;

        if (fabsf(ycValue - m_data.floatYc[addr].value) > (float)(data.THV))
        {   
            if (!m_firstPoll) {
                DataFP32Soe soe;
                DateService date;
                DateType    time;

                soe.addr  = addr;
                soe.value = ycValue;
                date.GetCurrentDate(&time);
                soe.dateTime[6] = time.m_year - 100;
                soe.dateTime[5] = time.m_mon;
                soe.dateTime[4] = time.m_mday | 0x20;
                soe.dateTime[3] = time.m_hour;
                soe.dateTime[2] = time.m_min;
                soe.dateTime[1] = (time.m_sec * 1000 + time.m_msec) >> 8;
                soe.dateTime[0] = (time.m_sec * 1000 + time.m_msec);

                m_FP32YcSoeFifo.pushBack(soe);                                      //二级SOE
            }
            m_data.floatYc[addr].change = true;                                 //无时标           
        }
        m_data.floatYc[addr].value      = ycValue;
    }
}

/*******************************************************************************
@ Function Name     : processB16YcEvent
@ Description       : 解析16位遥测数据帧
@ Input             : frame: CAN接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processB16YcEvent(const MBusRequest &request, const SerialNetBuf &frame)
{
    vector<B32SType> rdYc;
    uint16_t addr     = 0;
    float   ycValue  = 0.0;

    for (uint8_t i=0; i<frame.Buf[2]; i+=2)                                       //组合解析数据源
    {
        B32SType value;

        value.bytes[0] = frame.Buf[4+i];
        value.bytes[1] = frame.Buf[3+i];
        value.bytes[2] = 0;
        value.bytes[3] = 0;
        rdYc.push_back(value);
    }
	
    for (uint8_t i=0; i<request.dataNum; i++)
    {
        addr    = request.dataOffset + i;
        MBusParseData &data = m_dataParse.floatYc[addr];

        if (request.type == frameInt16Yc)
        {
            ycValue = (float)(rdYc[data.regIndex].intvalue) + data.compensate;
        }
		ycValue = ycValue*data.factor;

        if (fabsf(ycValue - m_data.floatYc[addr].value) > (float)(data.THV))
        {   
            if (!m_firstPoll) {
                DataFP32Soe soe;
                DateService date;
                DateType    time;

                soe.addr  = addr;
                soe.value = ycValue;
                date.GetCurrentDate(&time);
                soe.dateTime[6] = time.m_year - 100;
                soe.dateTime[5] = time.m_mon;
                soe.dateTime[4] = time.m_mday | 0x20;
                soe.dateTime[3] = time.m_hour;
                soe.dateTime[2] = time.m_min;
                soe.dateTime[1] = (time.m_sec * 1000 + time.m_msec) >> 8;
                soe.dateTime[0] = (time.m_sec * 1000 + time.m_msec);

                m_FP32YcSoeFifo.pushBack(soe);                                      //二级SOE
            }
            m_data.floatYc[addr].change = true;                                 //无时标           
        }
       
        m_data.floatYc[addr].value      = ycValue;
    }
}
/*******************************************************************************
@ Function Name     : processB8YcEvent
@ Description       : 解析8位遥测数据帧
@ Input             : frame: CAN接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processB8YcEvent(const MBusRequest &request, const SerialNetBuf &frame)
{
    vector<B32SType> rdYc;
    uint16_t addr     = 0;
    float   ycValue  = 0.0;

    for (uint8_t i=0; i<frame.Buf[2]; i+=1)                                       //组合解析数据源
    {
        B32SType value;

        value.bytes[0] = frame.Buf[3+i];
        value.bytes[1] = 0;
        value.bytes[2] = 0;
        value.bytes[3] = 0;
        
        rdYc.push_back(value);
    }
    for (uint8_t i=0; i<request.dataNum; i++)
    {
        addr    = request.dataOffset + i;
        MBusParseData &data = m_dataParse.floatYc[addr];

        if (request.type == frameInt8Yc)
        {
            ycValue = (float)(rdYc[data.regIndex].intvalue) + data.compensate;
        }
		ycValue = ycValue*data.factor;
		
        if (fabsf(ycValue - m_data.floatYc[addr].value) > (float)(data.THV))
        {   
            if (!m_firstPoll) {
                DataFP32Soe soe;
                DateService date;
                DateType    time;

                soe.addr  = addr;
                soe.value = ycValue;
                date.GetCurrentDate(&time);
                soe.dateTime[6] = time.m_year - 100;
                soe.dateTime[5] = time.m_mon;
                soe.dateTime[4] = time.m_mday | 0x20;
                soe.dateTime[3] = time.m_hour;
                soe.dateTime[2] = time.m_min;
                soe.dateTime[1] = (time.m_sec * 1000 + time.m_msec) >> 8;
                soe.dateTime[0] = (time.m_sec * 1000 + time.m_msec);

                m_FP32YcSoeFifo.pushBack(soe);                                      //二级SOE
            }
            m_data.floatYc[addr].change = true;                                 //无时标           
        }
       
        m_data.floatYc[addr].value      = ycValue;
    }
}

/*******************************************************************************
@ Function Name     : processYtEvent
@ Description       : 解析参数数据帧
@ Input             : frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processYtEvent(const MBusRequest &request, const SerialNetBuf &frame)
{
    vector<int16_t> rdYc;
    uint16_t addr     = 0;
    int16_t ycValue  = 0;
    const MBusParseData *data = NULL;
	uint8_t deviceId = frame.Buf[0];
	int16_t _version = 0; 

    for (uint8_t i=0; i<frame.Buf[2]; i+=2)                                       //组合解析数据源
    {
        rdYc.push_back((frame.Buf[3+i] << 8) | frame.Buf[4+i]);
    }
    if (rdYc.size() == 0)
        return ;
    for (uint8_t i=0; i<request.dataNum; i++)
    {
        addr    = request.dataOffset + i;
        data    = &m_dataParse.yt[addr];
        ycValue = rdYc[data->regIndex] ;
        
        m_data.yt[addr].value      = ycValue;
		if(data->dataType == frameParseDataVersion){
			_version = ycValue;
			for(uint8_t versionIndex =0; versionIndex < m_version.size(); versionIndex++){
				if(m_version[versionIndex].deviceId == deviceId){
					m_version[versionIndex].nodeVersion = (uint16_t)_version;
					break;
				}
			}
		}
    }
}
/*******************************************************************************
@ Function Name     : 故障报告解析帧
@ Description       : 解析故障报告数据帧
@ Input             : request,frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processframeYcErrReportEvent(const MBusRequest &request,const SerialNetBuf &frame)
{
	#if 1
    DateService     date;
    DateType        time;
    uint8_t           len = 0;
	//uint16_t			addr = 0;
	uint8_t			regIndex = 0;												//接受帧的数据帧开头
	SerialNetBuf	tempFrame;
    SpontEvent      ycErrReport;
	uint8_t           compareArry[SERIAL_BUFF_LEN];
	uint8_t           cirOffset = 0;
    uint16_t          dataOffset = 0;
 
    ycErrReport.srcModuleName    = "";
    ycErrReport.state            = eventSpont;
    ycErrReport.value            = successed;
	tempFrame = frame;
//	addr = request.dataOffset ;												//对应解析帧的位置
	memset(&compareArry, 0, sizeof(compareArry));

	if(!memcmp(compareArry, &frame.Buf[5], 24))								//数据全为0不解析
	{
		return;
	}
	//回路偏移计算
	for(uint8_t i = 0; i < m_dataParse.errReport.size(); i++){
		if(frame.Buf[0] == m_dataParse.errReport[i].deviceId){
			cirOffset =  m_dataParse.errReport[i].cirIndex;
			break;
		}
	}
	ycErrReport.addr = m_baseCir + cirOffset;
	 
	for(uint8_t errReportCouner = 0 ; errReportCouner < frame.Buf[2]/24; errReportCouner ++)
	{
		len	=	0;
		regIndex = 5 + 26*errReportCouner;
		if (parseMode)  													    //光芒模式或者南凯模式
	    {
	        ycErrReport.data.data[len ++] = (ycErrReport.addr + 1);   				//装置地址低(回路号)
	        ycErrReport.data.data[len ++] = (ycErrReport.addr + 1) >> 8;
			regIndex +=2;
	    }
	    else                                                
	    {
	        ycErrReport.data.data[len ++] = 0;                              	 //后续标志
	        regIndex +=2;
	    }
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];                  	 //信息序号低(故障类型)
	    
	    if (parseMode)
	    {
	        ycErrReport.data.data[len ++] = frame.Buf[regIndex++];              	//信息序号高(故障类型)
	    }
		else
		{
			regIndex +=2;
		}
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];                		//双点信息(故障状态)
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];                       //相对时间低
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];                       //        高
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];                       //故障序号低
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];                       //        高
	    date.GetCurrentDate(&time);
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];                       //时标 C32Time2a
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];
	    ycErrReport.data.data[len ++] = frame.Buf[regIndex++];
	    if (parseMode)                                                              //C56Time2a
	    {
	        ycErrReport.data.data[len ++] = frame.Buf[regIndex++];
	        ycErrReport.data.data[len ++] = frame.Buf[regIndex++];
	        ycErrReport.data.data[len ++] = frame.Buf[regIndex++];
	    }
        else
        {
            regIndex +=3;
        }

        dataOffset = frame.Buf[5 + 26*errReportCouner + 17] | (frame.Buf[5 + 26*errReportCouner + 18] << 8);
        if (m_HighVolDevice.find(frame.Buf[0]) != m_HighVolDevice.end() && dataOffset >= 3) {
            dataOffset += 3;
        }
		ycErrReport.data.data[len ++] = 1;                        					  //故障电量个数
	    ycErrReport.data.data[len ++] = dataOffset & 0xFF;                            //故障电量索引号低
	    ycErrReport.data.data[len ++] = (dataOffset >> 8) & 0xFF;                       //故障电量索引号高

        ycErrReport.data.data[len ++] = 7;                       					  //故障电量类型
        float faultRMS = 0.0;
        memcpy(&faultRMS, &frame.Buf[5 + 26*errReportCouner + 20], 4);
        memcpy(&ycErrReport.data.data[len], &faultRMS, 4);
        len += 4;
	    ycErrReport.data.len          = len;
        ycErrReport.type              = ycErrReportEvent;
        ycErrReport.availability      = true;
	    m_ycErrReportEvent.pushBack(ycErrReport);
		regIndex = 5 + 26 * errReportCouner;												//用于打印

		if ((tempFrame.Buf[regIndex+4] & 0x3) == 0x02)
    	{
    		if(tempFrame.Buf[regIndex+17] > 2)											//故障电量索引号处理
			{
				tempFrame.Buf[regIndex+17] -=3;
			}
            modbusPrint(LOG_WARNING, "设备ID:%d 回路:%d %c相%s 故障产生 >> " \
                      "序号:0x%04x 电量:%f 时标:%02d.%03d",\
            frame.Buf[0],
            (tempFrame.Buf[regIndex]-1+m_baseCir + cirOffset),(tempFrame.Buf[regIndex+17]+'A'),		//回路号配合底板进行减1操作	
            m_waveCause[tempFrame.Buf[regIndex+2]].c_str(),\
            (ycErrReport.data.data[7] | (ycErrReport.data.data[8] << 8)),\
            faultRMS,\
            (tempFrame.Buf[regIndex+9] | (tempFrame.Buf[regIndex+10] << 8)) / 1000,\
            (tempFrame.Buf[regIndex+9] | (tempFrame.Buf[regIndex+10] << 8)) % 1000);    
    	}
        else
    	{
            if(tempFrame.Buf[regIndex+17] > 2)
			{
				tempFrame.Buf[regIndex+17] -=3;
			}
            modbusPrint(LOG_WARNING, "设备ID:%d 回路:%d %c相%s 故障恢复 >> " \
                      "序号:0x%04x 电量:%f 时标:%02d.%03d",\
            frame.Buf[0],
            (tempFrame.Buf[regIndex]-1+m_baseCir + cirOffset),(tempFrame.Buf[regIndex+17]+'A'),			//回路号配合底板进行减1操作	
            m_waveCause[tempFrame.Buf[regIndex+2]].c_str(),\
            (ycErrReport.data.data[7] | (ycErrReport.data.data[8] << 8)),\
            faultRMS,\
            (tempFrame.Buf[regIndex+9] | (tempFrame.Buf[regIndex+10] << 8)) / 1000,\
            (tempFrame.Buf[regIndex+9] | (tempFrame.Buf[regIndex+10] << 8)) % 1000);     
    	}
	}
	#endif
}
/*******************************************************************************
@ Function Name     : 录波目录解析
@ Description       : 目录召取解析帧
@ Input             : request,frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processframeYcWaveDir(const MBusRequest &request,const SerialNetBuf &frame)
{
    if(d_doubleBackup != NULL)
    {
        return ;
    }
	if(frame.Buf[2] == 0)
	{
		return;
	}
//	uint16_t cirNum 			= (frame.Buf[6] << 8) | (frame.Buf[5]);
	uint16_t errReportNum 	= (frame.Buf[8] << 8) | (frame.Buf[7]);
	fileLength 				= (frame.Buf[11]  <<16) | (frame.Buf[10] << 8)|(frame.Buf[9]);

	if(!(fileLength%request.regNum))						//考虑是否被整除
	{
		waveFrameNum = fileLength/request.regNum;
	}
	else
	{
		waveFrameNum = fileLength/request.regNum +1;
	}	
	if(!fileLength)
	{
		return;
	}
	m_ycWaveNum.at(request.deviceAddr) ++;
	modbusPrint(LOG_INFO, "总线设备%d 目录文件存在 故障序号:0x%04x 文件长度:%5d",
	    request.deviceAddr, errReportNum, fileLength);

}

/*******************************************************************************
@ Function Name     : 混合帧解析
@ Description       : 混合帧解析
@ Input             : request,frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processframeMix(const MBusRequest &request,const SerialNetBuf &frame)
{
	SerialNetBuf derivesFrame;
	memset(&derivesFrame, 0, sizeof(SerialNetBuf));
	derivesFrame.Buf[derivesFrame.BufLen++] = request.deviceAddr;
	//应答帧的数据起始地址
	uint8_t frameBaseAddr = 3;
	uint8_t frameOffset = 0;

	for(uint8_t subRequestIndex = 0 ;subRequestIndex < request.subMBusRequest.size(); subRequestIndex++){
		MBusRequest subRequest = request.subMBusRequest[subRequestIndex];
		
		derivesFrame.BufLen = 1;
		frameOffset = frame.Buf[frameBaseAddr];
		derivesFrame.Buf[derivesFrame.BufLen++] = subRequest.funcode;
		frameOffset += 1;
		memcpy(&derivesFrame.Buf[derivesFrame.BufLen], &frame.Buf[frameBaseAddr], frameOffset);
		derivesFrame.BufLen += frameOffset;
		frameBaseAddr += frameOffset;

		modbusPrintBuf(LOG_DEBUG, "derivesFrame数据集Data:\t", derivesFrame.Buf,derivesFrame.BufLen);
		switch(subRequest.type){
			case frameSingleYx:
			case frameDoubleYx:{
				processMixYxEvent(subRequest, derivesFrame);
				break;
			}
			case frameYc:{
				processYcEvent(subRequest, derivesFrame);
				break;
			}
			case frameYm:{
				processYmEvent(subRequest, derivesFrame);
				break;
			}       	
			case frameYt:{
				processYtEvent(subRequest, derivesFrame);
				break;
			}
			case frameReadErrReport:{
				processframeYcErrReportEvent(subRequest, derivesFrame);
				break;
			}
			case frameReadDir:{
				processframeYcWaveDir(subRequest, derivesFrame);
				break;
			}
			case frameYxSoeNum:{
				processframeYxSoeNum(derivesFrame);
				break;
			}
			default:break;	
		}
	}
}

/*******************************************************************************
@ Function Name     : 遥信事件记录数目处理
@ Description       : 解析遥信事件记录
@ Input             : request,frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processframeYxSoeNum(const SerialNetBuf &frame)
{
	printfs(LOG_DEBUG, "进入遥信soe数目解析");

	uint16_t singleSoeNum = frame.Buf[9]<< 8 | frame.Buf[10];
	uint16_t doubleSoeNum = frame.Buf[7]<< 8 | frame.Buf[8];

	if (d_doubleBackup != NULL)
        return ;
	m_yxSoeCheck.soeAckNum.singleYx = singleSoeNum;
	m_yxSoeCheck.soeAckNum.doubleYx = doubleSoeNum;
	
	if(singleSoeNum + doubleSoeNum > 0){
		modbusPrint(LOG_INFO, "设备ID:%d,存在单点遥信SOE个数:%d,双点遥信SOE个数:%d\n",frame.Buf[0],singleSoeNum,doubleSoeNum);	
		sendReadSoeFile(singleSoeNum + doubleSoeNum,frame.Buf[0]);
	}
}

/*******************************************************************************
@ Function Name     : 遥信事件记录处理
@ Description       : 解析遥信事件记录
@ Input             : request,frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processframeYxSoe(const MBusReadFileRequest &request,const SerialNetBuf &frame)
{
	//STLDeque<DataSoe> *yxsoefifo  = NULL;
	uint16_t addr    = 0;
    uint8_t yxSt  = 0;
	//char   info[5] = "";
	DateType time;
	
	uint8_t dataLength = frame.Buf[2];
	uint8_t dataUnitLength = frame.Buf[3] + 1;
	uint8_t soeNum = dataLength/dataUnitLength;
	uint16_t quoteType = 0;
	uint16_t refType = 0;

	//返回无数据
	if((frame.Buf[4]&0x80) == 0x80){
		modbusPrint(LOG_INFO, "设备ID:%d返回无soe",frame.Buf[0]);
		return;
	}

	for(uint8_t dataIndex= 0; dataIndex < soeNum;dataIndex++){
		uint16_t baseAddr = dataUnitLength*dataIndex;
		modbusPrint(LOG_DEBUG, "设备ID:%d,解析偏移基址:%d,解析数目:%d,数据长度:%d,单个数据长度:%d\n",frame.Buf[0],baseAddr, dataIndex, dataLength, dataUnitLength);
		
		
		refType = (frame.Buf[5 + baseAddr] << 8 )|(frame.Buf[6 + baseAddr]) ;
		quoteType = frame.Buf[4 + baseAddr];
		if(quoteType&0x80){
			modbusPrint(LOG_INFO, "yxSoe引用类型高位置1,不存在该条soe");
			continue;
		}
		
		addr = ((frame.Buf[7 + baseAddr] << 8) | frame.Buf[8 + baseAddr]);
		if (refType == MD_DOUBLEYX_SOE)
	    {
	        //yxsoefifo  = &m_doubleYxSoeFifo;
			m_yxSoeCheck.soeAckNum.doubleYx -- ;
	        //sprintf(info, "双点");
			addr += request.soeBaseAddr.doubleYxBaseAddr;
	    }
	    else if (refType == MD_SINGLEYX_SOE)
	    {
	        //yxsoefifo  = &m_singleYxSoeFifo;
			m_yxSoeCheck.soeAckNum.singleYx -- ;
	        //sprintf(info, "单点");
			addr += request.soeBaseAddr.singleYxBaseAddr;
	    }
		
		yxSt = ((frame.Buf[9 + baseAddr] << 8) | frame.Buf[10 + baseAddr]);
		time.m_year = ((frame.Buf[11 + baseAddr] << 8) | frame.Buf[12 + baseAddr]);
		time.m_mon	= frame.Buf[13 + baseAddr];
		time.m_mday = frame.Buf[14 + baseAddr];
		time.m_hour = frame.Buf[15 + baseAddr];
		time.m_min  = frame.Buf[16 + baseAddr];
		time.m_sec  = ((frame.Buf[17 + baseAddr] << 8) | frame.Buf[18 + baseAddr])/1000;
		time.m_msec = ((frame.Buf[17 + baseAddr] << 8) | frame.Buf[18 + baseAddr])%1000;

		putSoeToFifoWthTime(addr, yxSt, (refType == MD_DOUBLEYX_SOE) ? m_doubleYxSoeFifo : m_singleYxSoeFifo, time);
		if(m_dYxSoeFifo.size() != 0){
			m_dYxSoeFifo.delFront();
		}
		//modbusPrint(LOG_WARNING, "%s遥信soe: 地址=%d 状态=%d 时间:%d.%02d.%02d %02d:%02d:%02d.%03d\n",frame.Buf[0], info, addr, yxSt, time.m_year,
		//time.m_mon,time.m_mday,time.m_hour, time.m_min, time.m_sec,  time.m_msec);
	}
	m_yxSoeCheck.isSoeNumCheck = true;
	//一帧超过15个时，进行多帧解析
	if(soeNum >= 15){
		m_yxSoeCheck.isSoeCheck = true;
	}
}

/*******************************************************************************
@ Function Name     : processframeYcWaveCall
@ Description       : 录波文件接收帧处理
@ Input             : request,frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processframeYcWaveCall(const SerialNetBuf &frame, char *devIpv4)
{
	SerialNetBuf WaveData;
	uint8_t tempFrame[SERIAL_BUFF_LEN];

	memset(&tempFrame, 0, sizeof(tempFrame));


	memcpy(tempFrame, frame.Buf, frame.BufLen);
	memcpy(WaveData.Buf, &frame.Buf[WAVEFRAMEHEAD], frame.BufLen - WAVEFRAMEHEAD - WAVEFRAMECRC);
	WaveData.BufLen  = frame.BufLen - WAVEFRAMEHEAD - WAVEFRAMECRC;

	if(WaveData.BufLen == 0)
	{
		modbusPrint(LOG_WARNING,"%-16s:录波文件数据帧长度:%-5d", devIpv4?devIpv4:" ", WaveData.BufLen);
		dataZero = true;
		return;
	}
	rcvWaveData.push_back(WaveData);
	modbusPrint(LOG_INFO,"%-16s:录波文件数据帧数目:%-5d 录波文件数据帧长度:%d 第%d帧",
        devIpv4?devIpv4:" ",waveFrameNum, WaveData.BufLen, rcvWaveData.size());
	if(rcvWaveData.size() != waveFrameNum)						//录波文件数据帧是否存储完整
	{
		return;
	}
	else{
		m_waveDeleteId = 	frame.Buf[0];
		saveYcWaveFile(devIpv4, frame.Buf[0]);
	}
}
/*******************************************************************************
@ Function Name     : waveFileConfirm
@ Description       : 确认帧插入
@ Input             : request,frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::waveFileConfirm(uint16_t reqptr)
{
	MBusReqFifo reqcmd;
    SerialNetBuf frame;

	frame.BufLen = 0;
	frame.Buf[frame.BufLen ++] = 0x01;
	frame.Buf[frame.BufLen ++] = 0x14;							
	frame.Buf[frame.BufLen ++] = 0x00;
	frame.Buf[frame.BufLen ++] = 0x06;
	frame.Buf[frame.BufLen ++] = 0x00;
	frame.Buf[frame.BufLen ++] = 0x03;
	frame.Buf[frame.BufLen ++] = 0x00;
	frame.Buf[frame.BufLen ++] = 0xFF;
	frame.Buf[frame.BufLen ++] = 0x00;
	frame.Buf[frame.BufLen ++] = 0x00;
	
	reqcmd.reqType        = rspReadFile;
	reqcmd.reqPtr         = reqptr+1;
	reqcmd.reqFrameType   = frameReadWave;
	reqcmd.reqFrame       = frame;
	reqcmd.reqDelaySend   = 0;
	
	m_reqFifo.pushBack(reqcmd);
	return;
}
/*******************************************************************************
@ Function Name     : waveFileConfirm
@ Description       : 确认帧插入
@ Input             : frame: 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::dealWaveFileConfirm(const SerialNetBuf &frame)
{
	SerialNetBuf confirmFrame;
	confirmFrame.BufLen = 0;
	confirmFrame.Buf[confirmFrame.BufLen++]=frame.Buf[0];
	confirmFrame.Buf[confirmFrame.BufLen++]=frame.Buf[1];
	confirmFrame.Buf[confirmFrame.BufLen++]=0x08;
	confirmFrame.Buf[confirmFrame.BufLen++]=0x07;
	confirmFrame.Buf[confirmFrame.BufLen++]=0x06;
	confirmFrame.Buf[confirmFrame.BufLen++]=0x00;
	confirmFrame.Buf[confirmFrame.BufLen++]=0x03;
	confirmFrame.Buf[confirmFrame.BufLen++]=0x00;
	confirmFrame.Buf[confirmFrame.BufLen++]=0xff;
	confirmFrame.Buf[confirmFrame.BufLen++]=0x00;
	confirmFrame.Buf[confirmFrame.BufLen++]=0x00;
	if(!memcmp(frame.Buf,confirmFrame.Buf,confirmFrame.BufLen))
		{
			return;
		}
	modbusPrint(LOG_WARNING, "应答录波确认帧不正确" );
}

/*******************************************************************************
@ Function Name     : saveYcWaveFile
@ Description       : 存储遥测录波文件,保持录波文件的存在数目为20个
                      文件命名规则: 以32位数字转换为8个字节的16进制字符串作为名字
                                    文件名=回路号(b31~24)|故障序号(b23~0)
@ Input             : none
@ Output            : 录波文件
@ Return            : true   :存储文件成功
                      false  :存储文件失败
*******************************************************************************/
bool ModBusProtocol::saveYcWaveFile(char *devIpv4, uint8_t deviceAddr)
{
	uint8_t	*data         			= new uint8_t[fileLength];
	uint32_t  l_FileName              = 0;
	char    filePath[64]  	        = "";
	uint32_t  lengStart				= 0;
	int     writeFd 				= -1;
	int     ret						= 0;
	uint8_t   cirOffset				= 0;
    uint16_t  dstCirID                = 0;
	uint8_t	CP56Time[5];

	//回路偏移计算
	for(uint8_t i = 0; i < m_dataParse.errReport.size(); i++){
		if(deviceAddr == m_dataParse.errReport[i].deviceId){
			cirOffset =  m_dataParse.errReport[i].cirIndex;
			break;
		}
	}
	
	for (uint16_t rcvFrameNum = 0; rcvFrameNum < rcvWaveData.size(); rcvFrameNum ++)
	{
		memcpy(&data[lengStart],&rcvWaveData[rcvFrameNum].Buf,(rcvWaveData[rcvFrameNum].BufLen));		//减去两位CRC校验码，最后一帧非整数	
		lengStart += (rcvWaveData[rcvFrameNum].BufLen);
	}
	rcvWaveData.clear();
	
	l_FileName  = data[4 + TIMEFRAMEADD] + data[5 + TIMEFRAMEADD] * 256;
	if (devIpv4 == NULL) {
        dstCirID = m_baseCir + cirOffset + data[6 + TIMEFRAMEADD];
	}
	else {
		dstCirID = m_baseCir + data[6 + TIMEFRAMEADD];			                //不同设备的文件名基址
		
	}
    l_FileName |= dstCirID << 16;
	sprintf(filePath, "%08X", (unsigned int)l_FileName);

	//录波回路号偏移
	data[6 + TIMEFRAMEADD] = (dstCirID + 1) & 0xFF;
    data[7 + TIMEFRAMEADD] = ((dstCirID + 1) >> 8) & 0xFF;
	CP56Time[4] = data[4];
	CP56Time[3] = data[3];
	CP56Time[2] = data[2] | 0x20;
	CP56Time[1] = data[1];
	CP56Time[0] = data[0];

    float  CT = 1;
    float  PT = 1;
    uint8_t ixFactor[4];
    uint8_t uxFactor[4];                                                          //微型RTU使用结构体存储，出现了对齐需特殊处理

    memcpy(ixFactor, &data[fileLength - 8], 4);
    memcpy(uxFactor, &data[fileLength - 4], 4);
	modbusPrint(LOG_DEBUG, "录波产生时间: Year:%d,Month:%d,Day:%d,Hour:%d,Minute:%d,Msecond:%d,",
		data[4], data[3], data[2], data[1], data[0], (data[5]|data[6]<<8));
	modbusPrintBuf(LOG_DEBUG, "接收到的数据集Data:%x\t", data,fileLength);
	modbusPrint(LOG_DEBUG, "文件管理指针d_waveFileManage:%d", d_waveFileManage);
	#if 1
	if (d_waveFileManage && ((writeFd = d_waveFileManage->openWriteFile(string(filePath),ret,devIpv4)) != -1))
    {
        d_waveFileManage->writeFile(writeFd,(unsigned char *)&data[0+TIMEFRAMEADD], 2);
        d_waveFileManage->writeFile(writeFd,(unsigned char *)CP56Time, sizeof(CP56Time));
        d_waveFileManage->writeFile(writeFd,(unsigned char *)&data[2+TIMEFRAMEADD], fileLength - 7 - 11);
        d_waveFileManage->writeFile(writeFd,(unsigned char *)ixFactor, 4);
        d_waveFileManage->writeFile(writeFd,(unsigned char *)uxFactor, 4);
        d_waveFileManage->writeFile(writeFd,(unsigned char *)&CT, 4);
        d_waveFileManage->writeFile(writeFd,(unsigned char *)&PT, 4);
		
		if (d_waveFileManage->getWriteLength(writeFd) >= fileLength+5)
        {
            char deviceDesc[128] = "";

            sprintf(deviceDesc, "总线设备%d", deviceAddr);
            modbusPrint(LOG_WARNING, "%-16s:%08X 录波文件创建成功!\n", 
                        devIpv4 ? devIpv4 : deviceDesc, l_FileName);
        }
		d_waveFileManage->writeEnd(writeFd);
        
		if (d_webWaveFile && ((writeFd=d_webWaveFile->openWriteFile(string(filePath),ret,devIpv4))!= -1))									
		{
			d_webWaveFile->writeFile(writeFd,(unsigned char *)&data[0+TIMEFRAMEADD], 2);
			d_webWaveFile->writeFile(writeFd,(unsigned char *)CP56Time, sizeof(CP56Time));
			d_webWaveFile->writeFile(writeFd,(unsigned char *)&data[2+TIMEFRAMEADD], fileLength - 7 - 11);
            d_webWaveFile->writeFile(writeFd,(unsigned char *)ixFactor, 4);
            d_webWaveFile->writeFile(writeFd,(unsigned char *)uxFactor, 4);
            d_webWaveFile->writeFile(writeFd,(unsigned char *)&CT, 4);
            d_webWaveFile->writeFile(writeFd,(unsigned char *)&PT, 4);
			d_webWaveFile->writeEnd(writeFd);
		}
		delete[] data;																					//释放内存
		m_ycWaveNum.at(deviceAddr) = 0;
//		ycWaveNum = 0;
		waveWriteSuc = true;
		waveFrameNum = 0;		
		return true;
	}
	#endif
	
 	delete[] data;															
	waveFrameNum = 0;
    modbusPrint(LOG_WARNING, "%-16s:%08X 录波文件创建失败, 错误原因:%d!\n",devIpv4?devIpv4:" ", l_FileName, errno);
    
    return false;
}

/*******************************************************************************
@ Function Name     : setFileManage
@ Description       : 设置相应的文件管理
@ Input             : manage: 文件管理类指针
                      type:   文件类型
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::setFileManage(FileManage *manage, RtuFileType type)
{
    switch (type)
    {
        case RtuFileTypeWaveFile:
        {
            d_waveFileManage = manage;
            break;
        }
        case RtuFileTypeWebWaveFile:
        {
            d_webWaveFile = manage;
            break;
        }
        default :break;
    }
}

/*******************************************************************************
@ Function Name     : setCommStat
@ Description       : 设置对应设备序号的通信状态
@ Input             : deviceNo: 设备序号
                      stat:   false:离线 true:在线
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::setCommStat(uint16_t deviceNo, bool stat, char *ip)
{
    if (m_config.commStatReport) {
        uint8_t commStat = stat ? 0 : 1;
        map<int, int>::iterator it = m_deviceIdToCommstatAddr.find(deviceNo);
        if (it == m_deviceIdToCommstatAddr.end()) {
            return ;
        }
        uint16_t addr = it->second;
        uint8_t nor = commStat ^ (m_data.singleYx.at(addr).value);

        m_data.singleYx[addr].value = commStat;
        if (nor) {
            if (!m_firstPoll && m_setPoll) {
                putSoeToFifo(addr, commStat, m_singleYxSoeFifo);
            }
            m_data.singleYx[addr].change = true;
            modbusPrint(LOG_WARNING, "设备%s地址:%d 通信状态遥信:%d %s", ip?ip:"", 
                    m_version[deviceNo].deviceId, addr, commStat==1?"离线":"在线");
        }
    }
    if (stat){
	    m_version[deviceNo].stat = NODE_ONLINE;
	}
	else{
		m_version[deviceNo].stat = NODE_OFFLINE;
    }
    if (m_version[deviceNo].oldStat ^ m_version[deviceNo].stat) {	
        NodeInfo info;

        info.nodeType = MBType;
        info.nodeID   = deviceNo;
        info.nodeLinkStat = m_version[deviceNo].stat;
        info.nodeVersion  = m_version[deviceNo].nodeVersion;
    	info.nodeExtInfo[0] = m_version[deviceNo].deviceId;
        m_nodeInfoSoeFifo.pushBack(info);
    }
    m_version[deviceNo].oldStat  = m_version[deviceNo].stat;
}

/*******************************************************************************
@ Function Name     : processYkEvent
@ Description       : 解析YK CAN帧
@ Input             : frame :遥控接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processYkEvent(const SerialNetBuf &frame)
{
    uint8_t  id       = frame.Buf[0];
    uint16_t straddr  = (frame.Buf[2] << 8) | frame.Buf[3];
    uint16_t action   = (frame.Buf[4] << 8) | frame.Buf[5];

    if (frame.Buf[1] & 0x80)
    {
        modbusPrint(LOG_ERROR, "设备:%d 遥控失败,异常码:%d\n", id, frame.Buf[2]);
		if (m_ykCmdEvent.type == ykCmdNoResponseEvent)
			return ;
        m_ykStatus                  = ykidle;
        m_ykCmdEvent.availability   = true;
        m_ykCmdEvent.state          = eventActCon;
        m_ykCmdEvent.value          = failed;
        return ;
    }
    if (action != 0x0000)
    {
        modbusPrint(LOG_ERROR, "设备:%d 地址:%d 控合成功!\n", id, straddr);
    }
    else
    {
        modbusPrint(LOG_ERROR, "设备:%d 地址:%d 控分成功!\n", id, straddr);
    }
	m_ykStatus              = ykidle;
	if (m_ykCmdEvent.type == ykCmdNoResponseEvent)
	 	return;
    m_ykCmdEvent.availability   = true;
    m_ykCmdEvent.state          = eventActCon;
    m_ykCmdEvent.value          = successed;
}

/*******************************************************************************
@ Function Name     : processYkSelectEvent
@ Description       : 解析YK选择帧
@ Input             : frame :遥控接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processYkSelectEvent(const SerialNetBuf &frame)
{
    uint8_t  id       = frame.Buf[0];

    if (frame.Buf[1] & 0x80)
    {
        modbusPrint(LOG_ERROR, "设备:%d 遥控%s失败,异常码:%d\n", 
            id, m_ykStatus == ykidle ? "选择" : "撤销", frame.Buf[2]);
		if (m_ykCmdEvent.type == ykCmdNoResponseEvent)
			return ;
        m_ykStatus                  = ykidle;
        m_ykCmdEvent.availability   = true;
        m_ykCmdEvent.state          = eventActCon;
        m_ykCmdEvent.value          = failed;
        t_Yk.EndTimer();
        return ;
    }
    
    if (m_ykStatus == ykidle) {
        t_Yk.StartTimer();
	    m_ykStatus                  = ykselect;
        modbusPrint(LOG_ERROR, "设备:%d 遥控选择成功!\n", id);
    }
    else if (m_ykStatus == ykselect) {
        t_Yk.EndTimer();
	    m_ykStatus                  = ykidle;
        modbusPrint(LOG_ERROR, "设备:%d 遥控撤销成功!\n", id);
    }
    m_ykCmdEvent.availability   = true;
    m_ykCmdEvent.state          = eventActCon;
    m_ykCmdEvent.value          = successed;
}

/*******************************************************************************
@ Function Name     : processYtEvent
@ Description       : 解析Yt帧
@ Input             : frame :遥调接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processWriteEvent(const SerialNetBuf &frame)
{
    uint8_t  id       = frame.Buf[0];
    uint16_t straddr  = (frame.Buf[2] << 8) | frame.Buf[3];
    uint16_t action   = (frame.Buf[4] << 8) | frame.Buf[5];

    if (frame.Buf[1] & 0x80)
    {
        modbusPrint(LOG_INFO, "设备:%d 遥调失败,异常码:%d\n", id, frame.Buf[2]);
        return ;
    }
    modbusPrint(LOG_INFO, "设备:%d 地址:%d 值:%d 遥调成功!\n", id, straddr, action);
}
/*******************************************************************************
@ Function Name     : processYtEvent
@ Description       : 解析Yt帧
@ Input             : frame :遥调接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processMultiWriteEvent(const SerialNetBuf &frame)
{

}
/*******************************************************************************
@ Function Name     : putSoeToFifo
@ Description       : 将数据值推入SOE fifo
@ Input             : addr : 地址 
                      value: 值
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::putSoeToFifo(uint16_t addr, int16_t value, STLDeque<DataSoe> &soefifo)
{
    DataSoe soe;
    DateService date;
    DateType    time;

    soe.addr  = addr;
    soe.value = value;
    date.GetCurrentDate(&time);
    soe.dateTime[6] = time.m_year - 100;
    soe.dateTime[5] = time.m_mon;
    soe.dateTime[4] = time.m_mday | 0x20;
    soe.dateTime[3] = time.m_hour;
    soe.dateTime[2] = time.m_min;
    soe.dateTime[1] = (time.m_sec * 1000 + time.m_msec) >> 8;
    soe.dateTime[0] = (time.m_sec * 1000 + time.m_msec);

    soefifo.pushBack(soe);                                                      //二级SOE
}

/*******************************************************************************
@ Function Name     : putSoeToFifo
@ Description       : 将数据值推入SOE fifo
@ Input             : addr : 地址 
                      value: 值
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::putSoeToFifo(uint16_t addr, int16_t value, uint8_t type,STLDeque<DataSoe> &soefifo)
{
    DataSoe soe;
    DateService date;
    DateType    time;

    soe.addr  = addr;
    soe.value = value;
	soe.type  = type;
    date.GetCurrentDate(&time);
    soe.dateTime[6] = time.m_year - 100;
    soe.dateTime[5] = time.m_mon;
    soe.dateTime[4] = time.m_mday | 0x20;
    soe.dateTime[3] = time.m_hour;
    soe.dateTime[2] = time.m_min;
    soe.dateTime[1] = (time.m_sec * 1000 + time.m_msec) >> 8;
    soe.dateTime[0] = (time.m_sec * 1000 + time.m_msec);

    soefifo.pushBack(soe);                                                      //二级SOE
}

/*******************************************************************************
@ Function Name     : putSoeToFifoWthTime
@ Description       : 将数据值推入SOE fifo
@ Input             : addr : 地址 
                      value: 值
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::putSoeToFifoWthTime(uint16_t addr, int16_t value, STLDeque<DataSoe> &soefifo, DateType time)
{
    DataSoe soe;
    //DateService date;
    //DateType    time;

    soe.addr  = addr;
    soe.value = value;
    //date.GetCurrentDate(&time);

    soe.dateTime[6] = time.m_year;
    soe.dateTime[5] = time.m_mon;
    soe.dateTime[4] = time.m_mday;
    soe.dateTime[3] = time.m_hour;
    soe.dateTime[2] = time.m_min;
    soe.dateTime[1] = (time.m_sec * 1000 + time.m_msec) >> 8;
    soe.dateTime[0] = (time.m_sec * 1000 + time.m_msec);

    soefifo.pushBack(soe);                                                      //二级SOE
}

/*******************************************************************************
@ Function Name     : getFrameType
@ Description       : 获取帧的类型
@ Input             : @type: 帧类型
@ Output            : None;
@ Return            : None;
*******************************************************************************/
MBusFrameType ModBusProtocol::getFrameType(const char *type)
{
    if (!strcasecmp(type, "singleyx"))
        return frameSingleYx;
    else if (!strcasecmp(type, "doubleyx"))
        return frameDoubleYx;
    else if (!strcasecmp(type, "yc"))
        return frameYc;
    else if (!strcasecmp(type, "ym"))
        return frameYm;
	else if (!strcasecmp(type, "yt"))
        return frameYt;
    else if (!strcasecmp(type, "yk"))
        return frameYk;
    else if (!strcasecmp(type, "floatyc"))
        return frameFloatYc;
    else if (!strcasecmp(type, "int32yc"))
        return frameInt32Yc;
    else if (!strcasecmp(type, "param"))
        return frameYt;
    else if (!strcasecmp(type, "write"))
        return frameWR;
    else if (!strcasecmp(type, "multiyk"))
        return frameMultiYk;
    else if (!strcasecmp(type, "multiwrite"))
        return frameMultiWR;
	else if (!strcasecmp(type, "readErrReport"))
        return frameReadErrReport;
	else if (!strcasecmp(type, "readDir"))
        return frameReadDir;
	else if (!strcasecmp(type, "readWave"))
        return frameReadWave;
	else if (!strcasecmp(type, "mix"))
        return frameMix;
	else if(!strcasecmp(type, "readSoe"))
		 return frameYxSoe;
	else if(!strcasecmp(type, "readSoeNum"))
		 return frameYxSoeNum;
	else if(!strcasecmp(type, "int16yc"))
		return frameInt16Yc;
	else if(!strcasecmp(type, "int8yc"))
		return frameInt8Yc;
	
    return frameNoType;
}
/*******************************************************************************
@ Function Name     : getParseFrameType
@ Description       : 获取帧的类型
@ Input             : @type: 帧类型
@ Output            : None;
@ Return            : None;
*******************************************************************************/
MBusParseType ModBusProtocol::getParseFrameType(const char *parseType)
{
    if (!strcasecmp(parseType, "version"))
        return frameParseDataVersion;
    
	
    return frameParseDataNoType;
}

/*******************************************************************************
@ Function Name     : getMixFrameCode
@ Description       : 获取帧的类型
@ Input             : @type: 帧类型
@ Output            : None;
@ Return            : None;
*******************************************************************************/
MBMixFrameTypeCode ModBusProtocol::getMixFrameCode(const char *type)
{
    if (!strcasecmp(type, "singleyx"))
    {
        return MixFrameSingleYx;
    }
	else if (!strcasecmp(type, "doubleyx"))
	{
        return MixFrameDoubleYx;
	}
	else if (!strcasecmp(type, "yc"))
	{
        return MixFrameYc;
	}
	else if (!strcasecmp(type, "ym"))
	{
        return MixFrameYt;
	}
	else if (!strcasecmp(type, "readErrReport"))
	{
		return MixFrameReadErrReport;
	}
	else if (!strcasecmp(type, "readDir"))
	{
        return MixFrameReadDir;
	}
	else
	{
    	return MixFrameNoType;
	}
}

/*******************************************************************************
@ Function Name     : setFrame
@ Description       : 向命令队列插入请求命令帧
@ Input             : none
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::setFrame(uint16_t reqptr, uint16_t delayTime)
{
    if (reqptr == 0xFFFF)
        return false;

    MBusReqFifo reqcmd;
    SerialNetBuf frame;
    MBusRequest &request       = m_readRequest[reqptr];
    uint16_t		recordCounter = 0;

    if (waveWriteSuc)										//文件确认帧
    {
        frame.BufLen = 0;
        frame.Buf[frame.BufLen ++] = m_waveDeleteId;
        frame.Buf[frame.BufLen ++] = 0x14;							
        frame.Buf[frame.BufLen ++] = 0x00;
        frame.Buf[frame.BufLen ++] = 0x06;
        frame.Buf[frame.BufLen ++] = 0x00;
        frame.Buf[frame.BufLen ++] = 0x03;
        frame.Buf[frame.BufLen ++] = 0x00;
        frame.Buf[frame.BufLen ++] = 0xFF;
        frame.Buf[frame.BufLen ++] = 0x00;
        frame.Buf[frame.BufLen ++] = 0x00;
        waveWriteSuc = false;

        reqcmd.reqType        = rspNoDeal;					//不处理
        reqcmd.reqPtr         = reqptr;
        reqcmd.reqFrameType   = request.type;
        reqcmd.reqFrame       = frame;
        reqcmd.reqDelaySend   = delayTime;

        m_reqFifo.pushBack(reqcmd);
        return true;
    }

    if(request.funcode != FUNC_RD_FILE)
    {
        frame.BufLen = 0;
        frame.Buf[frame.BufLen ++] = request.deviceAddr;
        frame.Buf[frame.BufLen ++] = request.funcode;
        if(request.funcode == FUNC_RD_MIX)      //混合帧
        {
            for(uint8_t mixIndex = 0; mixIndex < request.subMBusRequest.size(); mixIndex++)
            {
                if(request.subMBusRequest[mixIndex].funcode == FUNC_RD_FILE || request.subMBusRequest[mixIndex].funcode == FUNC_SOE)
                {
                    frame.Buf[frame.BufLen ++] = request.subMBusRequest[mixIndex].type;
                    frame.Buf[frame.BufLen ++] = request.subMBusRequest[mixIndex].refType;
                    frame.Buf[frame.BufLen ++] = (request.subMBusRequest[mixIndex].fileNum >>8 ) & 0xFF;
                    frame.Buf[frame.BufLen ++] = request.subMBusRequest[mixIndex].fileNum & 0xFF;
                    frame.Buf[frame.BufLen ++] = (request.subMBusRequest[mixIndex].recordNum >>8 )& 0xFF;
                    frame.Buf[frame.BufLen ++] = request.subMBusRequest[mixIndex].recordNum & 0xFF;
                    frame.Buf[frame.BufLen ++] = (request.subMBusRequest[mixIndex].recordLenth >> 8)& 0xFF;
                    frame.Buf[frame.BufLen ++] = request.subMBusRequest[mixIndex].recordLenth& 0xFF;
                }
                else
                {
                    frame.Buf[frame.BufLen ++] = request.subMBusRequest[mixIndex].type;
                    frame.Buf[frame.BufLen ++] = (request.subMBusRequest[mixIndex].startAddr >> 8) & 0xFF;
                    frame.Buf[frame.BufLen ++] = request.subMBusRequest[mixIndex].startAddr & 0xFF;
                    frame.Buf[frame.BufLen ++] = (request.subMBusRequest[mixIndex].regNum >> 8) & 0xFF;
                    frame.Buf[frame.BufLen ++] = request.subMBusRequest[mixIndex].regNum & 0xFF;
                }
            }
        }
        else            //非混合帧，正常modbus帧
        {
            frame.Buf[frame.BufLen ++] = (request.startAddr >> 8) & 0xFF;
            frame.Buf[frame.BufLen ++] = request.startAddr & 0xFF;
            frame.Buf[frame.BufLen ++] = (request.regNum >> 8) & 0xFF;
            frame.Buf[frame.BufLen ++] = request.regNum & 0xFF;
        }
    }
    else                    //读文件
    {
        if(request.fileNum == FILENUMERRREPORT)
        {
            frame.BufLen = 0;
            frame.Buf[frame.BufLen ++] = request.deviceAddr;
            frame.Buf[frame.BufLen ++] = request.funcode;
            frame.BufLen ++;							
            for(recordCounter = 0; recordCounter < request.recordNum; recordCounter++)			//子请求循环部分
            {
                frame.Buf[frame.BufLen ++] = request.refType;
                frame.Buf[frame.BufLen ++] = (request.fileNum >>8 ) & 0xFF;
                frame.Buf[frame.BufLen ++] = request.fileNum & 0xFF;
                frame.Buf[frame.BufLen ++] = ((recordCounter+1) >>8 )& 0xFF;
                frame.Buf[frame.BufLen ++] = (recordCounter+1) & 0xFF;
                frame.Buf[frame.BufLen ++] = (request.recordLenth >> 8)& 0xFF;
                frame.Buf[frame.BufLen ++] = request.recordLenth& 0xFF;
            }
            frame.Buf[2]			   = frame.BufLen - 3;										//子请求字节计数
        }
        if(request.fileNum == FILENUMWAVEDIR)										
        {
            frame.BufLen = 0;
            frame.Buf[frame.BufLen ++] = request.deviceAddr;
            frame.Buf[frame.BufLen ++] = request.funcode;
            frame.BufLen ++;							
            frame.Buf[frame.BufLen ++] = request.refType;
            frame.Buf[frame.BufLen ++] = (request.fileNum >>8 ) & 0xFF;
            frame.Buf[frame.BufLen ++] = request.fileNum & 0xFF;
            frame.Buf[frame.BufLen ++] = (request.recordNum>>8 )& 0xFF;
            frame.Buf[frame.BufLen ++] = request.recordNum & 0xFF;
            frame.Buf[frame.BufLen ++] = (request.recordLenth >> 8)& 0xFF;
            frame.Buf[frame.BufLen ++] = request.recordLenth& 0xFF;
            frame.Buf[2]			   = frame.BufLen - 3;										//子请求字节计数
        }
        if (request.fileNum == FILENUMWAVECALL)
        {
            if(m_ycWaveNum.at(request.deviceAddr) == 0) {
                modbusPrint(LOG_DEBUG, "设备id:%d录波数目:%d",request.deviceAddr,m_ycWaveNum.at(request.deviceAddr));
                return true;
            }
            else {
                modbusPrint(LOG_DEBUG, "设备id:%d录波数目:%d",request.deviceAddr,m_ycWaveNum.at(request.deviceAddr));
            }
            frame.BufLen = 0;
            frame.Buf[frame.BufLen ++] = request.deviceAddr;
            frame.Buf[frame.BufLen ++] = request.funcode;
            frame.BufLen ++;							
            frame.Buf[frame.BufLen ++] = request.refType;
            frame.Buf[frame.BufLen ++] = (request.fileNum >>8 ) & 0xFF;
            frame.Buf[frame.BufLen ++] = request.fileNum & 0xFF;
            frame.Buf[frame.BufLen ++] = (waveFrameCounter >> 8 ) & 0xFF;
            frame.Buf[frame.BufLen ++] = waveFrameCounter & 0xFF;
            frame.Buf[frame.BufLen ++] = (request.recordLenth >> 8)& 0xFF;
            frame.Buf[frame.BufLen ++] = request.recordLenth& 0xFF;
            frame.Buf[2]			   = frame.BufLen - 3;										//子请求字节计数

            reqcmd.reqType        = rspReadFile;
            reqcmd.reqPtr         = reqptr;
            reqcmd.reqFrameType   = request.type;
            reqcmd.reqFrame       = frame;
            reqcmd.reqDelaySend   = delayTime;

            m_reqFifo.pushBack(reqcmd);
            t_wave.StartTimer(m_config.waveTimeOut);
            modbusPrint(LOG_DEBUG, "请求数据: ID:%d 功能码:%d ", request.deviceAddr, request.funcode);
            return true;
        }
        if (request.fileNum == MD_FILE_NUM_SOE_NUM)
        {
            frame.BufLen = 0;
            frame.Buf[frame.BufLen ++] = request.deviceAddr;
            frame.Buf[frame.BufLen ++] = request.funcode;
            frame.BufLen ++;							
            frame.Buf[frame.BufLen ++] = request.refType;
            frame.Buf[frame.BufLen ++] = (request.fileNum >>8 ) & 0xFF;
            frame.Buf[frame.BufLen ++] = request.fileNum & 0xFF;
            frame.Buf[frame.BufLen ++] = (request.recordNum>>8 )& 0xFF;
            frame.Buf[frame.BufLen ++] = request.recordNum & 0xFF;
            frame.Buf[frame.BufLen ++] = (request.recordLenth >> 8)& 0xFF;
            frame.Buf[frame.BufLen ++] = request.recordLenth& 0xFF;
            frame.Buf[2]			   = frame.BufLen - 3;										//子请求字节计数									//子请求字节计数

            reqcmd.reqType        = rspReadFile;
            reqcmd.reqPtr         = reqptr;
            reqcmd.reqFrameType   = request.type;
            reqcmd.reqFrame       = frame;
            reqcmd.reqDelaySend   = delayTime;

            m_reqFifo.pushBack(reqcmd);
            modbusPrint(LOG_DEBUG, "请求数据: ID:%d 功能码:%d ", request.deviceAddr, request.funcode);
            return true;
        }
        if(request.fileNum == MD_FILE_NUM_SOE) {      //存在soe时才读取数据
            return true;
        }
    }

    reqcmd.reqType        = rspReadReq;
    reqcmd.reqPtr         = reqptr;
    reqcmd.reqFrameType   = request.type;
    reqcmd.reqFrame       = frame;
    reqcmd.reqDelaySend   = delayTime;
    if (request.deviceIndex < m_deviceRetryCount.size()) 
    {
        m_deviceRetryCount[request.deviceIndex] ++;
    }
    m_reqFifo.pushBack(reqcmd);
    modbusPrint(LOG_DEBUG, "%s请求数据: ID:%d 功能码:%d ",m_setPoll?" ":"备用监听-", request.deviceAddr, request.funcode);
    return true;
}
/*******************************************************************************
@ Function Name     : preocessParamSet
@ Description       : param参数处理
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::preocessParamSet()
{

}

/*******************************************************************************
@ Function Name     : setTimePeriod
@ Description       : 定时授时
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::setTimePeriod()
{
    if (m_writeReg.size() == 0){
        return ;
    }
	DateService     date;
	DateType        time;
	uint16_t			tempData[8];
    uint16_t          num = m_writeReg[0].num;
	vector <uint16_t> data;
	date.GetCurrentDate(&time);
	
	tempData[0]   = time.m_year - 100;
    tempData[1]   = time.m_mon;
    tempData[2]   = time.m_mday;
    tempData[3]   = time.m_hour;
    tempData[4]   = time.m_min;
	tempData[5]	  = time.m_sec;
    tempData[6]   = (time.m_sec * 1000 + time.m_msec);
    tempData[7]   = (time.m_sec * 1000 + time.m_msec) >> 8;
	for(int dataNum = 0; dataNum < 7; dataNum++)
	{
		data.push_back(tempData[dataNum]);
	}
    for (uint16_t addr=0; addr<m_writeReg.size(); addr++) {
	    modbusPrint(LOG_INFO, "写时间，ID=%d", m_writeReg[addr].deviceAddr);
	    sendMutiWriteReg(frameMultiWR, addr * num, data);
    }
}

/*******************************************************************************
@ Function Name     : setParamConfig
@ Description       : 设置遥测参数
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::setParamConfig(const SpontEvent &event)
{
    uint16_t  addr    = event.addr;
    int8_t   node    = 0;
    uint16_t  idx     = 0;
    uint16_t  value   = event.data.data[0] + event.data.data[1]*256;

    m_setParamEvent = event;
    m_setParamEvent.availability   = false;
    for (idx=0; idx<m_readRequest.size(); idx++)
    {
        if (m_readRequest[idx].type == frameYc)
        {
            if ((addr >= m_readRequest[idx].dataOffset) && 
                (addr < m_readRequest[idx].dataOffset + m_readRequest[idx].dataNum))
            {
                m_dataParse.yc[addr].THV = value;
                node     = m_readRequest[idx].deviceAddr;
               
                break;
            }
        }
    }
    
    if (node < 0)
    {
        m_setParamEvent.availability = true;
        m_setParamEvent.state        = eventActCon;
        m_setParamEvent.value        = failed;
        return ;
    }

    if (saveParamConfig(node, addr, Thv1, value))                 //存储遥测参数
    {
        m_setParamEvent.availability = true;
        m_setParamEvent.state        = eventActCon;
        m_setParamEvent.value        = successed;
        modbusPrint(LOG_INFO, "设置参数成功: node=%d addr=%d value=%d\n", 
                                node, addr, value);
        return ;
    }
    modbusPrint(LOG_INFO, "设置参数失败: node=%d addr=%d value=%d\n", 
                                node, addr, value);
    m_setParamEvent.availability = true;
    m_setParamEvent.state        = eventActCon;
    m_setParamEvent.value        = failed;

    return ;
}

/*******************************************************************************
@ Function Name     : setDBParam
@ Description       : 设置遥测参数
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::setDBParam(const SpontEvent &event)
{
	uint16_t addr = 0;
	uint8_t qpm = 0;
	int16_t id  = 0;
	if(event.data.data[2]>80 && event.data.data[2] <125){
		qpm     = event.data.data[2]-80;
		id      = event.addr;       //单板id号
		addr    = qpm + 0xc078;
	}
	else{
		id = event.data.data[2]-125;
		addr = event.addr;
	}
  
    uint16_t  value   = event.data.data[0] + event.data.data[1]*256;

    m_setParamEvent = event;
    m_setParamEvent.availability   = false;
    
    if (id < 0)
    {
        m_setParamEvent.availability = true;
        m_setParamEvent.state        = eventActCon;
        m_setParamEvent.value        = failed;
        return ;
    }

    MBusReqFifo  reqcmd;
    SerialNetBuf  frame;
    uint8_t        deviceID   = (uint8_t)id;
    uint8_t        funcode    = FUNC_WR_SIGREG;   
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = (addr>>8)&0xff;
    frame.Buf[frame.BufLen ++] = addr & 0xff;
    frame.Buf[frame.BufLen ++] = (value>>8)&0xff;
    frame.Buf[frame.BufLen ++] = value & 0xff;    

    reqcmd.reqType        = rspWriteReq;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameYt;
    reqcmd.reqDelaySend   = 0;

    t_write.StartTimer(m_config.norspTime);
    m_reqFifo.pushBack(reqcmd);

    return ;
}

/*******************************************************************************
@ Function Name     : getYt
@ Description       : 读取参数值
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::getYt(const SpontEvent &event)
{
	//uint16_t  idx     = 0;
    int16_t  value   = 0;
    uint16_t  addr    = event.addr - (m_compatibleYt ? m_config.writeRegs : 0);
	
    m_setParamEvent = event;
    
    value = m_data.yt[addr].value;
    m_setParamEvent.availability = true;
    m_setParamEvent.state        = eventReq;
    m_setParamEvent.value        = successed;
	m_setParamEvent.data.data[0] = value & 0xFF;
    m_setParamEvent.data.data[1] = value >> 8;
	modbusPrint(LOG_INFO, "遥调读参:addr=%d value=%d", addr, value);

    return ;
}

/*******************************************************************************
@ Function Name     : setYt
@ Description       : 设置参数值
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::setYt(const SpontEvent &event)
{
	uint16_t addr = 0;
    vector<uint16_t> data;

    addr = event.addr - (m_compatibleYt ? m_config.writeRegs : 0);
   
    if (addr < m_data.yt.size())
    {
        m_setParamEvent                = event;
        m_setParamEvent.availability   = false;
        m_setParamEvent.state          = eventActCon;
        m_setParamEvent.value          = successed;

        data.push_back(event.data.data[0] | (event.data.data[1]<<8));

        modbusPrint(LOG_INFO, "遥调写参:addr=%d value=%d", addr, data[0]);
        
        sendYtReg(frameYt, addr, data);
        t_write.StartTimer(m_config.norspTime);
        
        return;
    }

    modbusPrint(LOG_WARNING, "遥调地址越界, addr=%d, num=%d!", addr, m_data.yt.size());
    
    m_setParamEvent                = event;
    m_setParamEvent.availability   = true;
    m_setParamEvent.state          = eventNoInforAddr;
    m_setParamEvent.value          = failed;
    
    return ;
}

/*******************************************************************************
@ Function Name     : getParamConfig
@ Description       : 读取遥测门限值参数
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::getParamConfig(const SpontEvent &event)
{
    uint16_t  idx     = 0;
    uint16_t  value   = 0;
    uint16_t  addr    = event.addr;

    m_setParamEvent = event;
    m_setParamEvent.availability   = false;
    for (idx=0; idx<m_readRequest.size(); idx++)
    {
        if (m_readRequest[idx].type == frameYc)
        {
            if ((addr >= m_readRequest[idx].dataOffset) && 
                (addr < m_readRequest[idx].dataOffset + m_readRequest[idx].dataNum))
            {
                value    = m_dataParse.yc[addr].THV;
                modbusPrint(LOG_INFO, "读取参数:addr=%d value=%d\n", 
                                    addr, value);
                m_setParamEvent.availability = true;
                m_setParamEvent.state        = eventReq;
                m_setParamEvent.value        = successed;
                m_setParamEvent.data.data[0] = value & 0xFF;
                m_setParamEvent.data.data[1] = value >> 8;
                return ;
            }
        }
		else{

		}
    }
    
    m_setParamEvent.availability = true;
    m_setParamEvent.state        = eventReq;
    m_setParamEvent.value        = failed;

    return ;
}

/*******************************************************************************
@ Function Name     : saveParamConfig
@ Description       : 存储遥测参数至配置文件
@ Input             : node   :遥测板卡节点
                      circuit:回路号
                      qpm    :参数类型
                      value  :参数值
@ Output            : CanConfig.xml
@ Return            : true   :存储文件成功
                      false  :存储文件失败
*******************************************************************************/
bool ModBusProtocol::saveParamConfig(
                                  uint8_t node, 
                                  uint8_t offset, 
                                  uint8_t qpm, 
                                  uint16_t value
                                 )
{
    char paramName[5] = "THV";
    XmlNodeParser f_XmlNodeParser((int8_t *)m_configFile.c_str(), (int8_t *)"/ModBusMasterConfig");
    int8_t nodePath[120] = "";

    sprintf((char *)nodePath, "//Device[%d]/ReadRequest/frame[@type='yc']/data[%d]", 
                              node, offset+1);
    
    modbusPrint(LOG_INFO, "遥测参数存储:设备=%d, 地址=%d, 参数='%s', 值=%d\n",
            node, offset, paramName, value);
    
    if (f_XmlNodeParser.FindNode(nodePath))
    {
        if (f_XmlNodeParser.ModifyProperty((int8_t *)paramName, value))
        {
            modbusPrint(LOG_INFO, "存储遥测参数文件成功!\n");

            return true;
        }
    }

    return false;
}

/*******************************************************************************
@ Function Name     : sendMBusFrame
@ Description       : 发送modbus帧
@ Input             : none
@ Output            : m_sendFrameFifo;
@ Return            : true   :成功
                      false  :失败
*******************************************************************************/
bool ModBusProtocol::sendMBusFrame(SerialNetBuf &frame)
{
    uint16_t crc = CrcMake::MakeCrc(frame.Buf, frame.BufLen);
    bool   rsp = false;

    frame.Buf[frame.BufLen++] = crc & 0xFF;
    frame.Buf[frame.BufLen++] = (crc >> 8) & 0xFF;

	
    if ((d_doubleBackup == NULL))
    {
//        modbusPrint(LOG_DEBUG, "请求数据：ID:%d    功能码:%d", frame.Buf[0], frame.Buf[1]);
        modbusPrintBuf(LOG_DEBUG, "frame data:\t", frame.Buf, frame.BufLen);
        rsp = m_sendFrameFifo.pushBack(frame);
    }
    else
        rsp = true;     //备用模式下不发送帧到fifo
        
    if (rsp)
    {
        if(frame.Buf[0] == 0xff)        //广播帧时，写参帧超时减少为1ms，在线升级时写数据，为300ms
        {
            if(frame.Buf[1] == 0x10)
                t_rsp.StartMsTimer(1);
            else
                t_rsp.StartTimer(m_config.norspTime);
        }
        else
            t_rsp.StartTimer(m_config.norspTime);
    }
    
    return rsp;
}

/*******************************************************************************
@ Function Name     : setUpdate
@ Description       : 启动在线升级
@ Input             : event
@ Output            : None;
@ Return            : true or false
*******************************************************************************/
bool ModBusProtocol::setUpdate(const SpontEvent &event)
{
    DBNodeType type = (DBNodeType)(event.data.data[0]);
    uint8_t     cpuNum = event.data.data[1];


    if (m_updateBuff.status == ModbusUpdateStCodeBusy)
    {
        modbusPrint(LOG_ERROR, "正在进行在线升级,请稍后尝试!\n");
        return false;
    }

    if (type == MReset_Bottom)
    {
        type = MBottom;
        composeUpdateReset(cpuNum-1,type);      //走webSocket接口，cpuNum需-1
        return false;
    }
    else if (type == MReset_Roof)
    {
        type = MRoof;
        composeUpdateReset(cpuNum-1,type);
        return false;
    }
    else if (type == MSetParam)
    {
        sendYtReg(cpuNum-1, (event.data.data[3] << 8) | event.data.data[2], (event.data.data[5] << 8) | event.data.data[4]);
        return false;
    }
    m_updateEvent              = event;
    m_updateEvent.availability = false;

    if(composeUpdateStart(cpuNum, type))
    {
        modbusPrint(LOG_WARNING, "单板:%d，%s，在线升级准备...",cpuNum, type==MRoof?"顶板":"底板");
        m_updateBuff.node = cpuNum;
        m_updateBuff.type = type;
        m_updateBuff.segRepeat = 0;
        m_updateBuff.status = ModbusUpdateStCodeBusy;
        m_updateBuff.updateFlag = updateCmdStart;
        return true;
    }
    return false;
}

/*******************************************************************************
@ Function Name     : composeUpdateStart
@ Description       : 在线升级开始
@ Input             : node: 设备id
                      type: 节点类型
@ Output            : None;
@ Return            : true or false
*******************************************************************************/
bool ModBusProtocol::composeUpdateStart(uint8_t node, DBNodeType type)    
{
    MBusReqFifo  reqcmd;
    SerialNetBuf  frame;
    uint8_t        deviceID   = node;
    uint8_t        funcode    = 0;

    struct stat  file;

    if (type == MBottom  || type == MSerial0 || type == MSerial1 || type == MSerial2 || type == MSerial3 || type == MSerial4 || type == MSerial5 || type == MSerial6|| type == MSerial7|| type == MSerial)    //打开文件
    {
        if (lstat(Bottom_UPDATE_FILE, &file) == -1)
        {
            modbusPrint(LOG_ERROR, 
                    "\t单板底板在线升级文件不存在: errno=%d!\n", errno);
            return false;
        }
        funcode = FUNC_UPDATE_BOTTOM;
        m_updateBuff.dataLen = file.st_size;
    }
    else
    if (type == MRoof)
    {
        if (lstat(Roof_UPDATE_FILE, &file) == -1)
        {
            modbusPrint(LOG_ERROR, 
                "\t单板顶板在线升级文件不存在: errno=%d!\n", errno);
            return false;
        }
        funcode = FUNC_UPDATE_ROOF;
        m_updateBuff.dataLen = file.st_size;
    }
    else
    {
        modbusPrint(LOG_ERROR, "在线升级类型错误，升级失败");
        return false;
    }
    
    if (file.st_size == 0)
    {
        modbusPrint(LOG_ERROR, "\t在线升级文件长度为零!\n");
        return false;
    }    
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = updateCmdStart;
    frame.Buf[frame.BufLen ++] = (m_updateBuff.dataLen >> 16) & 0xFF;
    frame.Buf[frame.BufLen ++] = (m_updateBuff.dataLen >> 8) & 0xFF;
    frame.Buf[frame.BufLen ++] = (m_updateBuff.dataLen) & 0xFF;

    reqcmd.reqType        = rspUpdate;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameUpdate;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    
    modbusPrint(LOG_INFO, "在线升级开始...");

	//广播升级去掉超时
	if(deviceID == 0xff){
    	m_update.EndTimer();
	}
	else{
		m_update.StartTimer(ModbusUpdateWaitTime);
	}

    return true;
}

/*******************************************************************************
@ Function Name     : composeUpdateErase
@ Description       : 擦除指定单板存储分区
@ Input             : none
@ Output            : 录波文件
@ Return            : true   :存储文件成功
                      false  :存储文件失败
*******************************************************************************/
void ModBusProtocol::composeUpdateErase(uint8_t node, DBNodeType type)    //擦除FLASH
{
    SerialNetBuf  frame;
    MBusReqFifo  reqcmd;
    uint8_t        deviceID   = node;
    uint8_t        funcode    = 0;

    if(type == MBottom  || type == MSerial0 || type == MSerial1 || type == MSerial2 
        || type == MSerial3 || type == MSerial4 || type == MSerial5 || type == MSerial6
        || type == MSerial7 || type == MSerial)                                 ///??????
        funcode = FUNC_UPDATE_BOTTOM;
    else if(type == MRoof)
        funcode = FUNC_UPDATE_ROOF;
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = updateCmdErase;
    
    reqcmd.reqType        = rspUpdate;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameUpdate;
    reqcmd.reqDelaySend   = 0;

    modbusPrint(LOG_WARNING, "擦除FLASH...");
    m_reqFifo.pushBack(reqcmd);
	m_update.StartTimer(ModbusUpdateWaitTime);
}

/*******************************************************************************
@ Function Name     : parseUpdateFile
@ Description       : 解析在线升级文件数据至在线升级缓冲区中
@ Input             : none
@ Output            : 录波文件
@ Return            : true   :存储文件成功
                      false  :存储文件失败
*******************************************************************************/
bool ModBusProtocol::parseUpdateFile(const char *filename)
{
    FILE *fp = NULL;

    if ((fp = fopen(filename, "rb")) == NULL)
    {
        modbusPrint(LOG_WARNING, "打开文件%s失败: errno=%d!\n", filename, errno);
        return false;
    }

    fseek(fp, 0, SEEK_END);
//    m_updateBuff.dataLen = ftell(fp);
    rewind(fp);

    m_updateBuff.segNum = 0;
//    m_updateBuff.segRepeat = 0;
    m_updateBuff.sendCount = 0;
    m_updateBuff.prevSendCount = 0;
    m_updateBuff.d_data = new uint8_t[m_updateBuff.dataLen];

    if (fread(m_updateBuff.d_data, 1, m_updateBuff.dataLen, fp) == m_updateBuff.dataLen)
    {
        fclose(fp);
        return true;
    }

    modbusPrint(LOG_WARNING, "读取的数据长度与预期不一致!\n");
    fclose(fp);
    return false;
}

/*******************************************************************************
@ Function Name     : composeUpdateTrans
@ Description       : 数据传输
@ Input             : none
                      type
                      errCode
@ Output            : 
@ Return            : true   :存储文件成功
                      false  :存储文件失败
*******************************************************************************/
bool ModBusProtocol::composeUpdateTrans(uint8_t node, DBNodeType type, UpdateExceptionCode errCode)    
{
    SerialNetBuf  frame;
    MBusReqFifo  reqcmd;
    uint8_t        deviceID   = node;
    uint8_t        funcode    = 0;
    uint32_t      point = 0;      //指向待发送的数据地址

    if(type == MBottom  || type == MSerial0 || type == MSerial1 || 
        type == MSerial2 || type == MSerial3 || type == MSerial4 || 
        type == MSerial5 || type == MSerial6 || type == MSerial7 ||type == MSerial)
        funcode = FUNC_UPDATE_BOTTOM;
    else if(type == MRoof)
        funcode = FUNC_UPDATE_ROOF;
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = updateCmdTrans;
    frame.Buf[frame.BufLen ++] = 0;              //此帧包含的数据数

    if(errCode != updateErrNormal)
    {
        point = m_updateBuff.sendCount - m_updateBuff.prevSendCount;
        frame.Buf[frame.BufLen ++] = (point >> 16) & 0xff;
        frame.Buf[frame.BufLen ++] = (point >> 8  ) & 0xff;
        frame.Buf[frame.BufLen ++] = point & 0xff;

        memcpy(&frame.Buf[frame.BufLen], m_updateBuff.d_data+point, m_updateBuff.prevSendCount);
        frame.BufLen += m_updateBuff.prevSendCount;
    }
    else
    {
        point = m_updateBuff.sendCount;
        frame.Buf[frame.BufLen ++] = (point >> 16) & 0xff;
        frame.Buf[frame.BufLen ++] = (point >> 8 ) & 0xff;
        frame.Buf[frame.BufLen ++] = point & 0xff;
        if(point+200 > m_updateBuff.dataLen)        //最后一帧
        {
            m_updateBuff.prevSendCount = m_updateBuff.dataLen - point;
        }
        else
        {
            m_updateBuff.prevSendCount = 200;
        }
        memcpy(&frame.Buf[frame.BufLen], m_updateBuff.d_data+point, m_updateBuff.prevSendCount);
        frame.BufLen += m_updateBuff.prevSendCount;
        m_updateBuff.sendCount += m_updateBuff.prevSendCount;
        m_updateBuff.segNum ++;       
    }

    frame.Buf[3] = m_updateBuff.prevSendCount;
    
    reqcmd.reqType        = rspUpdate;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameUpdate;
    reqcmd.reqDelaySend   = 0;

    m_updateBuff.updateFlag = updateCmdTrans;
	
    modbusPrint(LOG_WARNING, "第%d段，数据传输...",m_updateBuff.segNum);
    m_reqFifo.pushBack(reqcmd);
	m_update.StartTimer(ModbusUpdateWaitTime);
    return !(m_updateBuff.prevSendCount == 200);
}

/*******************************************************************************
@ Function Name     : composeUpdateWrite
@ Description       : 写FLASH
@ Input             : none
                      type
@ Output            : 
@ Return            : 
*******************************************************************************/
void ModBusProtocol::composeUpdateWrite(uint8_t node, DBNodeType type)    //
{
    SerialNetBuf  frame;
    MBusReqFifo  reqcmd;
    uint8_t        deviceID   = node;
    uint8_t        funcode    = 0;

    if(type == MBottom  || type == MSerial0 || type == MSerial1 || 
        type == MSerial2 || type == MSerial3 || type == MSerial4 || 
        type == MSerial5 || type == MSerial6 || type == MSerial7 ||type == MSerial)
        funcode = FUNC_UPDATE_BOTTOM;
    else if(type == MRoof)
        funcode = FUNC_UPDATE_ROOF;
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = updateCmdWrite;
    
    reqcmd.reqType        = rspUpdate;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameUpdate;
    reqcmd.reqDelaySend   = 0;

    modbusPrint(LOG_WARNING, "写FLASH...");
    m_reqFifo.pushBack(reqcmd);

	m_update.StartTimer(ModbusUpdateWaitTime);
}

/*******************************************************************************
@ Function Name     : composeUpdateReset
@ Description       : 复位
@ Input             : none
                      type
@ Output            : 
@ Return            : 
*******************************************************************************/
void ModBusProtocol::composeUpdateReset(uint8_t node, DBNodeType type)    
{
    SerialNetBuf  frame;
    MBusReqFifo  reqcmd;
    uint8_t        deviceID   = node;
    uint8_t        funcode    = 0;

    if(type == MRoof)
        funcode = FUNC_UPDATE_ROOF;
	else{
		funcode = FUNC_UPDATE_BOTTOM;
	}
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = updateCmdReset;
    
    reqcmd.reqType        = rspUpdate;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameUpdate;
    reqcmd.reqDelaySend   = 0;

    modbusPrint(LOG_WARNING, "复位设备, 节点号：%d, 类型：%s", node, type==MRoof?"顶板":"底板");
    m_reqFifo.pushBack(reqcmd);
}
/*******************************************************************************
@ Function Name     : processUpdateRequest
@ Description       : 处理在线升级请求回复帧
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processUpdateRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd)
{
    const SerialNetBuf   &request = reqcmd.reqFrame;
    uint8_t       id       = frame.Buf[0];
    DBNodeType  type = (DBNodeType)(frame.Buf[1]-0x55);
    UpdateExceptionCode errCode = (UpdateExceptionCode)frame.Buf[3];

    if (frame.Buf[1] & 0x80)
    {
        modbusPrint(LOG_ERROR, "从设备应答异常,异常码:%d\n", frame.Buf[2]);
        return ;
    }
    if (id != request.Buf[0])
    {
        modbusPrint(LOG_ERROR, "应答帧设备地址不正确, req=%d, rsp=%d!\n", 
                    request.Buf[0], id);
        return ;
    }

    if (frame.Buf[1] != request.Buf[1])
    {
        modbusPrint(LOG_ERROR, "应答帧功能不一致, req=%d, rsp=%d!\n",
                    request.Buf[1], frame.Buf[1]);
        return ;
    }

    switch(m_updateBuff.updateFlag)      //功能字
    {
        case updateCmdStart:                                                                                            //升级启动
        {
			t_SetTime.EndTimer();																						//关闭授时帧
            m_update.EndTimer();
            if (errCode != updateErrNormal)
            {
                if (m_updateBuff.segRepeat++ >= UPDATE_MAX_RETRY_NUM)
                {
                    m_updateBuff.segRepeat = 0;
                    m_updateBuff.status = ModbusUpdateStCodeFailed;
                    UpdateStatReport();                    
                    modbusPrint(LOG_ERROR, "超过尝试次数，在线升级启动失败");
                }
                else
                {
                    composeUpdateStart(id, type);
                }
            }
            else
            {
                m_updateBuff.segRepeat = 0;                
                composeUpdateErase(id, type);
                m_updateBuff.updateFlag = updateCmdErase;
            }
            break;
        }
        case updateCmdErase:                                                                                    //擦除FLASH
        {
            m_update.EndTimer();
            if (errCode != updateErrNormal)
            {
                if (m_updateBuff.segRepeat++ >= UPDATE_MAX_RETRY_NUM)
                {
                    m_updateBuff.segRepeat = 0;
                    m_updateBuff.status = ModbusUpdateStCodeFailed;
                    UpdateStatReport();
                    modbusPrint(LOG_ERROR, "超过尝试次数，在线升级擦除失败");
                }
                else
                {
                    //擦除FLASH
                    composeUpdateErase(id, type);
                }
            }
            else
            {
                m_updateBuff.segRepeat = 0;               
                switch(type)        //将升级文件导入缓冲区
                {
                    case MBottom:
					case MSerial0:
					case MSerial1:
					case MSerial2:
					case MSerial3:
					case MSerial4:
					case MSerial5:
					case MSerial6:
					case MSerial7:
					case MSerial:
                    {
                        if(!parseUpdateFile(Bottom_UPDATE_FILE))
                        {
                            modbusPrint(LOG_ERROR, "文件出错，在线升级失败");
                            m_updateBuff.status = ModbusUpdateStCodeFailed;
                            UpdateStatReport();
                            return;
                        }
                        break;
                    }
                    case MRoof:
                    {
                        if(!parseUpdateFile(Roof_UPDATE_FILE))
                        {
                            modbusPrint(LOG_ERROR, "文件出错，在线升级失败");
                            m_updateBuff.status = ModbusUpdateStCodeFailed;
                            UpdateStatReport();
                            return;
                        }
                        break;
                    }
                    default:
                    {
                        modbusPrint(LOG_ERROR, "板卡类型出错，在线升级失败");
                        m_updateBuff.status = ModbusUpdateStCodeFailed;
                        UpdateStatReport();
                        return;
                    }
                }
                composeUpdateTrans(id, type, errCode);
                m_updateBuff.updateFlag = updateCmdTrans;
            }
            break;
        }
        case updateCmdTrans:                                                                                             //数据传输                         
        {
            m_update.EndTimer();
            if (errCode != updateErrNormal)
            {
                if (m_updateBuff.segRepeat++ >= UPDATE_MAX_RETRY_NUM)
                {
                    m_updateBuff.segRepeat = 0;
                    m_updateBuff.status = ModbusUpdateStCodeFailed;
                    UpdateStatReport();
                    modbusPrint(LOG_ERROR, "超过尝试次数，在线升级传输失败");
                }
                else
                {
                    composeUpdateTrans(id, type, errCode);
                }
            }
            else
            {
                if(composeUpdateTrans(id, type, errCode))       //传输完成
                {
                    m_updateBuff.updateFlag = updateCmdWrite;
                }
            }
            break;
        }
        case updateCmdWrite:                                                                                                //写FLASH
        {   
            m_update.EndTimer();
            if (errCode != updateErrNormal)              //若最后一帧数据出错
            {
                if (m_updateBuff.segRepeat++ >= UPDATE_MAX_RETRY_NUM)
                {
                    m_updateBuff.segRepeat = 0;
                    m_updateBuff.status = ModbusUpdateStCodeFailed;
                    UpdateStatReport();
                    modbusPrint(LOG_ERROR, "超过尝试次数，在线升级传输失败");
                }
                else
                {
                    composeUpdateTrans(id,type, errCode);
                }
            }
            else
            {
                m_updateBuff.segRepeat = 0;
                modbusPrint(LOG_WARNING, "数据传输完成!\n烧写FLASH...");
                composeUpdateWrite(id, type);
                m_updateBuff.updateFlag = updateCmdReset;                
            }
            break;
        }
        case updateCmdReset:
        {
            m_update.EndTimer();
			t_SetTime.StartTimer();
            if (errCode != updateErrNormal)
            {
                m_updateBuff.segRepeat = 0;
                m_updateBuff.status = ModbusUpdateStCodeFailed;
                UpdateStatReport();
                modbusPrint(LOG_ERROR, "超过尝试次数，在线升级烧写FLASH失败");
            }
            else
            {
                m_updateBuff.segRepeat = 0;
                modbusPrint(LOG_WARNING, "烧写FLASH成功!\n在线升级成功，复位设备!");
                composeUpdateReset(id,type);
                m_updateBuff.status = ModbusUpdateStCodeSuccessed;   
                UpdateStatReport();
            }
            break;
        }
        default:break;
    }
}
/*******************************************************************************
@ Function Name     : UpdateStatReport
@ Description       : 在线升级上报状态
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::UpdateStatReport(void)
{
    m_updateEvent.availability = true;
    m_updateEvent.state        = eventActCon;     
    
    if(m_updateBuff.status != ModbusUpdateStCodeSuccessed)
    {
        m_updateBuff.status = ModbusUpdateStCodeNone;
        m_updateEvent.value        = failed;
    }
    else
        m_updateEvent.value        = successed;
        
    if (m_updateBuff.d_data)
    {
        delete [] m_updateBuff.d_data;
        m_updateBuff.d_data = NULL;
    }
    m_updateBuff.updateFlag = updateCmdError;
}

/*******************************************************************************
@ Function Name     : ReadRecord
@ Description       : 招取事件记录
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::ReadRecord(const SpontEvent &event)
{
    uint8_t     funcode = 0x80;
    uint8_t     element[5] = {0};
    
    memcpy(element,  event.data.data, sizeof(element));
    funcode = element[0];
    modbusPrintBuf(LOG_INFO, "event.data: \t", event.data.data, sizeof(element));
    
    if (m_RecordBuff.status == ModbusRecordStCodeBusy)
    {
        modbusPrint(LOG_ERROR, "正在招取事件记录,请稍后尝试!\n");
        return false;
    }
    m_readRecord             = event;
    m_readRecord.availability = false;

    if(funcode == FUNC_RECORD_NUM)
    {
        m_RecordBuff.node = element[3];
        composeReadRecordNum(element[3]);
        return true;
    }
    else
    if(funcode == FUNC_RECORD)
    {
        if(m_RecordBuff.node == 0)
            return false;       
        uint16_t addr = element[1] << 8 | element[2];
        uint8_t   num = element[3];
        composeReadRecord(addr, num);
        return true;
    }
    else 
    if(funcode == FUNC_RECORD_CLEAN)
    {
        modbusPrint(LOG_WARNING, "web端清除单板事件记录");
        m_RecordBuff.node = element[3];
        composeRecordNumClean(element[3]);
        return true;
    }
    return false;
}

/*******************************************************************************
@ Function Name     : getPeerUpdatingStat
@ Description       : 获取对端升级状态
@ Input             : baseCircuitAddr
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::getPeerUpdatingStat() 
{
    return m_peerIsUpdating;
}

/*******************************************************************************
@ Function Name     : setBaseCircuitAddr
@ Description       : 设置回路基址
@ Input             : baseCircuitAddr
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::setBaseCircuitAddr(int baseCircuitAddr) 
{
    m_baseCir = baseCircuitAddr;
    modbusPrint(LOG_INFO, "回路基址:%d", m_baseCir);
}

/*******************************************************************************
@ Function Name     : composeReadRecordNum
@ Description       : 招取事件记录数目
@ Input             : node 设备ID
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::composeReadRecordNum(uint8_t node)    
{
    MBusReqFifo  reqcmd;
    SerialNetBuf  frame;
    uint8_t        deviceID   = node;
    uint8_t        funcode    = FUNC_RECORD_NUM;   
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = 0x00;
    frame.Buf[frame.BufLen ++] = 0x00;
    frame.Buf[frame.BufLen ++] = 0x00;
    frame.Buf[frame.BufLen ++] = 0x01;    

    reqcmd.reqType        = rspReadRecord;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameRecord;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    m_RecordBuff.status = ModbusRecordStCodeBusy;
    m_Record.StartTimer(m_RecordBuff.WaitTime);

    return true;
}

/*******************************************************************************
@ Function Name     : composeRecordNumClean
@ Description       : 清除事件记录
@ Input             : node 设备ID
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::composeRecordNumClean(uint8_t node)    
{
    MBusReqFifo  reqcmd;
    SerialNetBuf  frame;
    uint8_t        deviceID   = node;
    uint8_t        funcode    = FUNC_RECORD_CLEAN;   
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = 0x00;
    frame.Buf[frame.BufLen ++] = 0x00;
    frame.Buf[frame.BufLen ++] = 0xFF;
    frame.Buf[frame.BufLen ++] = 0x00;    

    reqcmd.reqType        = rspReadRecord;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameRecord;
    reqcmd.reqDelaySend   = 0;

    m_reqFifo.pushBack(reqcmd);
    m_RecordBuff.status = ModbusRecordStCodeBusy;
    m_Record.StartTimer(m_RecordBuff.WaitTime);

    return true;
}

/*******************************************************************************
@ Function Name     : composeReadRecord
@ Description       : 招取事件记录
@ Input             : addr 事件记录地址
                      num  记录数目
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::composeReadRecord(uint16_t addr, uint8_t num)    
{
    MBusReqFifo  reqcmd;
    SerialNetBuf  frame;
    uint16_t      SegNum  = 0xE000;
    uint8_t        deviceID   = m_RecordBuff.node;
    uint8_t        funcode    = FUNC_RECORD;   
    addr += SegNum;
    
    frame.BufLen = 0;
    frame.Buf[frame.BufLen ++] = deviceID;
    frame.Buf[frame.BufLen ++] = funcode;
    frame.Buf[frame.BufLen ++] = (addr >> 8) & 0xff;
    frame.Buf[frame.BufLen ++] = addr & 0xff;
    frame.Buf[frame.BufLen ++] = 0x00;
    frame.Buf[frame.BufLen ++] = num;    

    reqcmd.reqType        = rspReadRecord;
    reqcmd.reqPtr         = 0;
    reqcmd.reqFrame       = frame;
    reqcmd.reqFrameType   = frameRecord;
    reqcmd.reqDelaySend   = 0;
    m_RecordBuff.status = ModbusRecordStCodeBusy;
    m_reqFifo.pushBack(reqcmd);
        
    m_Record.StartTimer(m_RecordBuff.WaitTime);

    return true;
}

/*******************************************************************************
@ Function Name     : processRecordRequest
@ Description       : 文件记录响应帧处理
@ Input             : addr 事件记录地址
                      num  记录数目
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::processRecordRequest(const SerialNetBuf &frame, const MBusReqFifo &reqcmd, char *devIpv4)
{
    const SerialNetBuf   &request = reqcmd.reqFrame;
    uint8_t       id       = frame.Buf[0];
    uint8_t   funcode = frame.Buf[1];

    m_Record.EndTimer();
    if (frame.Buf[1] & 0x80)
    {
        modbusPrint(LOG_ERROR, "从设备应答异常,异常码:%d\n", frame.Buf[2]);
        ReSendFrame();
        return;
    }
    if (id != request.Buf[0])
    {
        modbusPrint(LOG_ERROR, "应答帧设备地址不正确, req=%d, rsp=%d!\n", 
                    request.Buf[0], id);
        ReSendFrame();
        return;
    }

    if (frame.Buf[1] != request.Buf[1])
    {
        modbusPrint(LOG_ERROR, "应答帧功能不一致, req=%d, rsp=%d!\n",
                    request.Buf[1], frame.Buf[1]);
        ReSendFrame(); 
        return;
    }

    m_RecordBuff.segRepeat = 0;
    switch(funcode)      //功能码
    {
        case FUNC_RECORD_NUM:
        {           
            memcpy(m_readRecord.data.data, frame.Buf, frame.BufLen);
//            modbusPrintBuf(LOG_INFO, "FUNC_RECORD_NUM, frame: ", frame.Buf, frame.BufLen);
            m_readRecord.data.len = frame.BufLen;
            reportErr(true);
            break;
        }
        case FUNC_RECORD:
        {           
            memcpy(m_readRecord.data.data, frame.Buf, frame.BufLen);
            m_readRecord.data.len = frame.BufLen;
            reportErr(true);
            break;
        }
        case FUNC_RECORD_CLEAN:
        {
            memcpy(m_readRecord.data.data, frame.Buf, frame.BufLen);
            m_readRecord.data.len = frame.BufLen;
            reportErr(true);
        }
        default :break;
    }
}

/*******************************************************************************
@ Function Name     : reportErr
@ Description       : 上报事件
@ Input             : stat 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::reportErr(bool stat)
{
    m_readRecord.value = stat==true?successed:failed;
    m_readRecord.availability = true;
    if(stat)
        m_RecordBuff.status = ModbusRecordStCodeSuccessed;
}

/*******************************************************************************
@ Function Name     : ReSendFrame
@ Description       : 错误重发
@ Input             : none
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::ReSendFrame(void)
{
    m_RecordBuff.segRepeat ++;
    m_RecordBuff.status = ModbusRecordStCodeFailed;
    if(m_RecordBuff.segRepeat > 3)
    {
        m_RecordBuff.segRepeat = 0;
        reportErr(false);
        return;
    }
    modbusPrint(LOG_ERROR, "错误重发，segRepeat = %d", m_RecordBuff.segRepeat);
    ReadRecord(m_readRecord);
}

/*******************************************************************************
@ Function Name     : simDownAckFrame
@ Description       : 模拟下行发送的应答帧
@ Input             : cmd 在线升级功能字
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusProtocol::simDownAckFrame(UpdateCmd _updateCmd){

//模拟下行发送的应答帧
	SerialNetBuf frame;

	frame.BufLen = 0;
	frame.Buf[frame.BufLen++] = BOARDCAST_ID;
	frame.Buf[frame.BufLen++] = FUNC_UPDATE_BOTTOM;
	frame.Buf[frame.BufLen++] = _updateCmd;
	frame.Buf[frame.BufLen++] = 0x00;
	m_recvFrameFifo.pushBack(frame);
}

/*******************************************************************************
@ Function Name     : getSerialIndex
@ Description       : ?????
@ Input             : 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
uint8_t ModBusProtocol::getSerialIndex(uint8_t serialNum){

	uint8_t serialIndex = 2;

	serialIndex += serialNum;
	
	return serialIndex;
}

/*******************************************************************************
@ Function Name     : setDataPoll
@ Description       : 在线升级
@ Input             : event:  事件
@ Output            : 
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusProtocol::setDataPoll(bool setStop, bool notify)
{
    return true;
}

/*******************************************************************************
@ Function Name     : eqAckMatch
@ Description       : 请求帧和应答匹配(混合帧,读线圈，读离线，读保持，读输入)
@ Input             : rcvframe 接收帧 
@ Output            : retReadRequest 返回对应的解析帧的索引号;
@ Return            : None;
*******************************************************************************/
uint16_t ModBusProtocol::eqAckMatch(const SerialNetBuf &rcvframe)
{
    uint8_t rcvId = rcvframe.Buf[0];
    uint8_t rcvFunCode = rcvframe.Buf[1];
    uint16_t  rcvStartAddr = rcvframe.Buf[2]<<8 | rcvframe.Buf[3];
    uint16_t rcvRegNum = rcvframe.Buf[4]<<8 | rcvframe.Buf[5];
    uint16_t retIndex = 0xFFFF;
    printfsBuf(LOG_DEBUG, "进入的匹配帧数据:", rcvframe.Buf, rcvframe.BufLen);

    if (rcvId == BOARDCAST_ID && rcvFunCode == FUNC_WR_MULTREG)     //备用机同步时间
    {
        return retIndex;
    }
    if(rcvFunCode == FUNC_RD_MIX)   //混合帧
    {
        printfs(LOG_INFO, "暂不支持监听复合帧的解析");
        return retIndex;
    }
    if ((rcvFunCode != FUNC_RD_COIL) &&
            (rcvFunCode != FUNC_RD_INPUT) &&
            (rcvFunCode != FUNC_RD_HOLDREG) &&
            (rcvFunCode != FUNC_RD_INPUTREG) &&
            (rcvFunCode != FUNC_RD_MIX) ) {
        modbusPrint(LOG_INFO,"功能码:%d处于备用不解析类别", rcvFunCode);
        return retIndex;
    }
    
    if (rcvframe.BufLen != 8)       //请求帧只有8字节
        return retIndex;

    MBusRequest readRequest;
    for(uint16_t index = 0; index <  m_readRequest.size(); index ++)      //轮询请求列表
    {
        readRequest = m_readRequest[index];

        if (readRequest.deviceAddr != rcvId)
            continue;
        if (readRequest.funcode != rcvFunCode)
            continue;
        if (readRequest.startAddr != rcvStartAddr)
            continue;
        if (readRequest.regNum != rcvRegNum)
            continue;
        retIndex = index;
        modbusPrint(LOG_DEBUG, "监听匹配的请求帧索引号:%d", retIndex);
        m_reqFifo.clearAll();
        break;
    }
    return retIndex;
}
/*******************************************************************************
@ Function Name     : dealFramePre
@ Description       : 预处理
@ Input             :  
@ Output            : 
@ Return            : None;
*******************************************************************************/
bool ModBusProtocol::dealFramePre(MBusReqFifo  &reqcmd){
	if(reqcmd.reqType == rspReadReq){
		return true;
	}
	 return false;
}

