using namespace std;
#include <unistd.h>
/******************************************************************************
@ Copy Right		: 2011 Beijing Togest Automation System Equipment Co.,Lt 
@ File Name 		: FileManageModule.h
@ Author     		: Li Jiazhen
@ Version    		: V1.0.0
@ Last Modify		: 20140611
@ Description		: 
-------------------------------------------------------------------------------
@ Modified History	: 
*******************************************************************************/
#include "log_manage.h"
#include "FileManageModule.h"
#include "xml_parser.h"

#define fileManageModuleConfig "../conf/FileManageConfig.xml"

FileManageModule * FileManageModule::module = NULL;

/*******************************************************************************
@ Function Name : FileManageModule
@ Description   : �ļ�����ģ�鹹�캯��,���ؽ��������ļ�
@ Input         : none
@ Output        : None;
@ Return        : none
*******************************************************************************/
FileManageModule::FileManageModule()
{
    _fileList.resize(RtuFileTypeEnd);
    loadConfig();
}

/*******************************************************************************
@ Function Name : loadConfig
@ Description   : ���ؽ��������ļ�
@ Input         : none
@ Output        : None;
@ Return        : none
*******************************************************************************/
void FileManageModule::loadConfig()
{
    XmlNodeParser f_XmlNodeParser((int8_t *)fileManageModuleConfig, (int8_t *)"/FileManageConfig");
    int8_t text[128] = "";
    int32_t value   = 0;
    int8_t path[64] = "";
	bool enable = false;

    printfs(LOG_INFO, "加载FileManageConfig.xml...");
    if (f_XmlNodeParser.FindNode((int8_t *)"//WaveFile"))
    {
        f_XmlNodeParser.GetProperty((int8_t *)"enable", text);
    	enable = f_XmlNodeParser.StrToBoolean(text);
        if (enable)
        {
            FileConfig conf;

            conf.enable = f_XmlNodeParser.StrToBoolean(text);
            f_XmlNodeParser.GetProperty((int8_t *)"dir", text);
            conf.directoryName = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"maxnum", value);
            conf.maxBackupIndex= value;

            f_XmlNodeParser.GetChildContent((int8_t *)"spontReport", text);
            _fileList[RtuFileTypeWaveFile].spontReport  = f_XmlNodeParser.StrToBoolean(text);
            _fileList[RtuFileTypeWaveFile].enable       = conf.enable;
            _fileList[RtuFileTypeWaveFile]._pFileManage = new WaveFileManage(conf.directoryName,
                                                                               conf.maxBackupIndex);
        }
    }
    if (f_XmlNodeParser.FindNode((int8_t *)"//WebWaveFile"))
    {
        f_XmlNodeParser.GetProperty((int8_t *)"enable", text);
    	enable = f_XmlNodeParser.StrToBoolean(text);
        if (enable)
        {
            FileConfig conf;

            conf.enable = f_XmlNodeParser.StrToBoolean(text);
            f_XmlNodeParser.GetProperty((int8_t *)"dir", text);
            conf.directoryName = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"maxnum", value);
            conf.maxBackupIndex= value;

            f_XmlNodeParser.GetChildContent((int8_t *)"spontReport", text);
            _fileList[RtuFileTypeWebWaveFile].spontReport  = f_XmlNodeParser.StrToBoolean(text);
            _fileList[RtuFileTypeWebWaveFile].enable       = conf.enable;
            _fileList[RtuFileTypeWebWaveFile]._pFileManage = new WaveFileManage(conf.directoryName,
                                                                               conf.maxBackupIndex);
        }
    }

    if (f_XmlNodeParser.FindNode((int8_t *)"//SoeFile"))
    {
        f_XmlNodeParser.GetProperty((int8_t *)"enable", text);
    	enable = f_XmlNodeParser.StrToBoolean(text);
        if (enable)
        {
            FileConfig conf;
            FileManageConfig manageConf;

            manageConf.enable = f_XmlNodeParser.StrToBoolean(text);
            
            f_XmlNodeParser.GetProperty((int8_t *)"dir", text);
            conf.directoryName = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"namePrefix", text);
            conf.namePrefix    = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"fileSize", value);
            conf.maxFileSize   = value;
            f_XmlNodeParser.GetProperty((int8_t *)"maxnum", value);
            conf.maxBackupIndex= value;

            f_XmlNodeParser.GetChildContent((int8_t *)"spontReport", text);
            manageConf.spontReport  = f_XmlNodeParser.StrToBoolean(text);
            f_XmlNodeParser.GetChildContent((int8_t *)"synchNum", value);
            manageConf.syncNum      = value;

            if (f_XmlNodeParser.FindNode((int8_t *)"//SoeFile/typeList"))
            {
                int cnt = f_XmlNodeParser.GetChildCounter("type");
                for (int i=1; i<=cnt; i++)
                {
                    sprintf((char *)path, "//SoeFile/typeList/type[%d]", i);
                    f_XmlNodeParser.FindNode(path);
                    f_XmlNodeParser.GetContent(value);
                    manageConf.filterList.push_back((int)value);
                }
            }

            manageConf._pFileManage = new RollSizeFile(conf.directoryName,
                                                  conf.namePrefix,
                                                  0,
                                                  conf.maxFileSize,
                                                  conf.maxBackupIndex);
            
            _fileList[RtuFileTypeSoeList] = manageConf;
        }
    }
    if (f_XmlNodeParser.FindNode((int8_t *)"//AnalogFile"))
    {
        f_XmlNodeParser.GetProperty((int8_t *)"enable", text);
    	enable = f_XmlNodeParser.StrToBoolean(text);
        if (enable)
        {
            FileConfig conf;
            FileManageConfig manageConf;

            manageConf.enable = f_XmlNodeParser.StrToBoolean(text);
            
            f_XmlNodeParser.GetProperty((int8_t *)"dir", text);
            conf.directoryName = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"namePrefix", text);
            conf.namePrefix    = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"fileSize", value);
            conf.maxFileSize   = value;
            f_XmlNodeParser.GetProperty((int8_t *)"maxnum", value);
            conf.maxBackupIndex= value;

            f_XmlNodeParser.GetChildContent((int8_t *)"spontReport", text);
            manageConf.spontReport  = f_XmlNodeParser.StrToBoolean(text);
            f_XmlNodeParser.GetChildContent((int8_t *)"synchNum", value);
            manageConf.syncNum      = value;
            f_XmlNodeParser.GetChildContent((int8_t *)"savePeriod", value);
            manageConf.savePeriod   = value;

            if (f_XmlNodeParser.FindNode((int8_t *)"//AnalogFile/addrList"))
            {
                int cnt = f_XmlNodeParser.GetChildCounter("addr");
                for (int i=1; i<=cnt; i++)
                {
                    sprintf((char *)path, "//AnalogFile/addrList/addr[%d]", i);
                    f_XmlNodeParser.FindNode(path);
                    f_XmlNodeParser.GetContent(value);
                    manageConf.filterList.push_back((int)value);
                }
            }

            manageConf._pFileManage = new RollSizeFile(conf.directoryName,
                                                  conf.namePrefix,
                                                  0,
                                                  conf.maxFileSize,
                                                  conf.maxBackupIndex);
            
            _fileList[RtuFileTypeAnalogList] = manageConf;
        }
    }
    if (f_XmlNodeParser.FindNode((int8_t *)"//ErrReportFile"))
    {
        f_XmlNodeParser.GetProperty((int8_t *)"enable", text);
    	enable = f_XmlNodeParser.StrToBoolean(text);
        if (enable)
        {
            FileConfig conf;
            FileManageConfig manage;
            
            f_XmlNodeParser.GetProperty((int8_t *)"dir", text);
            conf.directoryName = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"namePrefix", text);
            conf.namePrefix    = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"fileSize", value);
            conf.maxFileSize   = value;
            f_XmlNodeParser.GetProperty((int8_t *)"maxnum", value);
            conf.maxBackupIndex= value;

            manage.enable      = f_XmlNodeParser.StrToBoolean(text);
            f_XmlNodeParser.GetChildContent((int8_t *)"spontReport", text);
            manage.spontReport = f_XmlNodeParser.StrToBoolean(text);
            f_XmlNodeParser.GetChildContent((int8_t *)"synchNum", value);
            manage.syncNum     = value;

            manage._pFileManage= new RollSizeFile(conf.directoryName,
                                                  conf.namePrefix,
                                                  0,
                                                  conf.maxFileSize,
                                                  conf.maxBackupIndex);
            _fileList[RtuFileTypeErrReportList] = manage;
        }
    }
	if (f_XmlNodeParser.FindNode((int8_t *)"//VideoFile"))
    {
    	f_XmlNodeParser.GetProperty((int8_t *)"enable", text);
    	enable = f_XmlNodeParser.StrToBoolean(text);
        if (enable)
        {
            FileConfig conf;
            FileManageConfig manage;
            
            f_XmlNodeParser.GetProperty((int8_t *)"dir", text);
            conf.directoryName = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"namePrefix", text);
            conf.namePrefix    = string((char *)text);
            f_XmlNodeParser.GetProperty((int8_t *)"fileSize", value);
            conf.maxFileSize   = value;
            f_XmlNodeParser.GetProperty((int8_t *)"maxnum", value);
            conf.maxBackupIndex= value;

            manage.enable      = f_XmlNodeParser.StrToBoolean(text);
            f_XmlNodeParser.GetChildContent((int8_t *)"spontReport", text);
            manage.spontReport = f_XmlNodeParser.StrToBoolean(text);
            f_XmlNodeParser.GetChildContent((int8_t *)"synchNum", value);
            manage.syncNum     = value;

            manage._pFileManage= new RollSizeFile(conf.directoryName,
                                                  conf.namePrefix,
                                                  0,
                                                  conf.maxFileSize,
                                                  conf.maxBackupIndex);
            _fileList[RtuFileTypeVideoFile] = manage;
        }
    }
}

