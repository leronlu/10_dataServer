#include <unistd.h>
using namespace std;
#include <sys/syscall.h>
#include "MicrophoneModule.h"

MicrophoneModule::MicrophoneModule(const string &confFileName){
	configFileName = confFileName;
	initConfig();
	//db = DataBaseModule::getInstance();
}

MicrophoneModule::~MicrophoneModule(void){
	;
}

void MicrophoneModule::loadConfig(){
	//initConfig();
	BaseDataConfig config = getConfig(); 
    setConfig(config);
}

void MicrophoneModule::initModule(){
	;
}

void MicrophoneModule::initConfig(){
    XmlNodeParser f_XmlNodeParser((int8_t *)configFileName.c_str(), 
                                    (int8_t *)"/ACConfig");
    int32_t value        = 0;
    //int8_t  nodePath[120] = "";
    int8_t  text[128]     = "";

	int8_t  nodePath[120] = "";
    //uint16_t childCount   = 0;
    uint16_t doubleYxAddr = 0;
    uint16_t singleYxAddr = 0;

    //uint16_t ykAddr       = 0;				         
	uint16_t ycAddr       = 0;
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

	f_XmlNodeParser.FindNode((int8_t *)"/ACConfig/dataMsg");

    //uint16_t deviceAddr    = 0;
    uint16_t frameNum     = 0;
	//uint16_t sendDataLen	= 0;
	uint16_t parseDataLen	= 0;

    frameNum   = f_XmlNodeParser.GetChildCounter("frame");
    //requestNum = frameNum;
    for (uint16_t id=0; id<frameNum; id++){
        MRISEFrameType frameType = RISEFrameNoType;
        sprintf((char *)nodePath, "/ACConfig/dataMsg/frame[%d]",id+1);
        f_XmlNodeParser.FindNode(nodePath);
		memset(text, 0, sizeof(text));
		if(f_XmlNodeParser.GetProperty((int8_t*)"type", text)){
			frameType 		= getFrameType((char *)text);
		}

		DataINT16Yt ytdata;
		DataINT8UNew yxdata; 
		DataINT16S ycdata;
		uint16_t defaultValue = 0;
		//uint8_t qpm = 0;
		switch (frameType){
        case RISEFrameReadData: {	
			MRISEParseData ycParseData;
			ycParseData.THV = 1000;
			parseDataLen	= f_XmlNodeParser.GetChildCounter("data");
			for(uint16_t j = 0; j < parseDataLen; j++){				
				sprintf((char *)nodePath, "/ACConfig/dataMsg/frame[%d]/data[%d]", id+1, j+1);
                f_XmlNodeParser.FindNode(nodePath);
				
				f_XmlNodeParser.GetProperty((int8_t*)"type", text);
				RISERcvRegType type = getRcvRegType((char *)text);
				memset(text, 0, sizeof(text));
				//f_XmlNodeParser.GetProperty((int8_t*)"desc", text);
				//memcpy(rcvReg.desc, text, sizeof(text));

				f_XmlNodeParser.GetProperty((int8_t*)"defaultValue", value);
				defaultValue = value;

				f_XmlNodeParser.GetProperty((int8_t*)"repeat", value);
				uint16_t repeat = value;
				for(int index = 0; index < repeat; index++){
					if((type == RISERcvRegYx) || (type == RISERcvRegYx1)){
						yxdata.value = defaultValue;
						m_data.singleYx.push_back(yxdata);
						singleYxAddr += 1;
					}else if((type == RISERcvRegYc)|| (type == RISERcvRegYc1)){
						ycdata.value = defaultValue;
						m_data.Yc.push_back(ycdata);
						ycAddr += 1;
						//m_dataParse.yc.push_back(ycParseData);
					}else if(type == RISERcvRegDoubleYx){
						yxdata.value = defaultValue;
						m_data.doubleYx.push_back(yxdata);
						doubleYxAddr += 1;
					}
				}
				
			}
			f_XmlNodeParser.FindNode(nodePath);
            break;
        } 
		default:
			break;
		}

    }

	DataINT16Yt a;
	m_data.yt.resize(0, a);
	
    printfs(LOG_INFO, "双点遥信数目:   %d", m_data.doubleYx.size());
    printfs(LOG_INFO, "单点遥信数目:   %d", m_data.singleYx.size());
	printfs(LOG_INFO, "遥测数目:   		%d", m_data.Yc.size());
    printfs(LOG_INFO, "遥控数目:       %d", 0);
	printfs(LOG_INFO, "遥调数目:       %d", m_data.yt.size());
}

