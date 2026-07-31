#include <vector>
#include <string>
/******************************************************************************
@ Copy Right		: 2011 Beijing Togest Automation System Equipment Co.,Lt 
@ File Name 		: filemanage.h
@ Author     		: Li Jiazhen
@ Version    		: V1.0.0
@ Last Modify		: 07/22/2013
@ Description		: ��������ļ�������
                      class RollingFile :���ݻؾ��ļ�������
-------------------------------------------------------------------------------
@ Modified History	:   
*******************************************************************************/
#ifndef PUBLIC_FILEMANAGE_H
#define PUBLIC_FILEMANAGE_H

extern "C"
{
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <sys/file.h>
}
#include <string>
#include <list>
#include <iomanip>
#include "mutex_manage.h"

using namespace std;

/*�����ļ�����������*/
enum FileManageType
{
    FileManageTypeNormal,                                                       //FileManage
    FileManageTypeSizeRoll,                                                     //RollSizeFile
    FileManageTypeTimeRoll,                                                     //RollTimeFile
    FileManageTypeWaveRoll                                                      //WaveFileManage
};

/**************************************************************************
@ Description:      �����ļ��ṹ
***************************************************************************/
struct FileConfig
{
    bool    enable;
    std::string  directoryName;                                                  //�ļ�����Ŀ¼
    int     maxBackupIndex;                                                 //���洢���
    int     maxFileSize;                                                    //�������ߴ�
    std::string  namePrefix;                                                     //�ļ���ǰ׺
};

/*��������*/
enum FileOpType
{
    FileOpTypeNone,
    FileOpTypeRead,
    FileOpTypeWrite
};
/*����fifo���ļ�������
  ����openFile��flags������ѡ���Ƕ�ȡ����д���ļ�����
*/
class FileManage
{
public:
    FileManage(const std::string &directoryPath, 
               int flags, 
               bool append = true,
               bool fileLock = true);
    FileManage():
        _flags(0), _fileLock(false), _append(false), _writeFd(-1), _readFd(-1),
            _change(false),_lockNum(0)
    {}
    virtual ~FileManage(){writeEnd(); readEnd();}

    static void  createDir(const std::string &dir);
    virtual void createDirectory(const std::string &directoryPath);                  //����ļ�Ŀ¼���������½�Ŀ¼
    //void    createFile(const std::string &fileName) {}                               //ʹ��openWriteFile�����ļ�����
    void    removeFile(const std::string &fileName, bool spontChange = true);
    void    renameFile(const std::string &fileName, const std::string &dstFileName);
    void    addFileToList(const std::string &fileName);
    virtual void getFileCreateTime(const std::string &fileName, unsigned char *date);
    unsigned int getFileSize(const std::string &fileName, FileOpType optype=FileOpTypeNone);
    const std::vector<std::string> & getFileList();
    virtual std::vector<std::string> getFileList(const std::string &filter) { return _fileNameList;}
    const bool    isFileListChange() {return _change;}
    void    setFileListChange(bool st = false) {_change = st;}
    bool isFileNameListEmpty();

public:
    virtual bool    openWriteFile(const std::string &fileName);
	virtual int     openWriteFile(const std::string &fileName,int ret,char *devIpv4);
	virtual int		openWriteFile(const std::string &fileName,int ret);
    virtual size_t  writeFile(unsigned char *data, unsigned int len);           //д�ļ�
    virtual size_t  writeFile(int fd,unsigned char *data, unsigned int len);           //д�ļ�
    virtual void    writeEnd();
	virtual void    writeEnd(int fd);
//	virtual	void    writeEnd(int fd,bool fileListChangeCounterEnable);
    virtual size_t  getWriteLength();
	virtual size_t  getWriteLength(int fd);
//    int     _FileListChangeCounter;

public:    
    virtual bool    openReadFile(const std::string &fileName);
    virtual size_t  readFile(unsigned char *data, unsigned int len);            //���ļ�
    virtual void    readEnd();
    virtual bool    isEOF();
    virtual size_t  getReadLength();
    virtual void    setReadPos(size_t pos);

protected:
    int     openFile(const std::string &fileName, int flags, bool fileLock,char *devIpv4);         //���ļ�
    int     openFile(const std::string &fileName, int flags, bool fileLock);         //���ļ�
    bool    findFile(const std::string &fileName, int * pos = NULL);

protected:
    std::string  _directoryName;
    int     _flags;                                                             //�ļ�������ʽ(2���ơ��ı�)
    bool    _fileLock;
    bool    _append;
    std::string  _writeFileName;                                                     //��д���ļ��ļ���
    int     _writeFd;
    std::string  _readFileName;                                                      //�������ļ��ļ���
    int     _readFd;
    std::vector<std::string> _fileNameList;                                                 //�ļ��б�
    bool    _change;
	int		_lockNum;															//�������ļ�������
	//myMutex   f_mutex;
};

//���ļ���СΪ���ݽ����ļ��Ļؾ�
class RollSizeFile : public FileManage
{
public:
    RollSizeFile(const std::string& fileDir, 
                const std::string& filePre,
                int   flags = 0,
                size_t maxFileSize = 10*1024, 
                int    maxBackupIndex = 1,
                bool append = true,
                bool fileLock = true);
    ~RollSizeFile() {}

public:
    virtual void createDirectory(const std::string &directoryPath);                  //����ļ�Ŀ¼���������½�Ŀ¼
    virtual size_t writeFile(unsigned char *data, unsigned int len);            //д������

private:
    void    rollOver();                                                         //�ؾ�����

private:
    std::string  _fileName;                                                          //ȫ�ļ���
    size_t  _maxFileSize;                                                       //���洢����
    int     _maxBackupIndex;                                                    //�ؾ����
    int     _maxBackupIndexWidth;                                               //backupIndex��Ϊ�ļ���׺��ʱ����
};

//����ʱ��˳������ļ��Ļؾ�
class RollTimeFile : public FileManage
{
public:
    RollTimeFile(const std::string &fileDirectroy,
                 int           flags,
                 int           maxBackupIndex,
                 bool          append = true,
                 bool          fileLock = true);
    ~RollTimeFile() {}

    virtual void createDirectory(const std::string &directoryPath);                  //����ļ�Ŀ¼���������½�Ŀ¼
    virtual bool    openWriteFile(const std::string &fileName);                      //���Ǽ̳еķ����ڴ����ļ�ʱ���лؾ�����
    virtual int    openWriteFile(const std::string &fileName,int ret,char *devIpv4); //���Ǽ̳еķ����ڴ����ļ�ʱ���лؾ�����
    virtual int    openWriteFile(const std::string &fileName,int ret);				 //���Ǽ̳еķ����ڴ����ļ�ʱ���лؾ�����
private:
    void    rollOver();
    
private:
    int     _maxBackupIndex;
	myMutex   f_mutex;
};

#endif

