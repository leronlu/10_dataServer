#include <vector>
#include <string>
/******************************************************************************
@ Copy Right		: 2011 Beijing Togest Automation System Equipment Co.,Lt 
@ File Name 		: FileManageModule.h
@ Author     		: Li Jiazhen
@ Version    		: V1.0.0
@ Last Modify		: 20140611
@ Description		: 封装文件管理
-------------------------------------------------------------------------------
@ Modified History	:   
*******************************************************************************/
#ifndef PUBLIC_FILEMANAGEMODULE_H
#define PUBLIC_FILEMANAGEMODULE_H
#include <list>
#include <string>
#include "filemanage.h"

using namespace std;

/*设备文件类型*/
enum RtuFileType
{
    RtuFileTypeNormal = 1,
    RtuFileTypeWaveFile,
    RtuFileTypeSoeList,
    RtuFileTypeAnalogList,
    RtuFileTypeErrReportList,
    RtuFileTypeWebWaveFile,
    RtuFileTypeVideoFile,
    RtuFileTypeEnd
};

/**************************************************************************
@ Description:      定义录波文件结构描述
  TOV
  NOC
  NOE
  INT
  IxParamFactor
  UxParamFactor
  ctime[7]
  wavetype
  reason
  FAN
  circuitNum
  Ia channel id
  Ia data array
  Ib channel id
  Ib data array
  Ic channel id
  Ic data array
  Ua channel id
  Ua data array
  Ub channel id
  Ub data array
  Uc channel id
  Uc data array
***************************************************************************/
union FloatTypeValue
{
    float    fpValue;                                                            //float
    uint8_t   byteValue[4];                                                       //8bit array
};
struct WaveFileDataParam
{
    FloatTypeValue  fstRatedValue;                                              //一次额定值
    FloatTypeValue  sndRatedValue;                                              //二次额定值
    FloatTypeValue  paramFactor;                                                //参比因子
};
struct WaveFileDataInfo
{
    uint16_t  TOV;                                                                //值类型
    uint16_t  NOC;                                                                //通道数目
    uint16_t  NOE;                                                                //每通道信息元素数目
    uint16_t  INT;                                                                //信息元素间的间隔us
    WaveFileDataParam   IxParam;                                                //电压通道参比系数
    WaveFileDataParam   UxParam;                                                //电流通道参比系数
};

/*录波文件的管理类
    录波文件名: XXXXYYYY :xxxx:回路号, yyyy:故障序号
*/
class WaveFileManage : public RollTimeFile
{
public:
    WaveFileManage(const std::string &fileDirectroy,
                 int           maxBackupIndex,
                 bool          append = false,
                 bool          fileLock = true);
    ~WaveFileManage() {}

    virtual void getFileCreateTime(const std::string &fileName, unsigned char *date);
    virtual std::vector<std::string> getFileList(const std::string &filter);                     //
};

/**************************************************************************
@ Description:      定义录波文件结构描述

  TOV
  NOC
  NOE
  INT
  IxParamFactor
  UxParamFactor
  ctime[7]
  w
avetype
  reason

  
FAN
  circuitNum
  Ia chan
nel id
  Ia data array
 
 Ib channel id
  Ib data
 array
  Ic channel id
  I
    RtuFileTypeErrReportList,
    RtuFileTypeEnd t;
};
*/
/*统一管理各类文件,加载统一配置
*/
struct FileManageConfig
{
    FileManageConfig() :
        enable(false), spontReport(false), syncNum(0), savePeriod(0),
        _pFileManage(NULL)
    {
    }
    bool        enable;
    bool        spontReport;
    int         syncNum;
    int         savePeriod;
    std::vector<int> filterList;

    FileManage *_pFileManage;
};
/*实现文件管理的单例*/
class FileManageModule
{
private:
    FileManageModule();
    ~FileManageModule(){}

    FileManageModule(FileManageModule &module);    
    FileManageModule & operator= (FileManageModule &module);
        
public:
    static FileManageModule * getFileManageModule();
    std::vector<FileManageConfig> * getFileManageList();
    
private:
    void    loadConfig();

private:
    static FileManageModule *module;
    std::vector<FileManageConfig>  _fileList;
};

#endif

