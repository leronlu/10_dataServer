#include <unistd.h>
using namespace std;
/************************************************************
  Copyright (C), Beijing Togest Automation System Equitment Co.,Ltd
  FileName:     ModBusModule.cpp
  Author:       李佳臻
  Version :     1.0
  Date:         2012-11-29
  Description:  Ups网络通信管理     
  Version:      1.0
  Function List:   
    1. 
  History:         
      <author>  <time>       <version >   <desc>
      李佳臻    2012/11/29     1.0        修改  
*************************************************************/
#include "ModBusModule.h"

ModBusRecvErr ModBusModule::m_recvErr[TYPE_UART_NUM] = {{0, FRAME_RECV_ERR_MAX, 0, false}};

/*******************************************************************************
@ Function Name     : ModBusModule
@ Description       : 构造函数
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
ModBusModule::ModBusModule(const string &confFileName) : m_baseAddrOffset()
{
    d_protocol = NULL;
    m_processConf.configFileName = confFileName;
    m_processConf.d_protocol = NULL;
    m_processConf.recvFrame  = NULL;
	//d_doubleBackup = NULL;
	m_resetConfExsit = false;
	m_serialNo = -1;
}

/*******************************************************************************
@ Function Name     : ModBusModule
@ Description       : 构造函数
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
ModBusModule::~ModBusModule()
{
    delete d_protocol;
}

/*******************************************************************************
@ Function Name     : getConfig
@ Description       : 获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusModule::loadConfig()
{
    d_protocol                   = new ModBusProtocol(m_processConf.configFileName, m_baseAddrOffset.cirBaseAddr.baseAddr);

    m_processConf.invokModule    = "ModBusModule";
    m_processConf.d_protocol     = d_protocol;
    m_processConf.recvFrame      = NULL;
    m_processConf.needResetSerial= needResetSerial;

    initConfig();
    d_protocol->loadConfig();
    setConfig(d_protocol->getConfig());

	if (m_recvErr[m_serialNo].enableResetSys && !m_resetConfExsit) {
		if (d_protocol->getConfig().modbusDeviceNum < 6) {
			m_recvErr[m_serialNo].enableResetSys = false;
		}
	}
    m_recvErr[m_serialNo].recvErrMaxCount = d_protocol->getConfig().modbusDeviceNum * 
            (3);                               //设置连续接收出错的最大计数
    d_protocol->setRecvErrPointer(&m_recvErr[m_serialNo].recvErrCount, &m_recvErr[m_serialNo].continueResetCount);
}

/*******************************************************************************
@ Function Name     : initConfig
@ Description       : 获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusModule::initConfig()
{
    XmlNodeParser f_XmlNodeParser((int8_t *)m_processConf.configFileName.c_str(), 
                                    (int8_t *)"/ModBusMasterConfig/Serial");
    int32_t value        = 0;
    //int8_t  nodePath[120] = "";
    int8_t  text[20]     = "";

    f_XmlNodeParser.GetChildContent((int8_t *)"Port", value);
    m_serialNo = value;

    m_recvErr[m_serialNo].enableResetSys = true;
    if (f_XmlNodeParser.GetChildContent((int8_t *)"resetEnable", text)) {
        m_recvErr[m_serialNo].enableResetSys = f_XmlNodeParser.StrToBoolean(text);
		m_resetConfExsit = true;
    }
        
    f_XmlNodeParser.FindNode((int8_t *)"//BaseAddr");
    if (f_XmlNodeParser.GetChildContent((int8_t *)"YxBaseAddr", value))
    {
        m_baseAddrOffset.yxBaseAddr.baseAddr = value;
        f_XmlNodeParser.GetChildProperty((int8_t *)"YxBaseAddr", (int8_t *)"type", text);
        m_baseAddrOffset.yxBaseAddr.type = (!strcmp((char *)text, "relate")) 
                                        ? BASEADDR_TYPE_RELATE : BASEADDR_TYPE_ABSOLUTE;
    }
    if (f_XmlNodeParser.GetChildContent((int8_t *)"YcBaseAddr", value))
    {
        m_baseAddrOffset.ycBaseAddr.baseAddr = value;
        f_XmlNodeParser.GetChildProperty((int8_t *)"YcBaseAddr", (int8_t *)"type", text);
        m_baseAddrOffset.ycBaseAddr.type = (!strcmp((char *)text, "relate")) 
                                        ? BASEADDR_TYPE_RELATE : BASEADDR_TYPE_ABSOLUTE;
    }
    if (f_XmlNodeParser.GetChildContent((int8_t *)"YkBaseAddr", value))
    {
        m_baseAddrOffset.ykBaseAddr.baseAddr = value;
        f_XmlNodeParser.GetChildProperty((int8_t *)"YkBaseAddr", (int8_t *)"type", text);
        m_baseAddrOffset.ykBaseAddr.type = (!strcmp((char *)text, "relate")) 
                                        ? BASEADDR_TYPE_RELATE : BASEADDR_TYPE_ABSOLUTE;
    }
    if (f_XmlNodeParser.GetChildContent((int8_t *)"YtBaseAddr", value))
    {
        m_baseAddrOffset.ytBaseAddr.baseAddr = value;
        f_XmlNodeParser.GetChildProperty((int8_t *)"YtBaseAddr", (int8_t *)"type", text);
        m_baseAddrOffset.ytBaseAddr.type = (!strcmp((char *)text, "relate")) 
                                        ? BASEADDR_TYPE_RELATE : BASEADDR_TYPE_ABSOLUTE;
    }
}

/*******************************************************************************
@ Function Name     : setBaseAddr
@ Description       : 设置基址
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusModule::setBaseAddr(const BaseDataConfig &baseAddr) 
{    
    m_baseAddr = baseAddr;

    if (m_baseAddrOffset.yxBaseAddr.type == BASEADDR_TYPE_RELATE)
    {
        m_baseAddr.singleYxnum += m_baseAddrOffset.yxBaseAddr.baseAddr;
    }
    else
    {
        m_baseAddr.singleYxnum = m_baseAddrOffset.yxBaseAddr.baseAddr;
    }
    if (m_baseAddrOffset.ykBaseAddr.type == BASEADDR_TYPE_RELATE)
    {
        m_baseAddr.doubleYknum += m_baseAddrOffset.ykBaseAddr.baseAddr;
    }
    else
    {
        m_baseAddr.doubleYknum = m_baseAddrOffset.ykBaseAddr.baseAddr;
    }
    if (m_baseAddrOffset.ycBaseAddr.type == BASEADDR_TYPE_RELATE)
    {
        m_baseAddr.validYcnum += m_baseAddrOffset.ycBaseAddr.baseAddr;
    }
    else
    {
        m_baseAddr.validYcnum = m_baseAddrOffset.ycBaseAddr.baseAddr;
    }
    if (m_baseAddrOffset.ytBaseAddr.type == BASEADDR_TYPE_RELATE)
    {
        m_baseAddr.validYtNum += m_baseAddrOffset.ytBaseAddr.baseAddr;
    }
    else
    {
        m_baseAddr.validYtNum = m_baseAddrOffset.ytBaseAddr.baseAddr;
    }
	
	if (m_baseAddrOffset.cirBaseAddr.type == BASEADDR_TYPE_RELATE)
    {
        m_baseAddr.validYcCircuitNum += m_baseAddrOffset.cirBaseAddr.baseAddr;
    }
    else
    {
        m_baseAddr.validYcCircuitNum = m_baseAddrOffset.cirBaseAddr.baseAddr;
    }

    d_protocol->setBaseCircuitAddr(m_baseAddr.validYcCircuitNum);
}

/*******************************************************************************
@ Function Name     : getConfig
@ Description       : 获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
BaseDataConfig ModBusModule::getConfig() 
{
    return m_config;
}

/*******************************************************************************
@ Function Name     : initModule
@ Description       : 初始化模块
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusModule::initModule()
{

}
/*******************************************************************************
@ Function Name     : run
@ Description       : 启动server服务线程
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusModule::run()
{
    m_serialModule.setConfig(m_processConf);
    m_serialModule.run();
}

/*******************************************************************************
@ Function Name     : recvFrame
@ Description       : 静态成员函数,帧接收函数,协议处理方法
@ Input             : serialFd:设备文件描述字 frame:接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusModule::recvFrame(const int serialNo, const int serialFd, SerialNetBuf &frame)
{
    int    n   = 0;

	bzero(&frame, sizeof(frame));
    usleep(1000);
    
	n = read(serialFd, frame.Buf, SERIAL_BUFF_LEN);
    
	if (n < 0)
	{
		printfs(LOG_WARNING, "net error:%d\n", errno);
		
		return false;
	}
    frame.BufLen += n;
	
    printfsBuf(LOG_DEBUG, "ModBusModule_%d Recv: \t", frame.Buf, frame.BufLen, serialNo);
 	if (frame.BufLen > 0)
		return true;
	
    return false;
}

/*******************************************************************************
@ Function Name     : needResetSerial
@ Description       : 静态成员函数,串口复位函数
@ Input             : serialFd:设备文件描述字 frame:接收帧
@ Output            : None;
@ Return            : None;
*******************************************************************************/
bool ModBusModule::needResetSerial(int serialNo)
{
    if (m_recvErr[serialNo].recvErrCount >= m_recvErr[serialNo].recvErrMaxCount) {
        m_recvErr[serialNo].recvErrCount = 0;

        if (++m_recvErr[serialNo].continueResetCount >= 3 && m_recvErr[serialNo].enableResetSys) {
            printfs(LOG_WARNING,  
                    "串口%d接收异常复位系统...", serialNo);
            sleep(2);
            if (execl("/sbin/reboot", "reboot", static_cast<char *>(0)) == -1)
            {
                printfs(LOG_WARNING,  
                    "复位进程失败:%d!", errno);
            }
        }
        
        return true;
    }

    return false;
}