void MicrophoneModule::setBaseAddr(const BaseDataConfig &baseAddr) {    
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
    //pressure->setBaseCircuitAddr(m_baseAddr.validYcCircuitNum);
}

BaseDataConfig MicrophoneModule::getConfig(){
    BaseDataConfig config;

    config.doubleYxnum = m_data.doubleYx.size();
    config.singleYxnum = m_data.singleYx.size();
    config.doubleYknum = 0;
    config.singleYknum = 0;
	config.validYcnum = m_data.Yc.size();
	config.validYtNum = m_data.yt.size();

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

void MicrophoneModule::run(){	
	pthread_t thread8;
	pthread_attr_t attr8;
	pthread_attr_init(&attr8);
	pthread_attr_setdetachstate(&attr8, PTHREAD_CREATE_DETACHED);
	int rc8 = pthread_create(&thread8, &attr8,	runIPCProcess, static_cast<void *>(this));
	if(rc8 == 0){
		printfs(LOG_INFO,"create IPC Msg process thread success!\r\n");
	}else{
		printfs(LOG_INFO,"create IPC Msg process thread fail!\r\n");
		exit(1);
	}
}

void * MicrophoneModule::runIPCProcess(void * arg){
	printfs(LOG_INFO, "IPC Msg process thread:LWPID:%d TID:%d\n", 
        syscall(SYS_gettid), pthread_self());
	MicrophoneModule  *_this = static_cast<MicrophoneModule *>(arg);
	
	_this->IPCProcess();
    
    pthread_exit(NULL);
	return NULL;
}
bool MicrophoneModule::IPCProcess(){
	int id = (int)IPCMsgType::RTU;
	int msgid = getIPCMsg(id, "/tmp/rtumsg");		
    //int cnt = 0;
    RTUMessage message;

    while(true){
        usleep(100*1000);
        int ret = rcvIPCMsg(msgid, message);  									// 接收消息
        if (ret == -1){
        	continue;
        }
        //std::cout << "cnt: " << cnt++ << std::endl;
        std::vector<uint16_t> yx(message.data, message.data + 128);
		processYxMsg(yx);
		std::vector<uint16_t> yc(&(message.data[128]), &message.data[128] + 128);
		processYcMsg(yc);
        //process(b, 48000, 20000);
     }
    msgctl(msgid, IPC_RMID, nullptr);  											// 删除消息队列
    return 0;
}

bool MicrophoneModule::processYcMsg(vector<uint16_t> pressure){
	 // 遍历vector v并对每个元素进行赋值
    for(int i = 0; i < (int)pressure.size(); i++){
		uint16_t addr = i;
	    uint16_t ycValue = (uint16_t)(pressure[i]);

		if(addr < m_data.Yc.size()){
			if(m_data.Yc[addr].value != ycValue){
				m_data.Yc[addr].change = true;
			}
			m_data.Yc[addr].value      = ycValue;
		}
	}
	return true;
}

bool MicrophoneModule::processYxMsg(vector<uint16_t> status){
	 for(int i = 0; i < (int)status.size(); i++){
	 	uint16_t addr = i;
		processYxEvent(addr, status[i], (int8_t*)"", false);
	 }
	 return true;
}

bool MicrophoneModule::processYxEvent(uint16_t dataOffset, uint8_t yxSt, 
	int8_t* info, bool firstPool){
    uint8_t THV   = 0;
    //static uint8_t yxSt  = 0;													//地线初始状态为取出
    vector<uint16_t>     rdYx;
    vector<DataINT8UNew> *yxdata = NULL;
    //STLDeque<DataSoe> *yxsoefifo = NULL;
    //char   info[5] = "";
    uint16_t addr    = 0;
    //printfs(LOG_DEBUG, "进入遥信数据解析");

	bool bitChangeFlags = false;

	yxdata     = &m_data.singleYx;
    //sprintf(info, "单点");
 
    addr = dataOffset + 0;

    if (mAbs(yxSt, yxdata->at(addr).value) > THV){
        if (!firstPool) {
			putSoeToFifo(addr, yxSt, m_singleYxSoeFifo);
			bitChangeFlags = true;
        }
        yxdata->at(addr).change = true;                                 		//一级变位
	}
	
    yxdata->at(addr).value = yxSt;
    if (yxdata->at(addr).change) {
        printfs(LOG_WARNING, "描述:%s, 单点遥信变位: 地址=%d 状态=%d\n", 
                        info, addr, yxSt);
    }
	return bitChangeFlags;
}
	
bool MicrophoneModule::getDoubleYxSoe(DataSoe &soe){
    return m_doubleYxSoeFifo.popFront(soe);
}

bool MicrophoneModule::getSingleYxSoe(DataSoe &soe){
	return m_singleYxSoeFifo.popFront(soe);
}

bool MicrophoneModule::getValidYcSoe(DataSoe &soe){
    return m_validYcSoeFifo.popFront(soe);
}

bool MicrophoneModule::getDoubleYxData(const uint16_t addr, uint8_t &value, bool change){
    if (!change){
        if (addr < m_data.doubleYx.size()){
            value = m_data.doubleYx[addr].value;
            return true;
        }
    }else{
        if (addr <m_data.doubleYx.size()){
            if (m_data.doubleYx[addr].change){
                value = m_data.doubleYx[addr].value;
                m_data.doubleYx[addr].change = false; 
                return true;
            }
        }
    }
    return false;
}

bool MicrophoneModule::getSingleYxData(const uint16_t addr, uint8_t &value, bool change){
    if (!change){
        if (addr < m_data.singleYx.size()){
            value = m_data.singleYx[addr].value;
            return true;
        }
    }else {
        if (addr <m_data.singleYx.size()){
            if (m_data.singleYx[addr].change){
                value = m_data.singleYx[addr].value;
                m_data.singleYx[addr].change = false;
                return true;
            }
        }
    }
    return false;
}

bool MicrophoneModule::getValidYcData(const uint16_t addr, uint16_t &value, bool change){
    if (!change){
        if (addr < m_data.Yc.size()){
            value = m_data.Yc[addr].value;
            return true;
        }
    }else{
        if (addr <m_data.Yc.size()){
            if (m_data.Yc[addr].change){
                value = m_data.Yc[addr].value;
                m_data.Yc[addr].change = false;
                return true;
            }
        }
    }
    return false;
}

void MicrophoneModule::putSoeToFifo(uint16_t addr, int16_t value, STLDeque<DataSoe> &soefifo){
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

void MicrophoneModule::putSoeToFifo(uint16_t addr, int16_t value, uint8_t type,STLDeque<DataSoe> &soefifo){
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

MRISEFrameType MicrophoneModule::getFrameType(const char *type){
    if (!strcasecmp(type, "readData"))
        return RISEFrameReadData;
    else if (!strcasecmp(type, "bitChange"))
        return RISEFrameBitChange;
    else if (!strcasecmp(type, "readParam"))
        return RISEFrameReadParam;
	else if (!strcasecmp(type, "writeParam"))
		return RISEFrameWriteParam;
    return RISEFrameNoType;
}

MRISERspType MicrophoneModule::getRspFrameType(MRISEFrameType type){
    switch(type){
	case RISEFrameReadData :
		return RISERspReadData;
	case RISEFrameBitChange :
		return RISERspBitChange;	
	case RISEFrameReadParam :
		return RISERspReadParam;
	case RISEFrameWriteParam :
		return RISERspWriteParam;
	default:
		break;
	}
	return RISERspNoReq;
}

RISERcvRegType MicrophoneModule::getRcvRegType(const char *type){
    if (!strcasecmp(type, "yx")){
		return RISERcvRegYx;
	}
	else if (!strcasecmp(type, "yx1")){
		return RISERcvRegYx1;
	}
	else if (!strcasecmp(type, "doubleyx")){
		return RISERcvRegDoubleYx;
	}
	else if(!strcasecmp(type, "param")){
		return RISERcvRegParam;
	}
	else if(!strcasecmp(type, "null")){
		return RISERcvRegNull;
	}
	else if(!strcasecmp(type, "event1")){
		return RISERcvRegEvent1;
	}
	else if(!strcasecmp(type, "event2")){
		return RISERcvRegEvent2;
	}
	else if (!strcasecmp(type, "yc")){
		return RISERcvRegYc;
	}
	else if (!strcasecmp(type, "yc1")){
		return RISERcvRegYc1;
	}
    return RISERcvRegNull;
}

RISESndRegType MicrophoneModule::getSndRegType(const char *type){
    if (!strcasecmp(type, "uint8_t")){
		return RISESndRegINT8U;
	}
	else if(!strcasecmp(type, "uint16_t")){
		return RISESndRegINT16U;
	}
    return RISESndRegNull;
}


