#include <unistd.h>
#include <sys/syscall.h>
using namespace std;
/************************************************************
  Copyright (C), Beijing Togest Automation System Equitment Co.,Ltd
  FileName:     DownSidDataModule.cpp
  Author:       李佳臻
  Version :     1.0
  Date:         2012-06-26
  Description:  下行数据管理模块,处理由其他模块发送的消息,对消息解析调用相应的子模块,
                将子模块数据封装为消息发送给所需的模块
                子模块包括CAN1,CAN2
  Version:      1.0
  Function List:   
    1. 
  History:         
      <author>  <time>       <version >   <desc>
      李佳臻    2012/06/26     1.0        修改  
      李佳臻    2012/08/16     1.1        将消息处理从子模块中移转至该模块中处理,子模块
                                          只进行数据采集分析,不进行对其它模块的通信
      李佳臻    2014/06/16     1.2        使用STL库替换数组，修改配置文件，对每一个子模块
                                          添加配置文件路径,以达到可同时多个实例化同一类型的子模块
***********************************************************/

#include "DownSideDataModule.h"
#include <time.h>

/*******************************************************************************
@ Function Name     : DownSideDataModule
@ Description       : 下行数据管理模块构造函数
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
DownSideDataModule::DownSideDataModule() 
        : m_sendMsgFifo(256) 
{
    memset(m_moduleName, 0, sizeof(m_moduleName));
    m_timerYcPoll.SetTimer(1);                                                  //遥测采集周期
    m_timerYcPoll.StartTimer();
    m_YmPoll.SetTimer(1);
    m_YmPoll.StartTimer();	
	m_nodeCommYxEnable = false;
	memset(m_IEC104CommStat, 0, sizeof(m_IEC104CommStat));

    d_fileManage = FileManageModule::getFileManageModule();
    d_manage     = d_fileManage->getFileManageList();
}

/*******************************************************************************
@ Function Name     : initModule
@ Description       : 初始化模块
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
DownSideDataModule::~DownSideDataModule()
{
    m_msgManage.unregisterClient(string(MD_NAME_DOWN), 0);                      //注销模块

    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin();
                                          it!=m_moduleList.end();
                                          it++)
    {
        delete *it;
    }
}                                         

/*******************************************************************************
@ Function Name     : initModule
@ Description       : 初始化模块
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::initModule()
{
    BaseDataConfig baseAddr;

    loadConfig();                                                               //读取配置文件

    sprintf(m_moduleName, "%s_%d", MD_NAME_DOWN, 0);
    initDirectZmqMsgManage(m_msgManage);
    m_msgManage.registerClient(string(MD_NAME_DOWN), 0);                        //向消息管理模块注册模块

    for (vector<DownSideConfig>::iterator it=m_confList.begin(); it!=m_confList.end(); it++)
    {
        RtuBaseClass *d_down = NULL;		

        if (!(it->moduleName.compare("AC")))
        {
            d_down = new MicrophoneModule(it->moduleConfigFile);
			
            printfs(LOG_INFO, "实例化MicrophoneModule模块");
        }
		else if (!(it->moduleName.compare("Modbus")))
        {
            d_down = new ModBusModule(it->moduleConfigFile);
			
            printfs(LOG_INFO, "实例化ModBusModule模块");
        }
        else if (!(it->moduleName.compare("Minmea")))
        {
            d_down = new MinmeaModule(it->moduleConfigFile);
			
            printfs(LOG_INFO, "实例化MinmeaModule模块");
        }
        else if (d_down == NULL)
        {
            printfs(LOG_WARNING, "无相应的模块:%s", it->moduleName.c_str());
            continue ;
        }

        d_down->setModuleInfo(it->moduleName, 0);
        d_down->loadConfig();
        d_down->setBaseAddr(baseAddr);                                          //基址即为上一个模块的config
        
        it->config      = d_down->getConfig();
        it->baseAddr    = d_down->getBaseAddr();
        
        baseAddr += it->config;		

        d_down->setFileManage(d_manage->at(RtuFileTypeWaveFile)._pFileManage, 
                              RtuFileTypeWaveFile);
        d_down->setFileManage(d_manage->at(RtuFileTypeWebWaveFile)._pFileManage, 
                              RtuFileTypeWebWaveFile);

        d_down->run();
        d_down->initModule();

        m_moduleList.push_back(d_down);

        printfs(LOG_INFO, "模块名称:%s, 基址: 双点遥信:%d, 单点遥信:%d, 双点遥控:%d,"
                          "单点遥控:%d, 遥测量:%d, 参数量:%d, 遥脉量:%d, 短浮点:%d, 有效回路数:%d",
                          it->moduleName.c_str(),
                          it->baseAddr.doubleYxnum,
                          it->baseAddr.singleYxnum,
                          it->baseAddr.doubleYknum,
                          it->baseAddr.singleYknum,
                          it->baseAddr.validYcnum,
                          it->baseAddr.validYtNum,
                          it->baseAddr.validYmnum,
                          it->baseAddr.B32YcNum,
                          it->baseAddr.validYcCircuitNum);
        printfs(LOG_INFO, "模块名称:%s, 数量: 双点遥信:%d, 单点遥信:%d, 双点遥控:%d,"
                          "单点遥控:%d, 遥测量:%d, 参数量:%d, 遥脉量:%d, 短浮点:%d, 有效回路数:%d",
                          it->moduleName.c_str(),
                          it->config.doubleYxnum,
                          it->config.singleYxnum,
                          it->config.doubleYknum,
                          it->config.singleYknum,
                          it->config.validYcnum,
                          it->config.validYtNum,
                          it->config.validYmnum,
                          it->config.B32YcNum,
                          it->config.validYcCircuitNum);
    }	

}  

/*******************************************************************************
@ Function Name     : loadConfig
@ Description       : 获取配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::loadConfig()
{
    XmlNodeParser f_XmlNodeParser((int8_t *)DownSideConfigFile, 
                                  (int8_t *)"/DownSideDataConfig/ModuleInstance");
    int8_t  text[64] = "";
    uint8_t  childCnt = 0;
    int8_t  path[128] = "";

    printfs(LOG_INFO,  "加载下行数据管理模块配置...\n");

    m_nodeCommYxEnable = false;
    if (f_XmlNodeParser.FindNode((int8_t *)"//NodeCommunicationYx")) {
        f_XmlNodeParser.GetContent(text);
        m_nodeCommYxEnable = f_XmlNodeParser.StrToBoolean(text);
    }

    f_XmlNodeParser.FindNode((int8_t *)"/DownSideDataConfig/ModuleInstance");
    childCnt = f_XmlNodeParser.GetChildCounter("module");
    for (uint8_t i=0; i<childCnt; i++)
    {
        char name[32] = "";
        sprintf((char *)path, "/DownSideDataConfig/ModuleInstance/module[%d]", 
                                i+1);
        if (f_XmlNodeParser.FindNode(path))
        {
            f_XmlNodeParser.GetProperty((int8_t *)"enable", text);
            f_XmlNodeParser.GetProperty((int8_t *)"name", (int8_t*)name);
            if (f_XmlNodeParser.StrToBoolean(text))
            {
                DownSideConfig conf;
                
                conf.moduleName = string((char *)name);
                f_XmlNodeParser.GetProperty((int8_t *)"confFile", text);
                conf.moduleConfigFile = string((char *)text);

                m_confList.push_back(conf);
            }
            else if (!strcmp(name, "SoftVirtual") && m_nodeCommYxEnable) {
                DownSideConfig conf;
                
                conf.moduleName = string((char *)name);
                f_XmlNodeParser.GetProperty((int8_t *)"confFile", text);
                conf.moduleConfigFile = string((char *)text);
                m_confList.push_back(conf);
            }
        }
    }
	

}

/*******************************************************************************
@ Function Name     : run
@ Description       : 启动server服务线程
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::run()
{
    int ret = -1;
    
    ret = pthread_create(&m_serverPthread, NULL, processRoutine, static_cast<void *>(this));
    if (ret == 0)
    {
        printfsData(LOG_INFO, "\n");
        printfs(LOG_INFO,  "启动下行数据管理线程...\n");
    }
    else
    {
        printfs(LOG_ERROR,  "创建下行数据管理线程失败:%d!\n", ret);
        exit(1);
    }
}

/*******************************************************************************
@ Function Name     : getDataConfig
@ Description       : 获取下行子模块的数据量配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
BaseDataConfig DownSideDataModule::getDataConfig()
{
    BaseDataConfig dataConfig;

    for (vector<DownSideConfig>::iterator it=m_confList.begin(); it!=m_confList.end(); it++)
    {
        dataConfig += it->config;
    }

    return dataConfig;
}

/*******************************************************************************
@ Function Name     : getDownSideConfig
@ Description       : 获取下行子模块的数据量配置
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
vector<DownSideConfig> & DownSideDataModule::getDownSideConfig()
{
    return m_confList;
}

/*******************************************************************************
@ Function Name     : processRoutine
@ Description       : 静态成员函数,下行数据处理管理模块
@ Input             : *arg: static_cast<this>
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void *DownSideDataModule::processRoutine(void *arg)
{
    DownSideDataModule  *_this = static_cast<DownSideDataModule *>(arg);

    DebugPrint("Downside serverthread");

	printfs(LOG_INFO, "DownSide 服务线程:LWPID:%d TID:%d\n", 
        syscall(SYS_gettid), pthread_self());
	
    while (true)
    {
        _this->processSpontEvent();
        _this->sendMsg();
        _this->processMsg();
        _this->processChildSpontEvent();
            
        usleep(2000);
    }

	return NULL;
}

/*******************************************************************************
@ Function Name     : processEquMsg
@ Description       : 下行数据处理其他模块传递的消息
@ Input             : None;
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::processMsg(void)
{
    MsgData recvMsg;

    if (m_msgManage.recvMsg(string(MD_NAME_DOWN), 0, recvMsg))
    {
        IecMessage iecMsg; parseMessage(recvMsg.data1.data(), recvMsg.data1.size(), iecMsg);
        MsgData  msg;
        
        printfsBuf(LOG_DEBUG, "------------>接收到消息\t:", recvMsg.data1.data(), recvMsg.data1.size());
        
        msg.srcModuleName = m_moduleName;
        msg.dstModuleName = recvMsg.srcModuleName;

        switch (iecMsg.typ)
        {
            case MSG_C_IC_NA_1:                                                 //全召
                {
					//callAll(msg);
                    callAll(msg, iecMsg);
                    break;
                }
            case MSG_C_CI_NA_1:
                {              
                    //printfs(LOG_INFO, "Web招遥脉");
                    callAllYm(msg, iecMsg);
                    break;
                }
            case MSG_C_SC_TA_1:                                                 //单点遥控
            case MSG_C_SC_NA_1:
                {
                    printfs(LOG_ERROR, "下行模块接收到来自:%s 模块的单点遥控命令",
                        recvMsg.srcModuleName.c_str());
                    setYkCmd(msg, iecMsg, yksingleTable);
                    break;
                }
            case MSG_C_DC_TA_1:                                                 //双点遥控
            case MSG_C_DC_NA_1:
                {
                    printfs(LOG_ERROR, "下行模块接收到来自:%s 模块的双点遥控命令",
                        recvMsg.srcModuleName.c_str());
                    setYkCmd(msg, iecMsg);
                    
                    break;
                }
            case MSG_C_RD_NA_1:                                                 //参数读取
            case MSG_P_ME_NB_1:                                                 //参数设置
            case MSG_P_ME_NB_2:
            {
                setYt(msg, iecMsg);
                break;
            }
            case MSG_C_RD_NC_1:                                                 //读取版本信息
            {
                if (iecMsg.cot == MSG_COT_SPONT) {                          //解析104协议通信状态
                    if (m_nodeCommYxEnable) {
                        //4CommStat(recvMsg);
                    }
                } else {
                    getNodeVersion(msg);
                }
                break;
            }
            case MSG_C_UP_NA_1:                                                 //在线升级
            {
                setNodeUpdate(msg, iecMsg);
                break;
            }
            case MSG_P_EP_NA_1: 
            {
                setGroupYt(recvMsg, msg); 
                break;
            }
            case MSG_M_UP_NA_1:                                             //Modbus单板在线升级
            {
                //setDBUpdate(recvMsg, frame);
                break;
            }
            case MSG_M_IC_NA_1:                                             //web端招单板事件记录
            {
                //getDBRecord(msg, frame);                            
                break;
            }
            case MSG_C_IC_NB_1:                                                 //分组全召
            {
                callPartAll(msg, iecMsg);
                break;
            }
            case MSG_C_MW_NA_1:                                                 //手动录波
            {
                setManaulWave(msg, iecMsg); 
                break;
            }
            case MSG_C_SV_NA_1: {
                setVirtualData(iecMsg);
                break;
            }
            case MSG_C_MD_NA_1: {
                //exeModuleCMD(recvMsg);
                break;
            }
			case MSG_M_EC_NA_1: {
				setDoman(msg, iecMsg);
				break;
			}
            default:
            {
                errMsg(msg);
                break;
            }
        }
    }
}

/*******************************************************************************
@ Function Name     : processChildSpontEvent
@ Description       : 下行数据处理子模块的突发事件如一级变位、二级SOE和其它需要
                      确认的命令
@ Input             : None;
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::processChildSpontEvent()
{
#if 0    
clock_t s, e;

s = clock();    
#endif
    MsgData  msg;
    
    msg.srcModuleName = m_moduleName;
    msg.dstModuleName = "";
    
    getSpontEvent();
      
    getSingleYxCh(msg);                                                         //一级数据
    getDoubleYxCh(msg);
    getSingleYxSoe(msg);                                                        //二级SOE
    getDoubleYxSoe(msg);

    if (m_timerYcPoll.CheckTimeOut())                                           //以固定时间轮询遥测
    {      
        getValidYcCh(msg);
        getValidYcDcCh(msg);
        getFP32YcCh(msg);
        getValidYcSoe(msg);
        getValidYcDcSoe(msg);
        getFP32YcSoe(msg);
        
        m_timerYcPoll.StartTimer();
    }
    if(m_YmPoll.CheckTimeOut())
    {        
        getValidYmData(msg, MSG_COT_SPONT);
       
        m_YmPoll.StartTimer();
    }

    getNodeInfoCh(msg);
#if 0    
e = clock();

double t = (double)(e - s) / CLOCKS_PER_SEC;    
printf("----->>>s:%ld e:%ld t:%f \n", s, e, t);
#endif
}

/*******************************************************************************
@ Function Name     : processSpontEvent
@ Description       : 模块本身的突发事件
@ Input             : None;
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::processSpontEvent()
{
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); it!=m_moduleList.end(); it++)
    {
        (*it)->setDataPoll(false, false);
    }	
}

/*******************************************************************************
@ Function Name     : errMsg
@ Description       : 错误处理
@ Input             : None;
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::errMsg(MsgData &msg)
{

}

/*******************************************************************************
@ Function Name     : sendMsg
@ Description       : 向消息管理模块发送推入消息
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::sendMsg()
{
    MsgData queuedMsg;

    if (m_sendMsgFifo.popFront(queuedMsg)) {
        MsgData sendMsg;
        sendMsg.data1 = queuedMsg.data1;

        sendMsg.srcModuleName = m_moduleName;
        sendMsg.dstNodeIp = m_msgManage.getThisNodeIp();

        m_msgManage.sendMsg(sendMsg);
        printfsBuf(LOG_DEBUG, "<------------发送消息至%s %s\t:", sendMsg.data1.data(), sendMsg.data1.size(), 
            sendMsg.dstNodeIp.c_str(), sendMsg.dstModuleName.c_str());
    }
}
 
/*******************************************************************************
@ Function Name     : getTableNo
@ Description       : 向消息管理模块发送推入消息
@ Input             : addr   消息信息体地址
                      type   消息帧类型标识
@ Output            : None;
@ Return            : uint8_t  子模块序号 
*******************************************************************************/
int8_t DownSideDataModule::getTableNo(uint16_t addr, TableType type)
{    
    int8_t idx = 0;
    
    for (vector<DownSideConfig>::iterator it=m_confList.begin(); it!=m_confList.end(); it++, idx++)
    {
        switch (type) {
        case ykTable: {
            if ((addr >= it->baseAddr.doubleYknum)
                && (addr < it->baseAddr.doubleYknum + it->config.doubleYknum))
            {
                return idx;
            }
            break;
        }
        case paramTable: {
            if ((addr >= it->baseAddr.validYtNum)
                && (addr <  it->baseAddr.validYtNum + it->config.validYtNum))
            {
                return idx;
            }
            break;
        }
        case paramGroupTable: {
            if ((it->baseAddr.validYtNum/2 <= addr)
                && ((addr - it->baseAddr.validYtNum/2)
                    < it->config.validYtNum/2))
            {
                return idx;
            }
            break;
        }
        case yksingleTable: {
            if ((addr >= it->baseAddr.singleYknum)
                && (addr < it->baseAddr.singleYknum + it->config.singleYknum))
            {
                return idx;
            }
            break;
        }
        case ycCircuitTable: {
            if ((addr >= it->baseAddr.validYcCircuitNum)
                && (addr < it->baseAddr.validYcCircuitNum + it->config.validYcCircuitNum))
            {
                return idx;
            }
            break;
        }
        case singleYxTable: {
            if ((addr >= it->baseAddr.singleYxnum)
                && (addr < it->baseAddr.singleYxnum + it->config.singleYxnum)) {
                return idx;
            }
            break;
        }
        case doubleYxTable: {
            if ((addr >= it->baseAddr.doubleYxnum)
                && (addr < it->baseAddr.doubleYxnum + it->config.doubleYxnum)) {
                return idx;
            }
            break;
        }
        case validYcTable: {
            if ((addr >= it->baseAddr.validYcnum)
                && (addr < it->baseAddr.validYcnum + it->config.validYcnum)) {
                return idx;
            }
            break;
        }
        default: break;
        }
    }

    return -1;
}

