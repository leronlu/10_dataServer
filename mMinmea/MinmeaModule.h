#ifndef _MINMEAMODULE_H_
#define _MINMEAMODULE_H_

#include <string>
#include <mutex>
#include <vector>
#include "RtuBaseClass.h"
#include "serial_module.h"
#include "date_manage.h"
#include "MicrophoneModule.h"
#include "MinmeaProtocol.h"

enum {
    YcIndexLatitude = 0,
    YcIndexLongitude = 1,
    YcIndexYear = 2,
    YcIndexMonth = 3,
    YcIndexDay = 4,
    YcIndexHour = 5,
    YcIndexMinute = 6,
    YcIndexSecond = 7,
    YcCount = 8
};

class MinmeaModule: public RtuBaseClass
{
public:
    MinmeaModule(const std::string &confFileName);
	// 删除拷贝构造函数和赋值运算符
    MinmeaModule(const MinmeaModule&) = delete;
    MinmeaModule& operator=(const MinmeaModule&) = delete;
	~MinmeaModule();

	virtual void 	initModule(); 
    virtual void    run();
	
	virtual void setBaseAddr(const BaseDataConfig &baseAddr);
	virtual void    loadConfig();
	virtual BaseDataConfig getConfig();
protected:
    void   initConfig();
private:
	static void * 	runYcProcess(void *arg);
private:
    bool    ycProcess();
	bool 	processYcMsg();
private:
    virtual bool getFP32YcSoe(DataFP32Soe &soe);
    virtual bool getFP32YcData(const uint16_t addr, float &value, bool change = false);
private:
	std::string              configFileName;
    ACBaseAddr    		m_baseAddrOffset;
	BaseDataConfig      m_baseDataConfig;
private:
    MRISEData           m_data;                                                 //数据表 
	STLDeque<DataFP32Soe>   m_fpYcSoeFifo;                                   //遥测SOE队列
private:
    SerialProcess serialProcess;
    SerialModule serialModule;
    MinmeaProtocol* minmeaProtocol;
    std::vector<DataFP32> m_fpyc; 
};


#endif