/*******************************************************************************
@ Function Name : getFileManageByName
@ Description   : ������ģ���ṩ�ļ�������
@ Input         : none
@ Output        : None;
@ Return        : none
*******************************************************************************/
FileManageModule * FileManageModule::getFileManageModule()
{
    if (!module)
    {
        module = new FileManageModule();
    }

    return module;
}

/*******************************************************************************
@ Function Name : getFileMangeList
@ Description   : ������ģ���ṩ�ļ�������
@ Input         : none
@ Output        : None;
@ Return        : none
*******************************************************************************/
vector<FileManageConfig> * FileManageModule::getFileManageList()
{
    return &_fileList;
}

/*******************************************************************************
@ Function Name : WaveFileManage
@ Description   : ¼�������ļ��������̳���RollTimeFile
@ Input         : fileDir: �ļ�Ŀ¼(·��)
                  maxBackIndex:�ļ����ؾ���Ŀ
                  append:  �ļ��Ƿ���׷�ӷ�ʽд��
                  fileLock:�ļ��Ƿ����ӻ�����
@ Output        : None;
@ Return        : None;
*******************************************************************************/
WaveFileManage::WaveFileManage(const string &fileDirectroy,
                 int           maxBackupIndex,
                 bool          append,
                 bool          fileLock) :
             RollTimeFile(fileDirectroy, O_TRUNC, maxBackupIndex, append, fileLock)
{
 
}

/*******************************************************************************
@ Function Name : getFileList
@ Description   : ��ȡ��filter���˵��ļ����б�
@ Input         : filter: ������
@ Output        : None;
@ Return        : None;
*******************************************************************************/
vector<string> WaveFileManage::getFileList(const string &filter)
{
    vector<string> fileList;

    for (vector<string>::iterator it=_fileNameList.begin(); it!=_fileNameList.end(); it++)
    {
        if (!(it->find(filter)))
        {
            fileList.push_back(*it);
        }
    }

    return fileList;
}

/*******************************************************************************
@ Function Name : getFileCreateTime
@ Description   : ��ȡ�ļ�����ʱ��
@ Input         : fileName:�ļ���
                  date: ʱ��buff
@ Output        : None;
@ Return        : None;
*******************************************************************************/
void WaveFileManage::getFileCreateTime(const string &fileName, unsigned char *date)
{
    int fd = -1;

    fd = openFile(fileName, O_RDONLY, _fileLock);
	if( -1 == fd)
		return;
    read(fd, date, 7);
    
    close(fd);
}


