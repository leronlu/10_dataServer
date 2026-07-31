#ifndef _MICROPHONEMODULE_H_
#define _MICROPHONEMODULE_H_

#include <string>
#include <list>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <functional>

#include <sys/statvfs.h>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <mutex> 
#include <cmath>

// #include "typedef.h" removed
#include "log_manage.h"
#include "xml_parser.h"
#include "date_manage.h"
#include "RtuBaseClass.h"

#include "IPCMsg.h"


#define mAbs(x, y)  (((x) > (y)) ? ((x) - (y)) : ((y) - (x)))


//定义应答类型
enum MRISERspType 
{
    RISERspNoReq,                                                                   //无请求
    RISERspReadData,                                                             	//读数据
    RISERspBitChange,                                                      			//变位上报
	//RISERspTick,                                                      				//心跳
	//RISERspDomain, 																	//域名
	RISERspWriteParam,																//写参数
	RISERspReadParam,																//读参数
	RISERspNoDeal
};

//定义应答状态
enum MRISERspStat
{
    RISERspWaiting,                                                                 //等待应答中
    RISERspFinished                                                                 //应答结束(超时,正确、错误应答)
};

//定义帧类型
enum MRISEFrameType
{
    RISEFrameNoType,                                                                //无该种帧类型
    RISEFrameReadData,                                                              //读数据
    RISEFrameBitChange,                                                             //变位
	//RISEFrameTick,                                                               	//心跳
	//RISEFrameDoman,                                                                 //域名
    RISEFrameReadParam,                                                             //读参数
    RISEFrameWriteParam,                                                           	//写参数
};


//定义解析的数据类型
enum MRISEParseType{
	RISEFrameParseDataNoType = 1,
	RISEFrameParseDataVersion,													    //版本号类型
};

//
enum RISESndRegType{
	RISESndRegNull,
	RISESndRegINT8U,
	RISESndRegINT16U
};

//
enum RISERcvRegType{
	RISERcvRegNull,
	RISERcvRegYx,
	RISERcvRegYx1,
	RISERcvRegDoubleYx,
	RISERcvRegParam,
	RISERcvRegEvent1,
	RISERcvRegEvent2,
	RISERcvRegYc,
	RISERcvRegYc1,
};

//定义data格式
struct MRISEParseData                                                            
{
	MRISEParseData()
	{
		regIndex = 0;
		THV = 0;
		compensate = 0;
		factor = 0.0;
		deviceId = 0;
	}
    uint8_t   regIndex;                                                           //数据单元在数据流中序号
    uint16_t  THV;                                                                //门限值
    //MRISEParseType	dataType;													//数据类型
    int16_t  compensate;                                                         //加减型补偿系数
    float  factor;																//比列系数
	uint8_t deviceId;
};


//定义数据表
struct MRISEData
{
    std::vector<DataINT8UNew>    singleYx;                                           //单点遥信
    std::vector<DataINT8UNew>    doubleYx;                                           //双点遥信                                               //参数量
	std::vector<DataINT16S>      Yc;                                                 //16位遥测
    std::vector<DataINT32S>      Ym;                                                 //计量值
    std::vector<DataFP32>        floatYc;                                            //短浮点遥测值
    std::vector<DataINT16Yt>      yt;                                                 //参数量
};

//定义基址表
struct ACBaseAddr
{
    BaseAddrOffset          yxBaseAddr;                                         //遥信基址
    BaseAddrOffset          ycBaseAddr;                                         //遥测基址
    BaseAddrOffset          ykBaseAddr;                                         //遥控基址
    BaseAddrOffset          ytBaseAddr;                                         //遥调基址
    BaseAddrOffset          cirBaseAddr;                                        //回路基址
};

class MicrophoneModule: public RtuBaseClass
{
public:
	MicrophoneModule(const std::string &confFileName);

	// 删除拷贝构造函数和赋值运算符
    MicrophoneModule(const MicrophoneModule&) = delete;
    MicrophoneModule& operator=(const MicrophoneModule&) = delete;
	
	~MicrophoneModule();

	virtual void 	initModule(); 
    virtual void    run();
	
	virtual void setBaseAddr(const BaseDataConfig &baseAddr);
	virtual void    loadConfig();
	virtual BaseDataConfig getConfig();

protected:
    void   initConfig();
private:
	static void * 	runIPCProcess(void *arg);
private:
	bool 	processYcMsg(std::vector<uint16_t> pressure);
	bool 	processYxMsg(std::vector<uint16_t> status);
	bool 	processYxEvent(uint16_t dataOffset, uint8_t yxSt, int8_t* info, bool firstPool);
	bool 	IPCProcess();
private:
    virtual bool getDoubleYxSoe(DataSoe &soe);                                  //获取双点遥信
    virtual bool getSingleYxSoe(DataSoe &soe);                                  //获取单点遥信SOE
    virtual bool getValidYcSoe(DataSoe &soe);                                  //获取遥测soe
    
    virtual bool getDoubleYxData(const uint16_t addr, 
                                 uint8_t &value, bool change = false);            //获取双点遥信
    virtual bool getSingleYxData(const uint16_t addr, 
                                 uint8_t &value, bool change = false);            //获取单点遥信

	virtual bool getValidYcData(const uint16_t addr, 
								 uint16_t &value, bool change = false);
private:
    void    putSoeToFifo(uint16_t addr, int16_t value, STLDeque<DataSoe> &soefifo);
	void 	putSoeToFifo(uint16_t addr, int16_t value, uint8_t type, STLDeque<DataSoe> &soefifo);
	MRISEFrameType getFrameType(const char *type);
	MRISEParseType getParseFrameType(const char *parseType);
	MRISERspType getRspFrameType(MRISEFrameType type);
	RISERcvRegType getRcvRegType(const char *type);
	RISESndRegType getSndRegType(const char *type);
private:
	std::string              configFileName;
    ACBaseAddr    		m_baseAddrOffset;
	BaseDataConfig      m_baseDataConfig;
private:
	MRISEData           m_data;                                                 //数据表 
	STLDeque<DataSoe> 	m_singleYxSoeFifo;                                  //单点遥信SOE队列
    STLDeque<DataSoe>   m_doubleYxSoeFifo;                                  //双点遥信SOE队列
    STLDeque<DataSoe>   m_dYxSoeFifo;                                		//遥信SOE队列,用于触发向下发送查询soe数目帧
	STLDeque<DataSoe>   m_validYcSoeFifo;                                   //遥测SOE队列
};

#endif //_MICROPHONEMODULE_H_