/*******************************************************************************
@ Function Name     : getTableNo
@ Description       : 向消息管理模块发送推入消息
@ Input             : cpuNum   节点地址
                      type     节点类型
@ Output            : None;
@ Return            : uint8_t  子模块序号 
*******************************************************************************/
int8_t DownSideDataModule::getTableNo(uint8_t cpuNum, canNodeType type)
{
    int8_t idx = 0;
    
    for (vector<DownSideConfig>::iterator it=m_confList.begin(); it!=m_confList.end(); it++, idx++)
    {
        switch (type)
        {
        case canNodeYx:
        {
            if ((cpuNum >= it->baseAddr.yxNodeNum)
                && ((cpuNum - it->baseAddr.yxNodeNum) < it->config.yxNodeNum))
            {
                return idx;
            }
            break;
        }
        case canNodeYk:
        {
            if ((cpuNum >= it->baseAddr.ykNodeNum)
                && ((cpuNum - it->baseAddr.ykNodeNum) < it->config.ykNodeNum))
            {
                return idx;
            }
            break;
        }
        case canNodeYc:
        {
            if ((cpuNum >= it->baseAddr.ycNodeNum)
                && ((cpuNum - it->baseAddr.ycNodeNum) < it->config.ycNodeNum))
            {
                return idx;
            }
            break;
        }
        case canNodeGx:
        {
            if ((cpuNum >= it->baseAddr.gxNodeNum)
                && ((cpuNum - it->baseAddr.gxNodeNum) < it->config.gxNodeNum))
            {
                return idx;
            }
            break;
        }
        default: break;
        }
    }

    return -1;
}

/*******************************************************************************
@ Function Name     : callAll
@ Description       : 全召
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAll(MsgData &msg)
{
    callAllStart(msg);
    callSingleYx(msg);
    callDoubleYx(msg);
    callYc(msg);
    callFP32Yc(msg);
    callYcDc(msg);
	callYt(msg);
    callAllEnd(msg);
}

/*******************************************************************************
@ Function Name     : callAll
@ Description       : 全召
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAll(MsgData &msg, const IecMessage &iecMsg)
{
    uint8_t QOI = 0;
    
    if (!iecMsg.units.empty()) QOI = iecMsg.units[0].value[0];
    callAllStart(msg, QOI);

    switch (QOI)
    {
        case 20:    //总召组
        {
            callSingleYx(msg);
            callDoubleYx(msg);
            callYc(msg);
            callYcDc(msg);
            callFP32Yc(msg);
			callYt(msg);
            break;
        }
        case 1:     //遥信组
        {
            callSingleYx(msg);
            callDoubleYx(msg);
            break;
        }
        case 2:     //遥测组
        {
            callYc(msg);
            callYcDc(msg);
            break;
        }
        case 3:     //浮点
        {
            callFP32Yc(msg);
            break;
        }
		case 5:     //
        {
            break;
        }
        default : 
        {
            if (QOI >= 0x64)
            {
                for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
                     it!=m_moduleList.end(); 
                     it++)
                {
                    (*it)->callAll(QOI);
                }
            }
            break;
        }
    }
    callAllEnd(msg, QOI);
}

/*******************************************************************************
@ Function Name     : callAllStart
@ Description       : 全召确认
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAllStart(MsgData &msg, uint8_t QOI)
{
    IecMessage iecMsg;
    MsgData  smsg = msg;

    iecMsg.typ = MSG_C_IC_NA_1;
    iecMsg.cot = MSG_COT_ACTCON;
    iecMsg.commAddr = 1;
    iecMsg.units.push_back({0, {QOI}});

    { auto _p = packMessage(iecMsg); smsg.data1 = std::move(_p); }
    
    sendMsg(smsg);
}

/*******************************************************************************
@ Function Name     : callAllStart
@ Description       : 全召确认
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAllStart(MsgData &msg)
{
    IecMessage iecMsg;
    uint8_t    QOI = 0;

    QOI = 20;
    iecMsg.typ = MSG_C_IC_NA_1;
    iecMsg.cot = MSG_COT_ACTCON;
    iecMsg.commAddr = 1;
    iecMsg.units.push_back({0, {QOI}});

    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    
    sendMsg(msg);
}

/*******************************************************************************
@ Function Name     : callAllEnd
@ Description       : 全召结束
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAllEnd(MsgData &msg, uint8_t QOI)
{
    IecMessage iecMsg;
    MsgData  smsg= msg;

    iecMsg.typ = MSG_C_IC_NA_1;
    iecMsg.cot = MSG_COT_ACTTERM;
    iecMsg.commAddr = 1;
    iecMsg.units.push_back({0, {QOI}});

    { auto _p = packMessage(iecMsg); smsg.data1 = std::move(_p); }
    sendMsg(smsg);
}

/*******************************************************************************
@ Function Name     : callAllEnd
@ Description       : 全召结束
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAllEnd(MsgData &msg)
{
    IecMessage iecMsg;
    uint8_t    QOI = 0;

    QOI = 20;
    iecMsg.typ = MSG_C_IC_NA_1;
    iecMsg.cot = MSG_COT_ACTTERM;
    iecMsg.commAddr = 1;
    iecMsg.units.push_back({0, {QOI}});

    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    sendMsg(msg);
}

/*******************************************************************************
@ Function Name     : callDoubleYx
@ Description       : 全召双点遥信
@ Input             : srcModuleName:目标模块名称
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callDoubleYx(MsgData &msg)
{
    //DebugPrint("全召双点遥信...\n");
    getDoubleYxData(msg, MSG_COT_INTROGEN);
}

/*******************************************************************************
@ Function Name     : callSingleYx
@ Description       : 全召单点点遥信
@ Input             : srcModuleName:目标模块名称
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callSingleYx(MsgData &msg)
{
    //DebugPrint("全召单点遥信...\n");
    getSingleYxData(msg, MSG_COT_INTROGEN);
}

/*******************************************************************************
@ Function Name     : callYc
@ Description       : 全召遥测
@ Input             : srcModuleName:目标模块名称
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callYc(MsgData &msg)
{
    //DebugPrint("全召遥测...\n");
    getValidYcData(msg, MSG_COT_INTROGEN);
}

/*******************************************************************************
@ Function Name     : callYcDc
@ Description       : 全召遥测直流
@ Input             : srcModuleName:目标模块名称
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callYcDc(MsgData &msg)
{
    //DebugPrint("全召遥测直流...\n");
    getValidYcDcData(msg, MSG_COT_INTROGEN);
}

/*******************************************************************************
@ Function Name     : callFP32Yc
@ Description       : 全召短浮点遥测
@ Input             : srcModuleName:目标模块名称
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callFP32Yc(MsgData &msg)
{
    getFP32YcData(msg, MSG_COT_INTROGEN);
}

/*******************************************************************************
@ Function Name     : callAllYm
@ Description       : 全召
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAllYm(MsgData &msg, const IecMessage &iecMsg)
{
    uint8_t QCC = 0;    
    uint8_t RQT = 0;
    uint8_t FRZ = 0;
    
    if (!iecMsg.units.empty()) QCC = iecMsg.units[0].value[0];    
    RQT = QCC & 0x3f;
    FRZ = (QCC & ~0x3f)>>6;
    switch(RQT)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        {
            break;  //未配置
        }
        case 5:     //总的请求计数量
        {
            switch(FRZ)
            {
                case 0:         //读命令
                {
                    callAllYmStart(msg);    //请求累计量确认
                    callYm(msg);
                    callAllYmEnd(msg);  //计数量请求终止
                    break;
                }
                case 1:     //冻结不带复位
                case 2:     //冻结带复位
                case 3:     //复位
                {
                        if(YmFrz(iecMsg))
                        {                            
                            YmFrzStart(msg, FRZ);
                            }
                    break;
                }
            }
            break;
        }
        default:
        {
            if(RQT > 32)    //专用，做遥信板的遥脉召唤
            {
                
            }
        }
    }
    
}

/*******************************************************************************
@ Function Name     : callAllYmStart
@ Description       : 全召确认
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAllYmStart(MsgData &msg)
{
    IecMessage iecMsg;
    uint8_t    QOI = 0;

    QOI = 5;
    iecMsg.typ = MSG_C_CI_NA_1;
    iecMsg.cot = MSG_COT_ACTCON;
    iecMsg.commAddr = 1;
    iecMsg.units.push_back({0, {QOI}});

    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    
    sendMsg(msg);
}

/*******************************************************************************
@ Function Name     : callAllEnd
@ Description       : 全召结束
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callAllYmEnd(MsgData &msg)
{
    IecMessage iecMsg;
    uint8_t    QOI = 0;

    QOI = 5;
    iecMsg.typ = MSG_C_CI_NA_1;
    iecMsg.cot = MSG_COT_ACTTERM;
    iecMsg.commAddr = 1;
    iecMsg.units.push_back({0, {QOI}});

    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
  
    sendMsg(msg);
}

/*******************************************************************************
@ Function Name     : callYm
@ Description       : 全召遥脉
@ Input             : srcModuleName:目标模块名称
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callYm(MsgData &msg)
{
    //DebugPrint("全召遥脉...\n");
    getValidYmData(msg, MSG_COT_REQCOGEN);
}

/*******************************************************************************
@ Function Name     : callYt
@ Description       : 全召遥调
@ Input             : srcModuleName:目标模块名称
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callYt(MsgData &msg)
{
    //DebugPrint("全召遥测...\n");
    getValidYtData(msg, MSG_COT_INTROGEN);
}


/*******************************************************************************
@ Function Name     : YmFrzStart
@ Description       : 遥脉冻结确认
@ Input             : None;
@ Output            : None;
@ Return            : None;
*******************************************************************************/

