#include <unistd.h>
using namespace std;
#include <sys/syscall.h>
#include "MinmeaModule.h"

MinmeaModule::MinmeaModule(const string &confFileName):m_fpYcSoeFifo(10){
    minmeaProtocol = new MinmeaProtocol();
    serialProcess.configFileName    = confFileName;
    serialProcess.invokModule       = "MinmeaModule";
    serialProcess.d_protocol        = minmeaProtocol;
    serialProcess.recvFrame         = NULL;
    serialProcess.needResetSerial   = NULL;
    
	configFileName = confFileName;
	initConfig();
	//db = DataBaseModule::getInstance();
}

MinmeaModule::~MinmeaModule(void){
	if (minmeaProtocol) {
		delete minmeaProtocol;
		minmeaProtocol = nullptr;
	}
}

void MinmeaModule::loadConfig(){
	//initConfig();
	BaseDataConfig config = getConfig(); 
    setConfig(config);
}

void MinmeaModule::initModule(){
	;
}

void MinmeaModule::initConfig(){
    XmlNodeParser f_XmlNodeParser((int8_t *)configFileName.c_str(), 
                                    (int8_t *)"/MinmeaConfig");
    int32_t value        = 0;
    //// int8_t  nodePath[120] = ""; // Unused variable
    int8_t  text[128]     = "";

	int8_t  nodePath[120] = "";
    //uint16_t childCount   = 0;
    // uint16_t doubleYxAddr = 0; // Unused variable
    // uint16_t singleYxAddr = 0; // Unused variable

    //uint16_t ykAddr       = 0;				         
	// uint16_t ycAddr       = 0; // Unused variable
	//uint16_t ytAddr       = 0;
    //uint16_t  requestNum   = 0;
       
    f_XmlNodeParser.FindNode((int8_t *)"//BaseAddr");
    if (f_XmlNodeParser.GetChildContent((int8_t *)"YxBaseAddr", value)){
        m_baseAddrOffset.yxBaseAddr.baseAddr = value;
        f_XmlNodeParser.GetChildProperty((int8_t *)"YxBaseAddr", (int8_t *)"type", text);
        m_baseAddrOffset.yxBaseAddr.type = (!strcmp((char *)text, "relate")) 
                                        ? BASEADDR_TYPE_RELATE : BASEADDR_TYPE_ABSOLUTE;
    }
    if (f_XmlNodeParser.GetChildContent((int8_t *)"YkBaseAddr", value)){
        m_baseAddrOffset.ykBaseAddr.baseAddr = value;
        f_XmlNodeParser.GetChildProperty((int8_t *)"YkBaseAddr", (int8_t *)"type", text);
        m_baseAddrOffset.ykBaseAddr.type = (!strcmp((char *)text, "relate")) 
                                        ? BASEADDR_TYPE_RELATE : BASEADDR_TYPE_ABSOLUTE;
    }
	//f_XmlNodeParser.FindNode((int8_t *)"/MinmeaConfig/dataMsg");

    //uint16_t deviceAddr    = 0;
    // uint16_t frameNum     = 0; // Unused variable
	//uint16_t sendDataLen	= 0;
	// uint16_t parseDataLen	= 0; // Unused variable

    DataFP32 a;
    m_data.floatYc.resize(2, a);
	
    printfs(LOG_INFO, "双点遥信数目:   %d", m_data.doubleYx.size());
    printfs(LOG_INFO, "单点遥信数目:   %d", m_data.singleYx.size());
	printfs(LOG_INFO, "浮点遥测数目:   		%d", m_data.floatYc.size());
    printfs(LOG_INFO, "遥控数目:       %d", 0);
	printfs(LOG_INFO, "遥调数目:       %d", m_data.yt.size());
}