/*******************************************************************************
@ Function Name     : getNodeVersion
@ Description       : 读取节点的软件版本
@ Input             : type:  节点类型
                      cpuNum:节点序号
@ Output            : value 遥测值
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::getNodeVersion(const ModbusNodeType type, const uint8_t cpuNum, NodeInfo &info)
{
	return d_protocol->getNodeVersion(type, cpuNum, info);
}

/*******************************************************************************
@ Function Name     : getDoubleYxSoe
@ Description       : 读取遥信SOE
@ Input             : d_protocol->getDoubleYxSoe
@ Output            : soe
@ Return            : true  有SOE
                      false 无SOE
*******************************************************************************/
bool ModBusModule::getDoubleYxSoe(DataSoe &soe)
{
	return d_protocol->getDoubleYxSoe(soe);
}

/*******************************************************************************
@ Function Name     : getSingleYxSoe
@ Description       : 读取遥信SOE
@ Input             : d_protocol->getDoubleYxSoe
@ Output            : soe
@ Return            : true  有SOE
                      false 无SOE
*******************************************************************************/
bool ModBusModule::getSingleYxSoe(DataSoe &soe)
{
	return d_protocol->getSingleYxSoe(soe);
}

/*******************************************************************************
@ Function Name     : getValidYcSoe
@ Description       : 读取遥测SOE
@ Input             : d_protocol->getValidYcSoe
@ Output            : soe
@ Return            : true  有SOE
                      false 无SOE
*******************************************************************************/
bool ModBusModule::getValidYcSoe(DataSoe &soe)
{
    return d_protocol->getValidYcSoe(soe);
}