void DownSideDataModule::YmFrzStart(MsgData &msg, int8_t FRZ)
{
    IecMessage iecMsg;
    uint8_t    QCC = 5;

    QCC |= FRZ<<6;
    iecMsg.typ = MSG_C_CI_NA_1;
    iecMsg.cot = MSG_COT_ACTCON;
    iecMsg.commAddr = 1;
    iecMsg.units.push_back({0, {QCC}});

    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    
    sendMsg(msg);
}

/*******************************************************************************
@ Function Name     : YmFrz
@ Description       : 遥脉冻结
@ Input             : None;
@ Output            : None;
@ Return            : None;
*******************************************************************************/

bool DownSideDataModule::YmFrz(const IecMessage &iecMsg)
{
    uint8_t QCC = 0;    
    uint8_t RQT = 0;
    uint8_t FRZ = 0;
    uint16_t num = 0;
    
    if (!iecMsg.units.empty()) QCC = iecMsg.units[0].value[0];    
    RQT = QCC & 0x3f;
    FRZ = (QCC & ~0x3f)>>6;

    vector<DownSideConfig>::iterator confIt=m_confList.begin();
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
            it!=m_moduleList.end(); it++, confIt++)
    {
        num = confIt->config.validYmnum;
        if((num > 0) && !(*it)->FreezeYm(RQT, FRZ))
            return false;
    }

    return true;
}


/*******************************************************************************
@ Function Name     : callPartAll
@ Description       : 部分全召 该消息作为事件处理
@ Input             : srcModuleName:目标模块名称
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::callPartAll(MsgData &msg, const IecMessage &iecMsg)
{
    uint8_t  elemt[3] = {0};
    int8_t  tableId  = 0;
    SpontEvent      event;

    printfs(LOG_INFO, "指定范围全召...\n");

    if (!iecMsg.units.empty())
    {
        memcpy(elemt, iecMsg.units[0].value.data(), std::min(iecMsg.units[0].value.size(), sizeof(elemt)));
        event.srcModuleName = msg.dstModuleName;
        event.type = eventTypeICPart;
        event.cmd  = iecMsg.typ;
        event.addr = iecMsg.units[0].addr;
        event.state        = (EventState)iecMsg.cot;
        event.data.len     = 3;
        
        switch (elemt[0])
        {
            case canNodeYx: 
                {
                    tableId = getTableNo(elemt[1], (canNodeType)(elemt[0]));
                    if (tableId == -1)
                    {
                        event.state = eventNoInforAddr;
                        event.value = failed;
                        sendCallPartEventMsg(event, tableId);
                        return ;
                    }
                    elemt[1] -= m_confList[tableId].baseAddr.yxNodeNum;
                    break;
                }
            case canNodeYc: 
                {
                    tableId = getTableNo(elemt[2], (canNodeType)(elemt[0]));
                    if (tableId == -1)
                    {
                        event.state = eventNoInforAddr;
                        event.value = failed;
                        sendCallPartEventMsg(event, tableId);
                        return ;
                    }
                    elemt[2] -= m_confList[tableId].baseAddr.validYcnum/16;
                    break;
                }
            default:break;
        }
        memcpy(event.data.data, elemt, sizeof(elemt));
        m_moduleList[tableId]->callPart(event);
    }
}

/*******************************************************************************
@ Function Name     : setManaulWave
@ Description       : 手动录波
@ Input             : srcModuleName 源模块名称
                    : 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setManaulWave(MsgData &msg, const IecMessage &iecMsg)
{
    uint8_t  elemt = 0;
    int8_t  tableId  = 0;
    SpontEvent      event;

    printfs(LOG_INFO, "手动录波...\n");

    if (!iecMsg.units.empty())
    {
        memcpy(&elemt, iecMsg.units[0].value.data(), std::min(iecMsg.units[0].value.size(), sizeof(elemt)));
        event.srcModuleName = msg.dstModuleName;
        event.type = ycManualWaveEvent;
        event.cmd  = iecMsg.typ;
        event.addr = iecMsg.units[0].addr;
        event.state        = (EventState)iecMsg.cot;
        event.data.len     = 1;
        
        tableId = getTableNo(event.addr, ycCircuitTable);
        if (tableId == -1)
        {
            event.state = eventNoInforAddr;
            event.value = failed;
            sendManaulWaveEventMsg(event, tableId);
            return ;
        }

        event.data.data[0] = elemt;
        event.addr        -= m_confList[tableId].baseAddr.validYcCircuitNum;
        if (!m_moduleList[tableId]->manualWave(event))
        {
            event.state = eventActCon;
            event.value = failed;
            sendManaulWaveEventMsg(event, tableId); 
        }
    }
}

/*******************************************************************************
@ Function Name     : setYkCmd
@ Description       : 双点遥控
@ Input             : srcModuleName 源模块名称
                    : 
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setYkCmd(MsgData &msg, const IecMessage &iecMsg, TableType type)
{
    uint16_t addr = 0; 
    uint8_t  dco  = 0;
    int8_t  childNo = 0;
    SpontEvent event;

    if (iecMsg.units.empty())
        return;

    addr    = iecMsg.units[0].addr;
    childNo = getTableNo(addr, type);

    {
        memcpy(&dco, iecMsg.units[0].value.data(), std::min(iecMsg.units[0].value.size(), sizeof(dco)));
        event.srcModuleName = msg.dstModuleName;

        event.type = ykCmdEvent;
        event.cmd  = iecMsg.typ;
        event.addr = addr;
        event.state        = (EventState)iecMsg.cot;
        event.data.len     = 1;
        event.data.data[0] = dco;

        if (childNo == -1)
        {
            event.state = eventNoInforAddr;
            event.value = failed;
            printfs(LOG_ERROR, "%s的遥控命令地址%d超出范围!", msg.dstModuleName.c_str(), addr);
            sendYkEventMsg(event, childNo, type);
            return ;
        }		

        if (type == ykTable)
        {
            addr -= m_confList[childNo].baseAddr.doubleYknum;
            event.addr = addr;
            m_moduleList[childNo]->setDoubleYkCmd(event);
        }
        else if (type == yksingleTable)
        {
            addr -= m_confList[childNo].baseAddr.singleYknum;
            event.addr = addr;
            m_moduleList[childNo]->setSingleYkCmd(event);
        }
    }
}

/*******************************************************************************
@ Function Name     : setParam
@ Description       : 设置参数
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setParam(MsgData &msg, const IecMessage &iecMsg)
{
    uint16_t addr     = 0; 
    int8_t  childNo  = 0;
    uint8_t  elemt[3] = {0};
    SpontEvent      event;
    uint8_t  allvsq = iecMsg.units.size();
    uint8_t  seq = 0 /* SQ bit */;

    for (uint8_t vsq=0; vsq<allvsq; vsq++)
    {
        DebugPrint("设置参数...\n");
        addr    = (seq)?(iecMsg.units[vsq].addr+vsq) : iecMsg.units[vsq].addr;

        if (memcpy(elemt, iecMsg.units[vsq].value.data(), std::min(iecMsg.units[vsq].value.size(), sizeof(elemt))))
        {
            event.srcModuleName = msg.dstModuleName;
            event.type = ycSetParamEvent;
            event.cmd  = iecMsg.typ;
            event.addr = addr;
            event.state        = (EventState)iecMsg.cot;
            event.data.len     = 3;
            memcpy(event.data.data, elemt, sizeof(elemt));
            
            childNo = getTableNo(addr, paramTable);

            if (childNo == -1)
            {
                event.state = eventNoInforAddr;
                event.value = failed;
                sendSetParamEventMsg(event, childNo);
                return ;
            }
            
            addr -= m_confList[childNo].baseAddr.validYtNum;
            event.addr = addr;
            if (!m_confList[childNo].moduleName.compare("ModBus") ||
                        !m_confList[childNo].moduleName.compare("TunnelLight"))
            {
                m_moduleList[childNo]->setYcParam(event);
            }
        }
    }
}

