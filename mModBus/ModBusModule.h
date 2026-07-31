/*************************************************
  Copyright (C), 2012- , Beijing Togest Automation System Equitment Co.,Ltd
  File name:        ModBusModule.h
  Author:           李佳臻     
  Version:          1.0       
  Date:             2012-11-26
  Description:      该文件声明ModBus的网络通信处理
  Others:           
  Function List:    
    1. 
  History:         
    1. Date:        2012.11.26
       Author:      Li Jiazhen
       Modification:
*************************************************/

#ifndef _MODBUSMODULE_H_
#define _MODBUSMODULE_H_
#include "../RtuBaseClass.h"
#include "serial_module.h"
#include "ModBusProtocol.h"

#define FRAME_RECV_ERR_MAX  50                                                  //帧接收最大错误计数

//定义基址表
struct MBusBaseAddr
{
    BaseAddrOffset          yxBaseAddr;                                         //遥信基址
    BaseAddrOffset          ycBaseAddr;                                         //遥测基址
    BaseAddrOffset          ykBaseAddr;                                         //遥控基址
    BaseAddrOffset          ytBaseAddr;                                         //遥调基址
    BaseAddrOffset          cirBaseAddr;                                        //回路基址
};

//接收出错处理
struct ModBusRecvErr
{
    uint16_t  recvErrCount;                                                       //接收出错计数
    uint16_t  recvErrMaxCount;                                                    //接收出错最大计数
    uint16_t  continueResetCount;                                                 //连续复位串口次数
    bool    enableResetSys;                                                     //使能复位系统
};

class ModBusModule : public RtuBaseClass
{
public:
    ModBusModule(const std::string &confFileName);
    ~ModBusModule();

    virtual void loadConfig();
    virtual void run();
    virtual BaseDataConfig getConfig();
    virtual void setBaseAddr(const BaseDataConfig &baseAddr);
    virtual void initModule();
	virtual bool getNodeVersion(const ModbusNodeType type, const uint8_t cpuNum, NodeInfo &info);
    virtual bool getNodeInfoCh(NodeInfo &infoSoe) {return d_protocol->getNodeInfoCh(infoSoe);}
    virtual bool getDoubleYxSoe(DataSoe &soe);                                  //获取双点遥信
    virtual bool getSingleYxSoe(DataSoe &soe);                                  //获取单点遥信SOE
    virtual bool getValidYcSoe(DataSoe &soe);                                   //获取遥测SOE
    virtual bool getFP32YcSoe(DataFP32Soe &soe);                                //获取32位遥测SOE
    virtual bool getDoubleYxData(const uint16_t addr, 
                                 uint8_t &value, bool change = false);            //获取双点遥信
    virtual bool getSingleYxData(const uint16_t addr, 
                                 uint8_t &value, bool change = false);            //获取单点遥信
    virtual bool getValidYcData(const uint16_t addr, 
                                uint16_t &value, bool change = false);            //获取遥测值
    virtual bool getFP32YcData(const uint16_t addr, 
                                float &value, bool change = false);              //获取遥测值
    virtual bool getValidYmData(const uint16_t addr, 
                                int32_t &value, bool change = false);            //获取遥测值 

    virtual bool readEvent(SpontEvent &event);                                  //读取事件信息

    virtual bool setSingleYkCmd(const SpontEvent &event);                       //单点遥控
    virtual bool setDoubleYkCmd(const SpontEvent &event);                       //双点遥控
    virtual bool setYcParam(const SpontEvent &event);                           //设参
    virtual SpontEvent setYt(const SpontEvent &event);                          //遥调
    virtual void setFileManage(FileManage *manage, RtuFileType type);           //录波文件写入
	virtual bool setUpdate(const SpontEvent &event);                            //在线升级
    virtual bool setDBRecord(const SpontEvent &event);                       //招取单板事件记录

	virtual bool setDataPoll(const bool setStop, bool notify=true);
    
protected:
    void   initConfig();
    static bool recvFrame(const int serialNo, const int serialFd, SerialNetBuf &frame);
    static bool needResetSerial(int serialNo);
    
protected:
    SerialProcess   m_processConf;
    SerialModule    m_serialModule;
    ModBusProtocol *d_protocol;
    MBusBaseAddr    m_baseAddrOffset;
    static ModBusRecvErr m_recvErr[TYPE_UART_NUM];                                  //串口接收错误处理
    uint8_t           m_serialNo;                                                 //串口接收号
    bool            m_resetConfExsit;                                           
};

#endif