/*******************************************************************************
@ Function Name     : getFP32YcSoe
@ Description       : 读取遥测SOE
@ Input             : d_protocol->getValidYcSoe
@ Output            : soe
@ Return            : true  有SOE
                      false 无SOE
*******************************************************************************/
bool ModBusModule::getFP32YcSoe(DataFP32Soe &soe)
{
    return d_protocol->getFP32YcSoe(soe);
}

/*******************************************************************************
@ Function Name     : getDoubleYxData
@ Description       : 读取遥信值
@ Input             : d_protocol->getDoubleYxData
@ Output            : value 遥信状态
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::getDoubleYxData(const uint16_t addr, uint8_t &value, bool change)
{
    return d_protocol->getDoubleYxData(addr, value, change);
}

/*******************************************************************************
@ Function Name     : getSingleYxData
@ Description       : 读取遥信值
@ Input             : d_protocol->getSingleYxData
@ Output            : value 遥信状态
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::getSingleYxData(const uint16_t addr, uint8_t &value, bool change)
{
    return d_protocol->getSingleYxData(addr, value, change);
}

/*******************************************************************************
@ Function Name     : getValidYcData
@ Description       : 读取遥测值
@ Input             : d_protocol->getValidYcData
@ Output            : value 遥测值
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::getValidYcData(const uint16_t addr, uint16_t &value, bool change)
{
    return d_protocol->getValidYcData(addr, value, change);
}

/*******************************************************************************
@ Function Name     : getFP32YcData
@ Description       : 读取遥测值
@ Input             : d_protocol->getValidYcData
@ Output            : value 遥测值
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::getFP32YcData(const uint16_t addr, 
                                float &value, bool change)
{
    return d_protocol->getFP32YcData(addr, value, change);
}

/*******************************************************************************
@ Function Name     : getValidYmData
@ Description       : 读取遥脉值
@ Input             : d_protocol->getValidYmData
@ Output            : value 遥脉值
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::getValidYmData(const uint16_t addr, int32_t &value, bool change)
{
    return d_protocol->getValidYmData(addr, value, change);
}

/*******************************************************************************
@ Function Name     : readEvent
@ Description       : 读取事件列表
@ Input             : d_protocol->readEvent
@ Output            : value 遥测值
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::readEvent(SpontEvent &event)
{
    return d_protocol->getEvent(event);
}

/*******************************************************************************
@ Function Name     : setSingleYkCmd
@ Description       : 单点遥控
@ Input             : event
@ Output            : 
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::setSingleYkCmd(const SpontEvent &event)
{
    return d_protocol->setSingleYkCmd(event);
}

/*******************************************************************************
@ Function Name     : setDoubleYkCmd
@ Description       : 双点遥控
@ Input             : event
@ Output            : 
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::setDoubleYkCmd(const SpontEvent &event)
{
    return d_protocol->setDoubleYkCmd(event);
}

/*******************************************************************************
@ Function Name     : setYcParam
@ Description       : 遥测参数设置
@ Input             : event
@ Output            : 
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::setYcParam(const SpontEvent &event)
{
    return d_protocol->setYcParam(event);
}

/*******************************************************************************
@ Function Name     : setYt
@ Description       : 遥测参数设置
@ Input             : event
@ Output            : 
@ Return            : true  正确
                      false 出错
*******************************************************************************/
SpontEvent ModBusModule::setYt(const SpontEvent &event)
{
    return d_protocol->setParam(event);
}