/*******************************************************************************
@ Function Name     : setYt
@ Description       : 设置参数
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setYt(MsgData &msg, const IecMessage &iecMsg)
{
    uint16_t addr     = 0; 
    int8_t  childNo  = -1;
    uint8_t  elemt[3] = {0};
    SpontEvent      event;
    MsgData ytMsg;
    IecMessage ytFrame;  /* elemSize=3 */
    uint8_t  allvsq = iecMsg.units.size();
    uint8_t  isSQ   = 0 /* SQ bit */;

    ytMsg.dstModuleName = msg.dstModuleName;
    ytMsg.srcModuleName = string(MD_NAME_DOWN"_0");
    ytFrame.typ = iecMsg.typ;
    /* ytFrame VSQ: from units.size() */
    ytFrame.cot = event.state | event.value;
    ytFrame.commAddr = iecMsg.commAddr;
    if (isSQ)
    {
        addr = iecMsg.units[0].addr;
        ytFrame.units.push_back({addr, {}});
    }

    for (uint8_t vsq=0; vsq<allvsq; vsq++)
    {
        addr    = (isSQ) ? (iecMsg.units[vsq].addr+vsq) : iecMsg.units[vsq].addr;

        if (memcpy(elemt, iecMsg.units[vsq].value.data(), std::min(iecMsg.units[vsq].value.size(), sizeof(elemt))))
        {
            event.srcModuleName = msg.dstModuleName;
            if (elemt[2]>=80 && elemt[2]<125){
                event.type = eventTypeDBSetParam;
			}
			//else if(elemt[2]>=126 && elemt[2]<256){
			//	event.type = ycSetParamEvent;
			//}
            else{
                event.type = ycSetParamEvent;
			}                             
            event.cmd  = iecMsg.typ;
            event.addr = addr;
            event.state        = (EventState)iecMsg.cot;
            event.data.len     = 3;
            memcpy(event.data.data, elemt, sizeof(elemt));
			
            if ((canYcParamQpm)event.state == QPM_setID)
            {
                childNo = getTableNo(addr, canNodeYc);
                addr -= m_confList[childNo].baseAddr.ycNodeNum;
            }
            #if 1
            else if (elemt[2]>=80)
            {
            	printfs(LOG_DEBUG,"进入Modbus,QPM设参方式，QPM:%d",elemt[2]);
                int8_t idx = 0;
                for (vector<DownSideConfig>::iterator it=m_confList.begin(); it!=m_confList.end(); it++, idx++)
                {
                    if (!(it->moduleName.compare("AC")))
                    {
                    	if ((addr >= it->baseAddr.validYtNum) && (addr < (it->baseAddr.validYtNum+ it->config.validYtNum))){
							childNo = idx;
							addr -= it->baseAddr.validYtNum;
                            break;
						}
						else{
							printfs(LOG_INFO,"非AC地址，add:%d", addr);
						}
                    }
                }
            }
            #endif
            else
            {
                childNo = getTableNo(addr, paramTable);
                addr -= m_confList[childNo].baseAddr.validYtNum;
            }

            if (childNo == -1)
            {
                event.state = eventNoInforAddr;
                event.value = failed;
                ytFrame.cot = event.state | event.value;
                break;
            }
			printfs(LOG_DEBUG, "event.addr:%d  m_confList[%d].moduleName:%s ",event.addr,childNo, m_confList[childNo].moduleName.c_str());
            event.addr = addr;
            #if 0
            if (!m_confList[childNo].moduleName.compare("ModBus") || !m_confList[childNo].moduleName.compare("TunnelLight"))
            {
                m_moduleList[childNo]->setYcParam(event);
                continue ;
            }
            #endif
            event = m_moduleList[childNo]->setYt(event);
            if (event.availability == false) {
                continue;
            }
            ytFrame.cot = event.state | event.value;
            if (!isSQ)
                ytFrame.units.push_back({addr + m_confList[childNo].baseAddr.validYtNum, {}});
            memcpy(elemt, event.data.data, sizeof(elemt));
            ytFrame.units.back().value.assign(elemt, elemt + 3);
        }
    }
    if (ytFrame.cot == eventActCon)
        m_moduleList[childNo]->saveParam();
    
      { auto _p = packMessage(ytFrame); ytMsg.data1 = std::move(_p); }
    sendMsg(ytMsg);
}

/*******************************************************************************
@ Function Name     : setGroupYt
@ Description       : 设置参数区
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setGroupYt(const MsgData &msg, MsgData &ytMsg)
{
    uint16_t cir     = 0; 
    int8_t  childNo = 0;
    uint8_t  ytinf     = 0;
    uint8_t  ytngd     = 0;
    uint16_t ytsrcOffset = 0;
    uint16_t ytdstOffset = 0;
    uint8_t ytginGroup = 0;
    uint8_t ytginIndex = 0;
    uint8_t len = 0;
	int ngdGroup_ = 0;
	int ytGroupDataIdxGID0 = 0;
    SpontEvent      event;

    printfs(LOG_DEBUG, "inf:%#x ngd:%d srcoffset:%d dstoffset:%d gingroup:%d ginindex:%d this=%#08x",
            ytinf, ytngd, ytsrcOffset, ytdstOffset, ytginGroup, ytginIndex, this);
	printfsBuf(LOG_INFO,"msgData:",msg.data1.data(), msg.data1.size());
	
    ytinf = msg.data1[YtGroupDataIdxINF];
    ytngd = msg.data1[YtGroupDataIdxNGD] & 0x3F;
    //ytsrcOffset   = YtGroupDataIdxCirLow;
    //ytdstOffset   = YtGroupDataIdxCirLow;
    ytginGroup = msg.data1[YtGroupDataIdxGINGroup];
    ytginIndex = msg.data1[YtGroupDataIdxGINEntry];
	ytGroupDataIdxGID0 = msg.data1[YtGroupDataIdxGID0];
    
    ytMsg.dstModuleName = msg.srcModuleName;
    ytMsg.srcModuleName = string(MD_NAME_DOWN"_0");
    ytMsg.data1 = msg.data1;
    cir = msg.data1[YtGroupDataIdxCirLow] + (msg.data1[YtGroupDataIdxCirHig] << 8);
    
    childNo = getTableNo(cir, paramGroupTable);
    printfs(LOG_DEBUG, "childNo:%d cir:%d", childNo, cir);
    if (childNo == -1) {
        event.state = eventNoInforAddr;
        event.value = failed;
        ytMsg.data1[YtGroupDataIdxCOT] = event.state | event.value;

        sendMsg(ytMsg);
        return ;
    }
    
    cir    -= m_confList[childNo].baseAddr.validYtNum / 2;
	if (ytinf == YcGroupYtInfoReadValue) {
		if (ytGroupDataIdxGID0 != 0){
			ngdGroup_ = 1;
		}
		else{
			ngdGroup_ = NGDZoneNum;
		}
	}
	else if (ytinf == YcGroupYtInfoWriteValuePre || ytinf == YcGroupYtInfoWriteValueExe || ytinf == YcGroupYtInfoWriteValueAbort) {
		ngdGroup_ = 1;
	}

	for (uint8_t ngdGroup = 0; ngdGroup < ngdGroup_; ngdGroup ++)
	{
		ytsrcOffset   = YtGroupDataIdxCirLow;
   		ytdstOffset   = YtGroupDataIdxCirLow;
	    if (ytginGroup == 0 && ytginIndex == 1) {
	        len = 12;
	    }
	    else {
			len = 13;
	        if (ytinf == YcGroupYtInfoReadValue && ytginIndex == 0) {
	            ytngd = YcGroupYtValueIdxEnd;
				
	        }
			else if (ytinf == YcGroupYtInfoWriteValuePre || ytinf == YcGroupYtInfoWriteValueExe)
			{
				printfs(LOG_DEBUG, "ngdGroup_:%d  ngdGroup:%d  ytngd:%d gingroup:%d ginindex:%d len:%d", ngdGroup_, ngdGroup, ytngd, ytginGroup, ytginIndex, ytngd*8 + 5);
				//len = (msg.data1[YtGroupDataIdxNGD]&0x3f) * 8 + 5;
				len = msg.data1.size() - YtGroupDataIdxCirLow;
				ytngd = 1;
			}
	    }
		printfs(LOG_DEBUG, "ngdGroup_:%d  ngdGroup:%d  ytngd:%d gingroup:%d ginindex:%d len:%d", ngdGroup_, ngdGroup, ytngd, ytginGroup, ytginIndex,ytngd*8 + 5);
	    for (int i=0; i<ytngd; i++)
	    {
	        /*组合事件*/
	        event.srcModuleName = msg.dstModuleName;
	        event.type = ycSetParamEvent;
	        event.cmd  = msg.data1[YtGroupDataIdxType];
	        event.addr = cir;
	        event.state        = eventReq;
	        event.data.len     = len;
	        memcpy(event.data.data, &msg.data1[ytsrcOffset], len);
							
	        if (ytinf == YcGroupYtInfoReadValue && ytginIndex == 0) {
							//高两位为组号 低6位为区号 默认区号是2;
				event.data.data[11] = (((ngdGroup << 6)) | (0x02));
	            event.data.data[6] = i + YcGroupYtValueIdxIThv1;    
	        }
            #if 0
	        if (ytinf == YcGroupYtInfoWriteValuePre) {
	            ytsrcOffset += (i==0) ? len : (len - 5);
	        }
	        printfsBuf(LOG_DEBUG, "event--->dstoffset:%d", event.data.data, event.data.len, ytdstOffset);
            #endif
	        /*执行事件*/
	        event = m_moduleList[childNo]->setGroupYt(event);
	        
	        /*解析事件结果*/
	        ytMsg.data1[YtGroupDataIdxCOT] = event.state | event.value;

			if (ytinf == YcGroupYtInfoWriteValuePre || ytinf == YcGroupYtInfoWriteValueExe) {
				len = 12;
			}
			
	        if (i == 0) {
	            memcpy(&ytMsg.data1[ytdstOffset], &event.data.data[0], event.data.len);
	            ytdstOffset += event.data.len;
	        }
	        else {
	            memcpy(&ytMsg.data1[ytdstOffset], &event.data.data[5], event.data.len-5);
	            ytdstOffset += event.data.len - 5;
	        }
	          
	        ytMsg.data1.resize(ytdstOffset);
			//定值区号只召一次
			if (event.data.data[2] == YcGroupYtInfoReadValue && event.data.data[5] == 0 && event.data.data[6] == 1){
				sendMsg(ytMsg);

                return;
			}
	    }
		
	    if (ytinf == YcGroupYtInfoReadValue) {
			//ngd后续位选择
			if (ngdGroup == (NGDZoneNum - 1) || (ngdGroup_ == 1)){
				ytngd = ytngd & 0x7F;
			}
			else{
				ytngd = ytngd | 0x80;
			}
	        ytMsg.data1[YtGroupDataIdxNGD] = ytngd;
	    }
	    else{
	        ytMsg.data1[YtGroupDataIdxNGD] = 1;
			ytMsg.data1[YtGroupDataIdxGINGroup] = 1;
			ytMsg.data1[YtGroupDataIdxGINEntry] = 0;
			ytMsg.data1[YtGroupDataIdxKOD] = 0;
			ytMsg.data1[YtGroupDataIdxGID0] = 2;
	    }
	    
	    if (ytMsg.data1[YtGroupDataIdxCOT] == eventReq && ytinf == YcGroupYtInfoWriteValueExe)
	        m_moduleList[childNo]->saveParam();
	    
	    printfs(LOG_DEBUG, "inf:%#x ngd:%d srcoffset:%d dstoffset:%d gingroup:%d ginindex:%d this=%#08x",
	            ytinf, ytngd, ytsrcOffset, ytdstOffset, ytginGroup, ytginIndex, this);
		
	    if(ytinf == YcGroupYtInfoWriteValuePre && msg.data1[YtGroupDataIdxNGD]&0x80){
			printfs(LOG_INFO, "预置组后续位为真，非最后一组");
			//预置存在后续位不回复响应帧
		}
		else{
	    	sendMsg(ytMsg);
		}
		
	}
}