void MinmeaModule::setBaseAddr(const BaseDataConfig &baseAddr) {    
    m_baseAddr = baseAddr;

    if (m_baseAddrOffset.yxBaseAddr.type == BASEADDR_TYPE_RELATE){
        m_baseAddr.singleYxnum += m_baseAddrOffset.yxBaseAddr.baseAddr;
    }else{
        m_baseAddr.singleYxnum = m_baseAddrOffset.yxBaseAddr.baseAddr;
    }
    if (m_baseAddrOffset.ykBaseAddr.type == BASEADDR_TYPE_RELATE){
        m_baseAddr.doubleYknum += m_baseAddrOffset.ykBaseAddr.baseAddr;
    }else{
        m_baseAddr.doubleYknum = m_baseAddrOffset.ykBaseAddr.baseAddr;
    }
    if (m_baseAddrOffset.ycBaseAddr.type == BASEADDR_TYPE_RELATE){
        m_baseAddr.validYcnum += m_baseAddrOffset.ycBaseAddr.baseAddr;
    }else{
        m_baseAddr.validYcnum = m_baseAddrOffset.ycBaseAddr.baseAddr;
    }
    if (m_baseAddrOffset.ytBaseAddr.type == BASEADDR_TYPE_RELATE){
        m_baseAddr.validYtNum += m_baseAddrOffset.ytBaseAddr.baseAddr;
    }else{
        m_baseAddr.validYtNum = m_baseAddrOffset.ytBaseAddr.baseAddr;
    }
	if (m_baseAddrOffset.cirBaseAddr.type == BASEADDR_TYPE_RELATE){
        m_baseAddr.validYcCircuitNum += m_baseAddrOffset.cirBaseAddr.baseAddr;
    }else{
        m_baseAddr.validYcCircuitNum = m_baseAddrOffset.cirBaseAddr.baseAddr;
    }  
    if (m_baseAddrOffset.ycBaseAddr.type == BASEADDR_TYPE_RELATE){
        m_baseAddr.B32YcNum += m_baseAddrOffset.ycBaseAddr.baseAddr;   
    }else{
        m_baseAddr.B32YcNum = m_baseAddrOffset.ycBaseAddr.baseAddr;
    }
}

BaseDataConfig MinmeaModule::getConfig(){
    BaseDataConfig config;

    config.doubleYxnum = m_data.doubleYx.size();
    config.singleYxnum = m_data.singleYx.size();
    config.doubleYknum = 0;
    config.singleYknum = 0;
	config.validYcnum = m_data.Yc.size();
	config.validYtNum = m_data.yt.size();
    config.B32YcNum = m_data.floatYc.size();

    //config.rise3501DeviceNum = m_config.deviceNum;
	config.modbusDeviceNum = 0;
	config.modbusSerialIndex = 0;

    m_baseDataConfig = config;

    printfs(LOG_INFO, "VideoFusionProtocol config: doubleYxNum=%d, singleYxNum=%d, doubleYkNum=%d"\
                "singleYkNum=%d, validYcNum=%d, valieYtNum=%d, B32YcNum=%d", 
                config.doubleYxnum, config.singleYxnum, config.doubleYknum, 
                config.singleYknum, config.validYcnum, config.validYtNum, config.B32YcNum);  

    return config;
}

void MinmeaModule::run(){	
    serialModule.setConfig(serialProcess);
    serialModule.run();

    pthread_t thread8;
	pthread_attr_t attr8;
	pthread_attr_init(&attr8);
	pthread_attr_setdetachstate(&attr8, PTHREAD_CREATE_DETACHED);
	int rc8 = pthread_create(&thread8, &attr8,	runYcProcess, static_cast<void *>(this));
	if(rc8 == 0){
		printfs(LOG_INFO,"create YC process thread success!\r\n");
	}else{
		printfs(LOG_INFO,"create YC process thread fail!\r\n");
		exit(1);
	}
}

void * MinmeaModule::runYcProcess(void *arg){
    printfs(LOG_INFO, "IPC Msg process thread:LWPID:%d TID:%d\n", 
        syscall(SYS_gettid), pthread_self());
	MinmeaModule  *_this = static_cast<MinmeaModule *>(arg);
	
    while(true){    
        usleep(100*1000);
        _this->processYcMsg();
    }    
    pthread_exit(NULL);
	return NULL;
}

bool MinmeaModule::processYcMsg(){
    PositionInfo positionInfo = minmeaProtocol->getPositionInfo();
    uint16_t addr = 0;
    float ycValue = 0.0;

    if(positionInfo.hasFix){
        ycValue = positionInfo.latitude;
        if(abs(ycValue - m_data.Yc[addr+1].value) > 0.00001){
            m_data.floatYc[addr].change = true;
        }
        m_data.floatYc[addr].value      = (positionInfo.latitude);
    }
    
    if(positionInfo.hasFix){
        ycValue = positionInfo.longitude;
        if(abs(ycValue - m_data.Yc[addr+1].value) > 0.00001){
            m_data.floatYc[addr+1].change = true;
        }
        m_data.floatYc[addr+1].value     = (positionInfo.longitude);
    }
	return true;
}

bool MinmeaModule::getFP32YcSoe(DataFP32Soe &soe){
    return m_fpYcSoeFifo.popFront(soe);
}

bool MinmeaModule::getFP32YcData(const uint16_t addr, float &value, bool change) {
    if (!change) {
        if (addr < m_data.floatYc.size()) {
            value = m_data.floatYc[addr].value;
            return true;
        }
    } else {
        if (addr < m_data.floatYc.size()) {
            if (m_data.floatYc[addr].change) {
                value = m_data.floatYc[addr].value;
                m_data.floatYc[addr].change = false;
                return true;
            }
        }
    }
    return false;
}