/*******************************************************************************
@ Function Name     : setFileManage
@ Description       : 设置相应的文件管理
@ Input             : manage: 文件管理类指针
                      type:   文件类型
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void ModBusModule::setFileManage(FileManage *manage, RtuFileType type)
{
	d_protocol->setFileManage(manage,type);
}
/*******************************************************************************
@ Function Name     : setUpdate
@ Description       : 在线升级
@ Input             : event:  事件
@ Output            : 
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::setUpdate(const SpontEvent &event)
{
	DBNodeType type = (DBNodeType)(event.data.data[0]);
	if (type == MSetSerialSpeed) {
		uint32_t speed[] = {9600, 115200};
		if (event.data.data[2] >= sizeof(speed)/sizeof(uint32_t))
			return false;
		m_serialModule.setUartSpeed(speed[event.data.data[2]]);
		printfs(LOG_WARNING, "ModBusModule_%d 设置波特率: %d", m_serialNo, speed[event.data.data[2]]);
		return false;
	}
	else {
    	return d_protocol->setUpdate(event);
	}
}

/*******************************************************************************
@ Function Name     : setDataPoll
@ Description       : 在线升级
@ Input             : event:  事件
@ Output            : 
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::setDataPoll(const bool setStop, bool notify)
{
    return d_protocol->setDataPoll(setStop, notify);
}

/*******************************************************************************
@ Function Name     : setDBRecord
@ Description       : 招取单板事件记录
@ Input             : event:  事件
@ Output            : 
@ Return            : true  正确
                      false 出错
*******************************************************************************/
bool ModBusModule::setDBRecord(const SpontEvent &event)
{
    return d_protocol->ReadRecord(event);
}