/*******************************************************************************
@ Function Name     : setNodeUpdate
@ Description       : 在线升级
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setNodeUpdate(MsgData &msg, const IecMessage &iecMsg)
{
    uint8_t  elemt[2] = {0};
    int8_t  tableId  = 0;
    SpontEvent      event;

    printfs(LOG_ERROR, "在线升级...\n");

    if (!iecMsg.units.empty())
    {
        memcpy(elemt, iecMsg.units[0].value.data(), std::min(iecMsg.units[0].value.size(), sizeof(elemt)));
        event.srcModuleName = msg.dstModuleName;
        event.type = eventTypeUpdate;
        event.cmd  = iecMsg.typ;
        event.addr = iecMsg.units[0].addr;
        event.state        = (EventState)iecMsg.cot;
        event.data.len     = 2;
        
        tableId = getTableNo(elemt[1], (canNodeType)(elemt[0]));
        if (tableId == -1)
        {
            event.state = eventNoInforAddr;
            event.value = failed;
            sendUpdateEventMsg(event, tableId);
            return ;
        }
        switch (elemt[0])
        {
            case canNodeYx: elemt[1] -= m_confList[tableId].baseAddr.yxNodeNum;break;
            case canNodeYk: elemt[1] -= m_confList[tableId].baseAddr.ykNodeNum;break;
            case canNodeYc: elemt[1] -= m_confList[tableId].baseAddr.ycNodeNum;break;
            case canNodeGx: elemt[1] -= m_confList[tableId].baseAddr.gxNodeNum;break;
            default:break;
        }
        memcpy(event.data.data, elemt, sizeof(elemt));
        if (!m_moduleList[tableId]->setUpdate(event))
        {
            event.state = eventActCon;
            event.value = failed;
            sendUpdateEventMsg(event, tableId);
        }
    }
}

/*******************************************************************************
@ Function Name     : setDoman
@ Description       : 单板日志读取
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setDoman(MsgData &msg, const IecMessage &iecMsg)
{
    uint8_t  elemt[5] = {0};  
    int8_t  tableId  = -1;    
    SpontEvent      event;

    if (!iecMsg.units.empty())
    {
        memcpy(elemt, iecMsg.units[0].value.data(), std::min(iecMsg.units[0].value.size(), sizeof(elemt)));
        int8_t  idx = 0;
        event.srcModuleName = msg.dstModuleName;    //上行线程来源
        event.type = eventTypeDBRecord;
        event.cmd  = iecMsg.typ;
        event.addr = iecMsg.units[0].addr;      //信息体地址
        event.state        = (EventState)iecMsg.cot;
        event.data.len     = 1;
        
        for (vector<DownSideConfig>::iterator it=m_confList.begin(); it!=m_confList.end(); it++, idx++)
        {
            if (!(it->moduleName.compare("Rise3501")))
            {
                tableId = idx;
                printfs(LOG_INFO, "Rise3501设置域名, idx = %d", idx);
                break;
            }
        }
        if (tableId == -1)
        {
            event.state = eventNoInforAddr;
            event.value = failed;
            //sendDBRecordEventMsg(event, tableId);
            return ;
        }

        memcpy(event.data.data, &elemt, sizeof(elemt));
        if (!m_moduleList[idx]->easyCmd(event))
        {
            printfs(LOG_INFO, "简易命令执行失败!");
            event.state = eventActCon;
            event.value = failed;
            //sendDBRecordEventMsg(event, tableId);
        }
    }
}



/*******************************************************************************
@ Function Name     : getDoubleYxSoe
@ Description       : 获取子模块双点遥信SOE,并广播给其它所需模块
@ Input             : 子模块
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getDoubleYxSoe(MsgData &msg)
{
    DataSoe  soe;
    IecMessage iecMsg;  /* elemSize=1 timeScale=7 */
    uint8_t    vsq = 0;

    iecMsg.typ = MSG_M_DP_TB_1;
    /* iecMsg VSQ: set by units.size() */
    iecMsg.cot = MSG_COT_SPONT;
    iecMsg.commAddr = 1;

    vector<DownSideConfig>::iterator confIt=m_confList.begin();
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
            it!=m_moduleList.end(); it++, confIt++)
    {
        while ((*it)->getDoubleYxSoe(soe))
        {
            iecMsg.units.push_back({soe.addr + confIt->baseAddr.doubleYxnum, {}});
            iecMsg.units.back().value.assign((uint8_t*)&soe.value, (uint8_t*)&soe.value + 1);
            iecMsg.units.back().time.assign((const char*)soe.dateTime, 7);

            if (++vsq >= YXSOE_PER_FRAME)
            {
                /* iecMsg VSQ = units.size() */
                { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                sendMsg(msg);

                vsq = 0;
				iecMsg.units.clear();
            }
            printfs(LOG_WARNING, "双点遥信SOE: 地址:%d 状态:%d %04d-%02d-%02d %02d:%02d:%02d.%03d",
                soe.addr + confIt->baseAddr.doubleYxnum + 1, soe.value, soe.dateTime[6]+2000, soe.dateTime[5], soe.dateTime[4]&0x1F,
                    soe.dateTime[3], soe.dateTime[2], (soe.dateTime[1]*256+soe.dateTime[0])/1000, 
                    (soe.dateTime[1]*256+soe.dateTime[0])%1000);
        }
    }
    if (vsq > 0)
    {
        /* iecMsg VSQ = units.size() */
        { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
        sendMsg(msg);
    }
}

/*******************************************************************************
@ Function Name     : getSingleYxSoe
@ Description       : 上报子模块的单点遥信SOE,并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getSingleYxSoe(MsgData &msg)
{
    DataSoe  soe;
    IecMessage iecMsg;  /* elemSize=1 timeScale=7 */
    uint8_t    vsq = 0;
	
    iecMsg.typ = MSG_M_SP_TB_1;
    /* iecMsg VSQ: set by units.size() */
    iecMsg.cot = MSG_COT_SPONT;
    iecMsg.commAddr = 1;
    
    vector<DownSideConfig>::iterator confIt=m_confList.begin();
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
            it!=m_moduleList.end(); it++, confIt++)
    {
        while ((*it)->getSingleYxSoe(soe))
        {
            iecMsg.units.push_back({soe.addr + confIt->baseAddr.singleYxnum, {}});
            iecMsg.units.back().value.assign((uint8_t*)&soe.value, (uint8_t*)&soe.value + 1);
            iecMsg.units.back().time.assign((const char*)soe.dateTime, 7);			

            if (++vsq >= YXSOE_PER_FRAME)
            {
                /* iecMsg VSQ = units.size() */
                { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                sendMsg(msg);

                vsq = 0;
				iecMsg.units.clear();
            }
            printfs(LOG_WARNING, "单点遥信SOE: 地址:%d 状态:%d %04d-%02d-%02d %02d:%02d:%02d.%03d",
                soe.addr + confIt->baseAddr.singleYxnum + 1, soe.value, soe.dateTime[6]+2000, soe.dateTime[5], soe.dateTime[4]&0x1F,
                    soe.dateTime[3], soe.dateTime[2], (soe.dateTime[1]*256+soe.dateTime[0])/1000, 
                    (soe.dateTime[1]*256+soe.dateTime[0])%1000);
        }
    }
    if (vsq > 0)
    {
        /* iecMsg VSQ = units.size() */
        { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
        sendMsg(msg);
    }
}

/*******************************************************************************
@ Function Name     : getValidYcSoe
@ Description       : 上报子模块的遥测SOE信息,并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getValidYcSoe(MsgData &msg)
{
    DataSoe  soe;
    IecMessage iecMsg;  /* elemSize=3 timeScale=7 */
    uint8_t    vsq = 0;
    uint8_t    elemt[3] = {0};
    
    vector<DownSideConfig>::iterator confIt=m_confList.begin();
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
            it!=m_moduleList.end(); it++, confIt++)
    {
        iecMsg.typ = MSG_M_ME_TE_1;
        /* iecMsg VSQ: set by units.size() */
        iecMsg.cot = MSG_COT_SPONT;
        iecMsg.commAddr = 1;
	 
        while ((*it)->getValidYcSoe(soe))
        {
            iecMsg.units.push_back({soe.addr+confIt->baseAddr.validYcnum, {}});
            elemt[0] = soe.value;
            elemt[1] = soe.value >> 8;
            elemt[2] = 0;
            iecMsg.units.back().value.assign(elemt, elemt + 3);
            iecMsg.units.back().time.assign((const char*)soe.dateTime, 7);

            if (++vsq >= YCSOE_PER_FRAME)
            {
                /* iecMsg VSQ = units.size() */
                { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                sendMsg(msg);

                vsq = 0;
				iecMsg.units.clear();
            }
        }
    }
    if (vsq > 0)
    {
       /* iecMsg VSQ = units.size() */
       { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
       sendMsg(msg);
    }
}

/*******************************************************************************
@ Function Name     : getValidYcDcSoe
@ Description       : 上报子模块的遥测直流SOE信息,并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getValidYcDcSoe(MsgData &msg)
{
    DataSoe  soe;
    IecMessage iecMsg;  /* elemSize=3 timeScale=7 */
    uint8_t    vsq = 0;
    uint8_t    elemt[3] = {0};

    iecMsg.typ = MSG_M_ME_TD_1;                                                //用34标示遥测直流
    /* iecMsg VSQ: set by units.size() */
    iecMsg.cot = MSG_COT_SPONT;
    iecMsg.commAddr = 1;
    
    vector<DownSideConfig>::iterator confIt=m_confList.begin();
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
            it!=m_moduleList.end(); it++, confIt++)
    {
        while ((*it)->getValidYcDcSoe(soe))
        {
            iecMsg.units.push_back({soe.addr+confIt->baseAddr.validYcDcNum, {}});
            elemt[0] = soe.value;
            elemt[1] = soe.value >> 8;
            elemt[2] = 0;
            iecMsg.units.back().value.assign(elemt, elemt + 3);
            iecMsg.units.back().time.assign((const char*)soe.dateTime, 7);

            if (++vsq >= YCSOE_PER_FRAME)
            {
                /* iecMsg VSQ = units.size() */
                { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                sendMsg(msg);

                vsq = 0;
				iecMsg.units.clear();
            }
        }
    }
    if (vsq > 0)
    {
        /* iecMsg VSQ = units.size() */
        { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }

        sendMsg(msg);
    }
}

/*******************************************************************************
@ Function Name     : getFP32YcSoe
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getFP32YcSoe(MsgData &msg)
{
    DataFP32Soe  soe;
    IecMessage iecMsg;  /* elemSize=5 timeScale=7 */
    uint8_t    vsq = 0;
    uint8_t    elemt[5] = {0};

    iecMsg.typ = MSG_M_ME_TF_1;
    /* iecMsg VSQ: set by units.size() */
    iecMsg.cot = MSG_COT_SPONT;
    iecMsg.commAddr = 1;
    
    vector<DownSideConfig>::iterator confIt=m_confList.begin();
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
            it!=m_moduleList.end(); it++, confIt++)
    {
        while ((*it)->getFP32YcSoe(soe))
        {
            B32SType value;
            
            iecMsg.units.push_back({soe.addr + confIt->baseAddr.B32YcNum, {}});
            value.fpvalue = soe.value;
            elemt[0] = value.bytes[0];
            elemt[1] = value.bytes[1];
            elemt[2] = value.bytes[2];
            elemt[3] = value.bytes[3];
            elemt[4] = 0;
            iecMsg.units.back().value.assign(elemt, elemt + sizeof(elemt));
            iecMsg.units.back().time.assign((const char*)soe.dateTime, 7);

            if (++vsq >= FP32YCSOE_PER_FRAME)
            {
                /* iecMsg VSQ = units.size() */
                { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                sendMsg(msg);

                vsq = 0;
				iecMsg.units.clear();
            }
        }
    }
    if (vsq > 0)
    {
        /* iecMsg VSQ = units.size() */
        { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }

        sendMsg(msg);
    }
}

/*******************************************************************************
@ Function Name     : getDoubleYxCh
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getDoubleYxCh(MsgData &msg)
{
    getDoubleYxData(msg, MSG_COT_SPONT);
}

/*******************************************************************************
@ Function Name     : getSingleYxCh
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getSingleYxCh(MsgData &msg)
{
    getSingleYxData(msg, MSG_COT_SPONT);
}

/*******************************************************************************
@ Function Name     : getValidYcCh
@ Description       : 轮询一级交流遥测值变位信息
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getValidYcCh(MsgData &msg)
{
    getValidYcData(msg, MSG_COT_SPONT);
}

/*******************************************************************************
@ Function Name     : getValidYcCh
@ Description       : 轮询一级直流遥测值变位信息
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getValidYcDcCh(MsgData &msg)
{
    getValidYcDcData(msg, MSG_COT_SPONT);
}

/*******************************************************************************
@ Function Name     : getFP32YcCh
@ Description       : 轮询一级短浮点遥测值变位信息
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getFP32YcCh(MsgData &msg)
{
    getFP32YcData(msg, MSG_COT_SPONT);
}

/*******************************************************************************
@ Function Name     : getValidYmData
@ Description       : 召取遥脉数据
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getValidYmData(MsgData &msg, uint8_t msgCOT)
{
    IecMessage iecMsg;  /* elemSize=5 */
    uint8_t    element[5] = {0};
    int32_t   value    = 0;
    uint8_t    vsq      = 0;
    uint16_t   addr     = 0;
    uint16_t   num      = 0;
    bool     change   = false;
    
    iecMsg.typ = MSG_M_IT_NA_1;
    iecMsg.cot = msgCOT;
    iecMsg.commAddr = 1;
    if (msgCOT == MSG_COT_SPONT)
    {
        change = true;
    }
    vector<DownSideConfig>::iterator confIt=m_confList.begin();
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
            it!=m_moduleList.end(); it++, confIt++)
    {
        vsq  = 0;
        num  = 0;
        addr = confIt->baseAddr.validYmnum;
        while (num < confIt->config.validYmnum)
        {
            if ((*it)->getValidYmData(num, value, change))
            {
                element[0] = value & 0xFF;
                element[1] = value >> 8 & 0xFF;
                element[2] = value >> 16 & 0xFF;
                element[3] = value >> 24 & 0xFF;
                element[4] = 0;
                
                iecMsg.units.push_back({addr, {}});
                iecMsg.units.back().value.assign(element, element + sizeof(element));

                if (++vsq >= YMNUM_PER_FRAME)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);

                    vsq     = 0;
				iecMsg.units.clear();
                }
            }
            addr ++;
            num  ++;
        }
        if(change)  //突发模式
        {
            (*it)->setYmFreezeConfirm(!change);
        }
        if(!change)
        {
            (*it)->setYmCallEnd(!change);
        }
        if (vsq > 0)
        {
            /* iecMsg VSQ = units.size() */
            { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
            sendMsg(msg);
        }
    }
}


/*******************************************************************************
@ Function Name     : getNodeVersion
@ Description       : 获取指定板卡的软件版本号
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getNodeVersion(MsgData &msg)
{
    IecMessage iecMsg;  /* elemSize=8 */
    NodeInfo info;
    uint8_t  elemt[8] = {0};
    uint8_t  tableId  = 0;
    uint8_t  vsq      = 0;
    uint8_t  maxNode  = 0;
    uint8_t  type     = 0;
    uint8_t  baseNode = 0;

    printfs(LOG_INFO, "读取版本号...\n");

    while (tableId < m_confList.size())
    {
    	if(!strcmp(m_confList[tableId].moduleName.c_str(), "CAN")){
			for (uint8_t i=0; i<canNodeEnd-1; i++)
	        {
	            switch (i)
	            {
	                case 0: type = canNodeYx; 
	                        maxNode = m_confList[tableId].config.yxNodeNum; 
	                        baseNode= m_confList[tableId].baseAddr.yxNodeNum;
	                        break;
	                case 1: type = canNodeYk; 
	                        maxNode = m_confList[tableId].config.ykNodeNum; 
	                        baseNode= m_confList[tableId].baseAddr.ykNodeNum;
	                        break;
	                case 2: type = canNodeYc; 
	                        maxNode = m_confList[tableId].config.ycNodeNum; 
	                        baseNode= m_confList[tableId].baseAddr.ycNodeNum;
	                        break;
	                case 3: type = canNodeGx; 
	                        maxNode = m_confList[tableId].config.gxNodeNum; 
	                        baseNode= m_confList[tableId].baseAddr.gxNodeNum;
	                        break;
	                default: break;
	            }
				for (uint8_t j=0; j<maxNode; j++)
	            {
	                if (!m_moduleList[tableId]->getNodeVersion((canNodeType)type, j, info)){
						printfs(LOG_DEBUG, "m_moduleList[%d]获取版本号失败", tableId);
						continue;
					}

	                elemt[0] = info.nodeType;
	                elemt[1] = info.nodeID + baseNode + 1;
	                elemt[2] = info.nodeLinkStat;
	                elemt[3] = info.nodeVersion >> 8;
	                elemt[4] = info.nodeVersion & 0xFF;
	                elemt[5] = info.nodeExtInfo[0];
	                elemt[6] = info.nodeExtInfo[1];
	                elemt[7] = info.nodeExtInfo[2];
	                
	                iecMsg.units.push_back({0, {}});
	                iecMsg.units.back().value.assign(elemt, elemt + sizeof(elemt));
	                if (vsq >= NODEINFO_PER_FRAME)                                  //信息体按7个字节计算,每帧最大34个
	                {
	                    iecMsg.typ = MSG_C_RD_NC_1;
	                    /* iecMsg VSQ = units.size() */
	                    iecMsg.cot = MSG_COT_INTROGEN;
	                    iecMsg.commAddr = 1;
	                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
	                    sendMsg(msg);
	                    vsq = 0;
				iecMsg.units.clear();
	                }
	            }
	        }
			
		}
		else {
			type = MBType; 
            maxNode = m_confList[tableId].config.modbusDeviceNum; 
            baseNode= m_confList[tableId].baseAddr.modbusDeviceNum;
            for (uint8_t j=0; j<maxNode; j++){	
				if (!m_moduleList[tableId]->getNodeVersion((ModbusNodeType)type, j, info)){
					printfs(LOG_DEBUG, "m_moduleList[%d]获取版本号失败", tableId);
					continue;
				}
	            elemt[0] = LinkTypeModbus;
	            elemt[1] = info.nodeID + baseNode + 1;
	            elemt[2] = info.nodeLinkStat;
	            elemt[3] = info.nodeVersion >> 8;
	            elemt[4] = info.nodeVersion & 0xFF;
	            elemt[5] = info.nodeExtInfo[0];
	            elemt[6] = info.nodeExtInfo[1];
	            elemt[7] = info.nodeExtInfo[2];
	            
	            iecMsg.units.push_back({0, {}});
	            iecMsg.units.back().value.assign(elemt, elemt + sizeof(elemt));
	            if (vsq >= NODEINFO_PER_FRAME)                                  //信息体按7个字节计算,每帧最大34个
	            {
	                iecMsg.typ = MSG_C_RD_NC_1;
	                /* iecMsg VSQ = units.size() */
	                iecMsg.cot = MSG_COT_INTROGEN;
	                iecMsg.commAddr = 1;
	                { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
	                sendMsg(msg);
	                vsq = 0;
				iecMsg.units.clear();
	            }
	        }
		}

        tableId ++;                                                             
    }
    if (vsq)
    {
        iecMsg.typ = MSG_C_RD_NC_1;
        /* iecMsg VSQ = units.size() */
        iecMsg.cot = MSG_COT_INTROGEN;
        iecMsg.commAddr = 1;
        { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
        sendMsg(msg);
    }
}

/*******************************************************************************
@ Function Name     : getNodeInfoCh
@ Description       : 获取版本信息及通信状态变位
@ Input             : m_sendMsgFifo
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getNodeInfoCh(MsgData &msg)
{
    NodeInfo info;
    IecMessage iecMsg;  /* elemSize=8 */
    uint8_t    vsq     = 0;
    uint8_t    nodeID  = 0;
    uint8_t    nodeType= 0;
    uint8_t    data[8] = {0};
    //通信状态转遥信
    DownSideConfig &conf = m_confList.back();
    uint16_t addrOffset = 0;

    //msg.dstModuleName = string(MD_NAME_WEB);
    iecMsg.typ = MSG_C_RD_NC_1;
    /* iecMsg VSQ: set by units.size() */
    iecMsg.cot = MSG_COT_SPONT;
    iecMsg.commAddr = 1;
    
    vector<DownSideConfig>::iterator confIt=m_confList.begin();
    for (vector<RtuBaseClass *>::iterator it=m_moduleList.begin(); 
            it!=m_moduleList.end(); it++, confIt++)
    {
        while ((*it)->getNodeInfoCh(info))
        {
            switch (info.nodeType)
            {
                case canNodeYx: {
                    nodeID = confIt->baseAddr.yxNodeNum;
                    nodeType = LinkTypeYx;
                    if (m_nodeCommYxEnable) {
                        addrOffset = NODE_COMM_IEC104 
                            + confIt->baseAddr.yxNodeNum + info.nodeID;
                    }
                    break;
                }
                case canNodeYk: {
                    nodeID = confIt->baseAddr.ykNodeNum;
                    nodeType = LinkTypeYk;
                    if (m_nodeCommYxEnable) {
                        addrOffset = NODE_COMM_IEC104 + conf.baseAddr.yxNodeNum + conf.config.yxNodeNum 
                            + confIt->baseAddr.ykNodeNum + info.nodeID;
                    }
                    break;
                }
                case canNodeYc: {
                    nodeID = confIt->baseAddr.ycNodeNum;
                    nodeType = LinkTypeYc;
                    if (m_nodeCommYxEnable) {
                        addrOffset = NODE_COMM_IEC104 + conf.baseAddr.yxNodeNum + conf.config.yxNodeNum 
                            + conf.baseAddr.ykNodeNum + conf.config.ykNodeNum 
                            + confIt->baseAddr.ycNodeNum + info.nodeID;
                    }
                    break;
                }
                case canNodeGx: {
                    nodeID = confIt->baseAddr.gxNodeNum;
                    nodeType = LinkTypeGx;
                    if (m_nodeCommYxEnable) {
                        addrOffset = NODE_COMM_IEC104 + conf.baseAddr.yxNodeNum + conf.config.yxNodeNum 
                            + conf.baseAddr.ykNodeNum + conf.config.ykNodeNum 
                            + conf.baseAddr.ycNodeNum + conf.config.ycNodeNum 
                            + confIt->baseAddr.gxNodeNum + info.nodeID;
                    }
                    break;
                }
                case MBType: {
                    nodeID = confIt->baseAddr.modbusDeviceNum; 
                    nodeType = LinkTypeModbus;
                    if (m_nodeCommYxEnable) {
                        addrOffset = NODE_COMM_IEC104 + conf.baseAddr.yxNodeNum + conf.config.yxNodeNum 
                            + conf.baseAddr.ykNodeNum + conf.config.ykNodeNum 
                            + conf.baseAddr.ycNodeNum + conf.config.ycNodeNum 
                            + conf.baseAddr.gxNodeNum + conf.config.gxNodeNum 
                            + confIt->baseAddr.modbusDeviceNum + info.nodeID;
                    }
                    break;
                }
				case Rise3501Type: {
                    nodeID = confIt->baseAddr.rise3501DeviceNum; 
                    nodeType = LinkRise3501;
                    if (m_nodeCommYxEnable) {
                        addrOffset = NODE_COMM_IEC104 + conf.baseAddr.yxNodeNum + conf.config.yxNodeNum 
                            + conf.baseAddr.ykNodeNum + conf.config.ykNodeNum 
                            + conf.baseAddr.ycNodeNum + conf.config.ycNodeNum 
                            + conf.baseAddr.gxNodeNum + conf.config.gxNodeNum 
                            + confIt->baseAddr.modbusDeviceNum + info.nodeID
                            + confIt->baseAddr.rise3501DeviceNum + info.nodeID;
                    }
                    break;
                }
                default:break;
            }			
			//addrOffset = addrOffset;
			if(addrOffset != 0)
			{
				//printfs(LOG_DEBUG, "addrOffset=%d", addrOffset);
			}
            data[0] = nodeType;
            data[1] = info.nodeID + nodeID + 1;
            data[2] = info.nodeLinkStat;
            data[3] = info.nodeVersion >> 8;
            data[4] = info.nodeVersion;
            data[5] = info.nodeExtInfo[0];
            data[6] = info.nodeExtInfo[1];
            data[7] = info.nodeExtInfo[2];

            iecMsg.units.push_back({info.nodeID + nodeID, {}});
            iecMsg.units.back().value.assign(data, data + 8);
            if (vsq >= NODEINFO_PER_FRAME)                                      //信息体按7个字节计算,每帧最大34个
            {
                /* iecMsg VSQ = units.size() */
                { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                sendMsg(msg);
                vsq = 0;
				iecMsg.units.clear();
            }
        }
    }
    if (vsq > 0)
    {
        /* iecMsg VSQ = units.size() */
        { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
        sendMsg(msg);
    }
}

/*******************************************************************************
@ Function Name     : getSpontEvent
@ Description       : 读取子模块的突发事件,根据目的模块名发送至特定模块或者广播
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getSpontEvent()
{
    for (uint8_t childNo=0; childNo<m_moduleList.size(); childNo++)
    {
        SpontEvent event;
        if (m_moduleList[childNo]->readEvent(event))
        {
            switch (event.type)
            {
                case ykCmdEvent:
                {
                    sendYkEventMsg(event, childNo);
                    break;
                }
                case ycSetParamEvent:
                {
                    sendSetParamEventMsg(event, childNo);
                    break;
                }				
                case ycErrReportEvent:
                {
                    sendYcErrReportEventMsg(event, childNo);
                    break;
                }
                case eventTypeUpdate:
                case eventTypeDBUpdate:
                {
                    sendUpdateEventMsg(event, childNo);
                    break;
                }
                case eventTypeICPart:
                {
                    sendCallPartEventMsg(event, childNo);
                    break;
                }
                case ycManualWaveEvent:
                {
                    sendManaulWaveEventMsg(event, childNo);
                    break;
                }				
				case eventTypeSpectrum:
				case eventTypeBeam:
				case eventTypeWave:
				case eventTypePrpd:
				{
				#if 0
					spectrumOnFrame(event.data.data, event.data.len);
				#else
					vector<uint8_t> data = std::vector<uint8_t>(event.data.data, event.data.data + event.data.len);
					spectrumOnFrame1(data);
				#endif //0
					break;
				}
                default:
                    break;
            }
        }
    }
}

/*******************************************************************************
@ Function Name     : getDoubleYxData
@ Description       : 获取子模块双点遥信信息,并发送
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getDoubleYxData(MsgData &msg, uint8_t msgCOT)
{
    IecMessage iecMsg;  /* elemSize=1 */
    uint8_t    DIQ  = 0;
    uint8_t    vsq  = 0;
    uint16_t   addr = 0;
    uint16_t   num  = 0;
    bool     change = false;
    
    iecMsg.typ = MSG_M_DP_NA_1;
    iecMsg.cot = msgCOT;
    iecMsg.commAddr = 1;

    if (msgCOT == MSG_COT_SPONT)
    {
        change = true;
    }
    for (uint8_t i=0; i<m_confList.size(); i++)
    {
        vsq  = 0;
        num  = 0;
        addr = m_confList[i].baseAddr.doubleYxnum;

        while (num < m_confList[i].config.doubleYxnum)
        {
            if (m_moduleList[i]->getDoubleYxData(num, DIQ, change))
            {
					
                iecMsg.units.push_back({addr, {}});
                iecMsg.units.back().value.assign(&DIQ, &DIQ + 1);
                					
                if (++vsq >= YXNUM_PER_FRAME)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);

                    vsq     = 0;
				iecMsg.units.clear();
                }

            }   
            
            num  ++;
            addr ++;
        }
        if (vsq > 0)
        {
            /* iecMsg VSQ = units.size() */
            { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
            sendMsg(msg);
        }
    }
}

/*******************************************************************************
@ Function Name     : getDoubleYxCh
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getSingleYxData(MsgData &msg, uint8_t msgCOT)
{
    IecMessage iecMsg;  /* elemSize=1 */
    uint8_t    DIQ  = 0;
    uint8_t    vsq  = 0;
    uint16_t   addr = 0;
    uint16_t   num  = 0;
    bool     change = false;
    
    iecMsg.typ = MSG_M_SP_NA_1;
    iecMsg.cot = msgCOT;
    iecMsg.commAddr = 1;
    if (msgCOT == MSG_COT_SPONT)
    {
        change = true;
    }
    for (uint8_t i=0; i<m_confList.size(); i++)
    {
        vsq  = 0;
        num  = 0;
        addr = m_confList[i].baseAddr.singleYxnum;

        while (num < m_confList[i].config.singleYxnum)
        {
            if (m_moduleList[i]->getSingleYxData(num, DIQ, change))
            {
                iecMsg.units.push_back({addr, {}});
                iecMsg.units.back().value.assign(&DIQ, &DIQ + 1);

                if (++vsq >= YXNUM_PER_FRAME)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);

                    vsq     = 0;
				iecMsg.units.clear();
                }			

            }
            addr ++;
            num  ++;
        }
        
        if (vsq > 0)
        {
            /* iecMsg VSQ = units.size() */
            { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
            sendMsg(msg);
        }
    }
}

/*******************************************************************************
@ Function Name     : getValidYcData
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getValidYcData(MsgData &msg, uint8_t msgCOT)
{
    IecMessage iecMsg;  /* elemSize=3 */
    uint8_t    element[3] = {0};
    uint16_t   value    = 0;
    uint8_t    vsq      = 0;
    uint16_t   addr     = 0;
    uint16_t   num      = 0;
    bool     change   = false;
    
    if (msgCOT == MSG_COT_SPONT)
    {
        change = true;
    }
    for (uint8_t i=0; i<m_confList.size(); i++)
    {  
        vsq  = 0;
        num  = 0;
        addr = m_confList[i].baseAddr.validYcnum;
        iecMsg.typ = MSG_M_ME_NB_1;
        iecMsg.cot = msgCOT;
        iecMsg.commAddr = 1;
        
        while (num < m_confList[i].config.validYcnum)
        {
            if (m_moduleList[i]->getValidYcData(num, value, change))
            {
                element[0] = value & 0xFF;
                element[1] = value >> 8;
                element[2] = 0;
                
                iecMsg.units.push_back({addr, {}});
                iecMsg.units.back().value.assign(element, element + sizeof(element));

                if (++vsq >= YCNUM_PER_FRAME)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);

                    vsq     = 0;
				iecMsg.units.clear();
                }
            }
            
            addr ++;
            num  ++;
        }
        
        if (vsq > 0)
        {
            /* iecMsg VSQ = units.size() */
            { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
            sendMsg(msg);
        }
    }
}

/*******************************************************************************
@ Function Name     : getValidYtData
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getValidYtData(MsgData &msg, uint8_t msgCOT)
{
    IecMessage iecMsg;  /* elemSize=3 */
    uint8_t    element[3] = {0};
    uint16_t   value    = 0;
    uint8_t    vsq      = 0;
    uint16_t   addr     = 0;
    uint16_t   num      = 0;
    bool     change   = false;
    
    if (msgCOT == MSG_COT_SPONT)
    {
        change = true;
    }
    for (uint8_t i=0; i<m_confList.size(); i++)
    {  
        vsq  = 0;
        num  = 0;
        addr = m_confList[i].baseAddr.validYtNum;
        iecMsg.typ = MSG_P_ME_NB_2;
        iecMsg.cot = msgCOT;
        iecMsg.commAddr = 1;
        
        while (num < m_confList[i].config.validYtNum)
        {
            if (m_moduleList[i]->getValidYtData(num, value, change))
            {
                element[0] = value & 0xFF;
                element[1] = value >> 8;
                element[2] = 0;
                
                iecMsg.units.push_back({addr, {}});
                iecMsg.units.back().value.assign(element, element + sizeof(element));

                if (++vsq >= YCNUM_PER_FRAME)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);

                    vsq     = 0;
				iecMsg.units.clear();
                }
            }
            
            addr ++;
            num  ++;
        }
        
        if (vsq > 0)
        {
            /* iecMsg VSQ = units.size() */
            { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
            sendMsg(msg);
        }
    }
}

/*******************************************************************************
@ Function Name     : getValidYcDcData
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getValidYcDcData(MsgData &msg, uint8_t msgCOT)
{
    IecMessage iecMsg;  /* elemSize=3 */
    uint8_t    element[3] = {0};
    uint16_t   value    = 0;
    uint8_t    vsq      = 0;
    uint16_t   addr     = 0;
    uint16_t   num      = 0;
    bool     change   = false;
    
    iecMsg.typ = MSG_M_ME_NA_1;                                                //用9标示直流一级
    iecMsg.cot = msgCOT;
    iecMsg.commAddr = 1;
    if (msgCOT == MSG_COT_SPONT)
    {
        change = true;
    }
    for (uint8_t i=0; i<m_confList.size(); i++)
    {
        vsq  = 0;
        num  = 0;
        addr = m_confList[i].baseAddr.validYcDcNum;
        while (num < m_confList[i].config.validYcDcNum)
        {
            if (m_moduleList[i]->getValidYcDcData(num, value, change))
            {
                element[0] = value & 0xFF;
                element[1] = value >> 8;
                element[2] = 0;
                
                iecMsg.units.push_back({addr, {}});
                iecMsg.units.back().value.assign(element, element + sizeof(element));

                if (++vsq >= YCNUM_PER_FRAME)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);

                    vsq     = 0;
				iecMsg.units.clear();
                }
            }
            addr ++;
            num  ++;
        }
        
        if (vsq > 0)
        {
            /* iecMsg VSQ = units.size() */
            { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
            sendMsg(msg);
        }
    }
}

/*******************************************************************************
@ Function Name     : getFP32YcData
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::getFP32YcData(MsgData &msg, uint8_t msgCOT)
{
    IecMessage iecMsg;  /* elemSize=5 */
    uint8_t    element[5] = {0};
    B32SType value;
    uint8_t    vsq      = 0;
    uint16_t   addr     = 0;
    uint16_t   num      = 0;
    bool     change   = false;
    
    iecMsg.typ = MSG_M_ME_NC_1;
    iecMsg.cot = msgCOT;
    iecMsg.commAddr = 1;
    if (msgCOT == MSG_COT_SPONT)
    {
        change = true;
    }
    for (uint8_t i=0; i<m_confList.size(); i++)
    {
        vsq  = 0;
        num  = 0;
        addr = m_confList[i].baseAddr.B32YcNum;
        while (num < m_confList[i].config.B32YcNum)
        {
            if (m_moduleList[i]->getFP32YcData(num, value.fpvalue, change))
            {
                element[0] = value.bytes[0];
                element[1] = value.bytes[1];
                element[2] = value.bytes[2];
                element[3] = value.bytes[3];
                element[4] = 0;
                
                iecMsg.units.push_back({addr, {}});
                iecMsg.units.back().value.assign(element, element + sizeof(element));

                if (++vsq >= FP32YC_PER_FRAME)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);

                    vsq     = 0;
				iecMsg.units.clear();
                }
            }
            addr ++;
            num  ++;
        }
        
        if (vsq > 0)
        {
            /* iecMsg VSQ = units.size() */
            { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
            sendMsg(msg);
        }
    }
}

/*******************************************************************************
@ Function Name     : sendYkEventMsg
@ Description       : 根据子模块遥控返回信息,将其发送给需要的模块
@ Input             : event: 子模块遥控事件
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::sendYkEventMsg(SpontEvent &event, int8_t childNo, TableType type)
{
    MsgData  msg;
    uint8_t    timeLen = 0;
    uint16_t   addr = 0;
    
    if ((event.cmd == MSG_C_DC_NA_1)
        || (event.cmd == MSG_C_SC_NA_1))
    {
        timeLen = 0;
    }
    else if ((event.cmd == MSG_C_DC_TA_1)
        || (event.cmd == MSG_C_SC_TA_1))
    {
        timeLen = 7;
    }
    if ((event.cmd == MSG_C_SC_NA_1) || (event.cmd == MSG_C_SC_TA_1)) {
        type = yksingleTable;
    }
    
    IecMessage iecMsg;  /* elemSize=1 timeScale=timeLen */

    msg.dstModuleName = event.srcModuleName;
    msg.srcModuleName = string(MD_NAME_DOWN"_0");

    iecMsg.typ = event.cmd;
    /* iecMsg VSQ: set by units.size() */

    iecMsg.cot = event.state | event.value;
    iecMsg.commAddr = 1;
    if (childNo == -1)
    {
        iecMsg.units.push_back({event.addr, {}});
    }
    else
    {
        if (type == ykTable)
            addr = event.addr + m_confList[childNo].baseAddr.doubleYknum;
        else if (type == yksingleTable)
            addr = event.addr + m_confList[childNo].baseAddr.singleYknum;
        iecMsg.units.push_back({addr, {}});
    }
    iecMsg.units.back().value.assign(event.data.data, event.data.data + event.data.len);
    if (timeLen == 7)
    {
        DateService date;
        DateType    time;
        uint8_t       dateTime[7] = {0};

        date.GetCurrentDate(&time);
        dateTime[6] = time.m_year - 100;
        dateTime[5] = time.m_mon;
        dateTime[4] = time.m_mday | 0x20;
        dateTime[3] = time.m_hour;
        dateTime[2] = time.m_min;
        dateTime[1] = (time.m_sec * 1000 + time.m_msec) >> 8;
        dateTime[0] = (time.m_sec * 1000 + time.m_msec);
        iecMsg.units.back().time.assign((const char*)dateTime, 7);
    }
    
    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
        
    sendMsg(msg);
    sendYkStr(msg);                                                             //记录遥控操作
    /*信息安全测试执行完成之后需发送停止激活确认*/
    #if 0
    /*执行*/
    uint8_t    action = 0;
    if (!(event.data.data[0] & 0x80)) {
        action = 1;
    }
    if (event.value == failed || !action) 
        return ;
    iecMsg.cot = MSG_COT_ACTTERM;
    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    sendMsg(msg);
    sendYkStr(msg);                                                             //记录遥控操作
    #endif
}

/*******************************************************************************
@ Function Name     : sendSetParamEventMsg
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::sendSetParamEventMsg(SpontEvent &event, int8_t childNo)
{
    MsgData  msg;
    IecMessage iecMsg;

    msg.dstModuleName = event.srcModuleName;
    msg.srcModuleName = string(MD_NAME_DOWN"_0");
    
    iecMsg.typ = event.cmd;
    iecMsg.cot = event.state | event.value;
    iecMsg.commAddr = 1;
    if (childNo == -1)
    {
        iecMsg.units.push_back({event.addr, {}});
    }
    else
    {
        iecMsg.units.push_back({event.addr + m_confList[childNo].baseAddr.validYtNum, {}});
    }
    iecMsg.units.back().value.assign(event.data.data, event.data.data + event.data.len);
    
    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    sendMsg(msg);
}

/*******************************************************************************
@ Function Name     : sendYcErrReportEventMsg
@ Description       : 轮询一级变位信息，并广播给其它所需模块
@ Input             : msg:  消息
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::sendYcErrReportEventMsg(SpontEvent &event, int8_t childNo)
{
    MsgData  msg;
    IecMessage iecMsg;

    msg.dstModuleName = "";
    msg.srcModuleName = string(MD_NAME_DOWN"_0");
    
    iecMsg.typ = MSG_M_ER_TA_1;
    iecMsg.cot = event.state | event.value;
    iecMsg.commAddr = 1;
    
    iecMsg.units.push_back({event.addr, {event.data.data, event.data.data + event.data.len}});
    
    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    sendMsg(msg);
}

/*******************************************************************************
@ Function Name     : sendUpdateEventMsg
@ Description       : 发送在线升级信息
@ Input             : event:  在线升级结果返回事件
                      childNo: 子模块
@ Output            : msg
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::sendUpdateEventMsg(SpontEvent &event, int8_t childNo)
{
    uint8_t    elemt[2] = {0};
    MsgData  msg;
    IecMessage iecMsg;

    elemt[0] = event.data.data[0];
    elemt[1] = event.data.data[1];

    if(event.type == eventTypeUpdate)
    {
        switch (elemt[0])
        {
            case canNodeYx: elemt[1] += m_confList[childNo].baseAddr.yxNodeNum;break;
            case canNodeYk: elemt[1] += m_confList[childNo].baseAddr.ykNodeNum;break;
            case canNodeYc: elemt[1] += m_confList[childNo].baseAddr.ycNodeNum;break;
            case canNodeGx: elemt[1] += m_confList[childNo].baseAddr.gxNodeNum;break;
            default:break;
        }
    }

    msg.dstModuleName = event.srcModuleName;
    msg.srcModuleName = string(m_moduleName);
    
    iecMsg.typ = event.cmd;
    iecMsg.cot = event.state | event.value;
    iecMsg.commAddr = 1;
    iecMsg.units.push_back({event.addr, {elemt[0], elemt[1]}});
    
    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    sendMsg(msg);
}


/*******************************************************************************
@ Function Name     : sendCallPartEventMsg
@ Description       : 接收全召结束事件
@ Input             : event:  在线升级结果返回事件
                      childNo: 子模块
@ Output            : msg
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::sendCallPartEventMsg(SpontEvent &event, int8_t childNo)
{
    uint8_t    elemt[2] = {0};
    MsgData  msg;

    msg.dstModuleName = event.srcModuleName;
    msg.srcModuleName = string(m_moduleName);
    elemt[0] = event.data.data[0];
    switch (elemt[0])
    {
        case canNodeYx:
            {
                IecMessage iecMsg;  /* elemSize=1 */
                uint8_t    vsq    = 0;
                uint8_t    point  = 0;
                uint8_t    yx     = 0;

                iecMsg.typ = MSG_M_SP_NA_1;
                iecMsg.cot = MSG_COT_INTROGEN;
                iecMsg.commAddr = 1;
                for (point=3; point<event.data.len; point++)                    //单点
                {
                    if (event.data.data[point] & 0x80)                              
                        break;
                    yx = event.data.data[point];
                    iecMsg.units.push_back({vsq, {}});
                    iecMsg.units.back().value.assign(&yx, &yx + 1);
                    vsq ++;
                    if (vsq >= YXNUM_PER_FRAME)
                    {
                        /* iecMsg VSQ = units.size() */
                        { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                        sendMsg(msg);
                        vsq     = 0;
				iecMsg.units.clear();
                    }
                }
                if (vsq > 0)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);
                }

                vsq = 0;
                iecMsg.typ = MSG_M_DP_NA_1;
                for (; point<event.data.len; point++)                           //双点
                {
                    yx = event.data.data[point] & 0x03;
                    iecMsg.units.push_back({vsq, {}});
                    iecMsg.units.back().value.assign(&yx, &yx + 1);
                    vsq ++;
                }
                if (vsq > 0)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);
                }
                break;
            }
        case canNodeYc:
            {
                IecMessage iecMsg;  /* elemSize=3 */
                uint8_t    vsq    = 0;
                uint8_t    data[3]= {0};

                iecMsg.typ = MSG_M_ME_NB_1;
                iecMsg.cot = MSG_COT_INTROGEN;
                iecMsg.commAddr = 1;
                for (uint8_t i=3; i<event.data.len; i+=2)
                {
                    data[0] = event.data.data[i];
                    data[1] = event.data.data[i+1];
                    data[2] = 0;
                    iecMsg.units.push_back({vsq, {}});
                    iecMsg.units.back().value.assign(data, data + sizeof(data));
                    vsq ++;
                }
                if (vsq > 0)
                {
                    /* iecMsg VSQ = units.size() */
                    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
                    sendMsg(msg);
                }
                break;
            }
        default:break;
    }

    {                                                                           //召取结束
        IecMessage iecMsg;  /* elemSize=1 */
        
        iecMsg.typ = MSG_C_IC_NB_1;
        /* iecMsg VSQ: set by units.size() */

        iecMsg.cot = event.state | event.value;
        iecMsg.commAddr = 1;
        iecMsg.units.push_back({event.addr, {}});
        iecMsg.units.back().value.assign(elemt, elemt + sizeof(elemt));

        { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
        sendMsg(msg);
    }
}

/*******************************************************************************
@ Function Name     : sendManaulWaveEventMsg
@ Description       : 发送手动录波事件消息
@ Input             : msg:消息内容
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::sendManaulWaveEventMsg(SpontEvent &event, int8_t childNo)
{
    MsgData  msg;
    IecMessage iecMsg;

    msg.dstModuleName = event.srcModuleName;
    msg.srcModuleName = string(MD_NAME_DOWN"_0");
    
    iecMsg.typ = event.cmd;
    iecMsg.cot = event.state | event.value;
    iecMsg.commAddr = 1;
    if (childNo == -1)
    {
        iecMsg.units.push_back({event.addr, {}});
    }
    else
    {
        iecMsg.units.push_back({event.addr + m_confList[childNo].baseAddr.validYcCircuitNum, {}});
    }
    iecMsg.units.back().value.assign(event.data.data, event.data.data + event.data.len);
    
    { auto _p = packMessage(iecMsg); msg.data1 = std::move(_p); }
    sendMsg(msg);
}

/*******************************************************************************
@ Function Name     : sendYkStr
@ Description       : 向数据库模块发送遥控操作记录字符串
@ Input             : msg:消息内容
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::sendYkStr(const MsgData &rmsg)
{
    MsgData msg;
    char    info[240] = "";
    char    cmd[24]   = "";
    char    type[10]   = "";
    char    stat[10]   = "";
	sprintf(stat, "成功");
    uint8_t   con       = rmsg.data1[6];
    uint8_t   cot       = rmsg.data1[2];

    if (rmsg.data1[0] == MSG_C_DC_NA_1 || rmsg.data1[0] == MSG_C_DC_TA_1)
    {
        con -= 0x01;
        sprintf(type, "双点");
    }
    else
    if (rmsg.data1[0] == MSG_C_SC_NA_1 || rmsg.data1[0] == MSG_C_SC_TA_1)
    {
        sprintf(type, "单点");
    }
    else
        sprintf(type, "异常");

    if (cot & 0x40)
        sprintf(stat, "失败");

    switch (con)
    {
        case 0x80: (cot == MSG_COT_ACTTERM)? sprintf(cmd, "控分取消") : sprintf(cmd, "控分选择"); break;
        case 0x81: (cot == MSG_COT_ACTTERM)? sprintf(cmd, "控合取消") : sprintf(cmd, "控合选择"); break;
        case 0x00: sprintf(cmd, "控分执行"); break;
        case 0x01: sprintf(cmd, "控合执行"); break;
        default: break;
    }
    sprintf(info, "%s遥控%s命令%s!", type, cmd, stat);

    msg.dstModuleName = string(MD_NAME_DATABASE"_0");
    msg.srcModuleName = m_moduleName;
    msg.data1.clear();
    msg.data1.push_back(MSG_C_ST_NA_1);
    msg.data1.push_back(1);
    msg.data1.push_back(MSG_COT_REQ);
    msg.data1.push_back(1);
    msg.data1.push_back(MAddrDownSide);
    msg.data1.push_back(StrMTypeYkStr);
    msg.data1.push_back(strlen(info));
    msg.data1.insert(msg.data1.end(), info, info + strlen(info));
    
    sendMsg(msg);

    printfs(LOG_ERROR, "模块:%s 的%s", rmsg.dstModuleName.c_str(), info);
}

/*******************************************************************************
@ Function Name     : setVirtualData
@ Description       : 设置虚拟数据
@ Input             : msg:消息内容
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setVirtualData(const IecMessage &iecMsg)
{
    uint16_t addr     = 0; 
    int8_t  childNo  = 0;
    uint8_t  elemt[5] = {0};
    uint8_t  allvsq = iecMsg.units.size();
    uint8_t  isSQ   = 0 /* SQ bit */;


    if (isSQ)
    {
        addr = iecMsg.units[0].addr;
    }
   
    for (uint8_t vsq=0; vsq<allvsq; vsq++)
    {
        addr    = (isSQ) ? (iecMsg.units[vsq].addr+vsq) : iecMsg.units[vsq].addr;

        if (memcpy(elemt, iecMsg.units[vsq].value.data(), std::min(iecMsg.units[vsq].value.size(), sizeof(elemt))))
        {
            switch (elemt[0]) {
            case virtualDataSingleYx: {
                if ((childNo = getTableNo(addr, singleYxTable)) == -1)
                    break;
                addr -= m_confList[childNo].baseAddr.singleYxnum;
                m_moduleList[childNo]->setSingleYxStat(addr, elemt[1]);
                break;
            }
            case virtualDataDoubleYx: {
                if ((childNo = getTableNo(addr, doubleYxTable)) == -1)
                    break;
                addr -= m_confList[childNo].baseAddr.doubleYxnum;
                m_moduleList[childNo]->setDoubleYxStat(addr, elemt[1]);
                break;
            }
            case virtualDataYc: {
                if ((childNo = getTableNo(addr, validYcTable)) == -1)
                    break;
                addr -= m_confList[childNo].baseAddr.validYcnum;
                m_moduleList[childNo]->setValidYcValue(addr, elemt[1] | (elemt[2] << 8));
                break;
            }
            default: break;
            }
        }
    }
}

/*******************************************************************************
@ Function Name     : setCallbackFunctionSpectrum
@ Description       : 设置回调函数
@ Input             : msg:消息内容
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setCallbackFunction(std::function<void(const uint8_t *, uint32_t)> func)
{
	spectrumOnFrame = func;
}

/*******************************************************************************
@ Function Name     : setCallbackFunctionSpectrum
@ Description       : 设置回调函数
@ Input             : msg:消息内容
@ Output            : None;
@ Return            : None;
*******************************************************************************/
void DownSideDataModule::setCallbackFunction(std::function<void(vector<uint8_t> &)> func)
{
	spectrumOnFrame1 = func;
}

