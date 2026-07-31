using namespace std;
#include <unistd.h>
#include <dirent.h>
/******************************************************************************
@ Copy Right		: 2011 Beijing Togest Automation System Equipment Co.,Lt 
@ File Name 		: filemanage.h
@ Author     		: Li Jiazhen
@ Version    		: V1.0.0
@ Last Modify		: 07/22/2013
@ Description		: 
-------------------------------------------------------------------------------
@ Modified History	: 
*******************************************************************************/
#include "process_manage.h"
#include "log_manage.h"
#include "date_manage.h"
#include "filemanage.h"
#include "process_manage.h"

/*******************************************************************************
@ Function Name : openFile
@ Description   : 打开文件
@ Input         : fileName :文件名
                  flags:    文件打开选项
                  fileLock: 是否加锁
@ Output        : None;
@ Return        : 成功打开的文件描述符
*******************************************************************************/
FileManage::FileManage(const string &directoryPath, 
               int flags, 
               bool append,
               bool fileLock) :
            _directoryName(directoryPath),
            _flags(flags),
            _fileLock(fileLock),
            _append(append)
{
    //createDirectory(_directoryName);
    
    if (!_append)
        _flags |= O_TRUNC;

    _writeFd = -1;
    _readFd  = -1;
    _change  = false;
	_lockNum = 0;
//	_FileListChangeCounter = 0;
	
}

/*******************************************************************************
@ Function Name : createDir
@ Description   : 新建目录,如果文件目录不存在则新建
@ Input         : fileDir:文件目录名称
@ Output        : None;
@ Return        : None
*******************************************************************************/
void FileManage::createDir(const string &dir)
{
    struct stat dirSt;

    if ((stat(dir.c_str(), &dirSt) == -1) && (errno == ENOENT))             //目录不存在
    {
        ProcessService process;
        string cmd = string("mkdir ") + dir;
        process.Vsystem(cmd.c_str());
    }
}

/*******************************************************************************
@ Function Name : createDir
@ Description   : 新建目录,如果文件目录不存在则新建
@ Input         : fileDir:文件目录名称
@ Output        : None;
@ Return        : None
*******************************************************************************/
void FileManage::createDirectory(const string & directoryPath)
{
    struct stat dirSt;

    if ((stat(directoryPath.c_str(), &dirSt) == -1) && (errno == ENOENT))             //目录不存在
    {
        ProcessService process;
        string cmd = string("mkdir ") + directoryPath;
        process.Vsystem(cmd.c_str());
    }
    else
    {
        DIR *dir = NULL;
        struct dirent *ptr = NULL;

        dir = opendir(directoryPath.c_str());
        while ((ptr = readdir(dir)))
        {
            if (!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, ".."))
            {
                continue;
            }
            _fileNameList.push_back(string(ptr->d_name));
        }
        closedir(dir);
    }
}

/*******************************************************************************
@ Function Name : removeFile
@ Description   : 删除指定文件
@ Input         : fileName :文件名
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::removeFile(const string &fileName, bool spontChange)
{
    int it = 0;
//	int err = 0;
//    char  cmd[50]  = "";
	
    if (findFile(fileName, &it))
    {
        string filePath = _directoryName + fileName;

		
		//f_mutex.Lock();
		remove(filePath.c_str());
		//f_mutex.UnLock();

		#if 0
        if((err = )) == -1);
		{
	//			printfs(LOG_WARNING, "remove file :%s error,errno:%d\n",filePath.c_str(), errno);
		}
		sprintf(cmd, "rm -f %s",filePath.c_str());
		ProcessService process;
		process.Vsystem(cmd);
		#endif
		
        _fileNameList.erase(_fileNameList.begin() + it);
        if (spontChange)
        {
            _change = true;
//			_FileListChangeCounter -= 1;
        }
    }
}

/*******************************************************************************
@ Function Name : renameFile
@ Description   : 重命名指定文件
@ Input         : fileName :目标文件名
                  dstFileName: 待命名为的文件名
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::renameFile(const string &fileName, const string &dstFileName)
{
    for (vector<string>::iterator it=_fileNameList.begin(); it!=_fileNameList.end(); ++it)
    {
        if (!(it->compare(fileName)))
        {
            *it = dstFileName;

            string filePath = _directoryName + *it;
            string dstFilePath = _directoryName + dstFileName;
            rename(filePath.c_str(), dstFilePath.c_str());
            return ;
        }
    }
}

/*******************************************************************************
@ Function Name : addFileToList
@ Description   : 添加文件到filelist, 根据先后顺序添加
@ Input         : fileName :目标文件名
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::addFileToList(const string &fileName)
{
    _fileNameList.push_back(fileName);
}

/*******************************************************************************
@ Function Name : getFileCreateTime
@ Description   : 获取文件的创建时间
@ Input         : fileName :目标文件名
                  date:     时间
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::getFileCreateTime(const string &fileName, unsigned char *date)
{
    struct stat filestat;
    DateType  dateTime;
    
    if((stat(string(_directoryName+fileName).c_str(), &filestat)) == -1);

    dateTime = DateService::convTime_t(filestat.st_mtime);

    date[0]  = dateTime.m_msec;
    date[1]  = dateTime.m_sec;
    date[2]  = dateTime.m_min;
    date[3]  = dateTime.m_hour;
    date[4]  = dateTime.m_mday;
    date[5]  = dateTime.m_mon;
    date[6]  = dateTime.m_year;
}

/*******************************************************************************
@ Function Name : getFileSize
@ Description   : 获取文件长度
@ Input         : fileName :目标文件名
@ Output        : None
@ Return        : None
*******************************************************************************/
unsigned int FileManage::getFileSize(const string &fileName, FileOpType optype)
{
    int fd = -1;

    switch (optype)
    {
        case FileOpTypeRead: fd = _readFd; break;
        case FileOpTypeWrite: fd = _writeFd; break;
        default: break;
    }
    if (fd == -1)
    {
        fd = openFile(fileName, O_RDONLY, _fileLock);
    }
  
    off_t offset = ::lseek(fd, 0, SEEK_CUR);
    off_t size   = ::lseek(fd, 0, SEEK_END);

    ::lseek(fd, offset, SEEK_SET);
    
    if (!optype)
    {
        close(fd);
    }  

    return (unsigned int)size;
}

/*******************************************************************************
@ Function Name : getFileList
@ Description   : 返回文件名列表
@ Input         : none
@ Output        : None
@ Return        : None
*******************************************************************************/
const vector<string> & FileManage::getFileList()
{
    return _fileNameList;
}

/*******************************************************************************
@ Function Name : openWriteFile
@ Description   : 以写的方式打开文件
@ Input         : fileName: 文件名
@ Output        : None
@ Return        : None
*******************************************************************************/
bool FileManage::openWriteFile(const string &fileName)
{
    int flags          = O_CREAT | O_APPEND | O_WRONLY;

    removeFile(fileName, false);                                                       //删除同名文件

    _writeFileName = fileName;
//	f_mutex.Lock();
    _writeFd       = openFile(fileName.c_str(), _flags|flags, _fileLock);
//	f_mutex.UnLock();
    if (_writeFd != -1)
    {
        _fileNameList.push_back(fileName);
        
        return true;
    }

    return false;
}

/*******************************************************************************
@ Function Name : openWriteFile,重载
@ Description   : 以写的方式打开文件
@ Input         : fileName: 文件名
@ Output        : None
@ Return        : None
*******************************************************************************/
int FileManage::openWriteFile(const string &fileName, int ret,char *devIpv4)
{
    int flags          = O_CREAT | O_APPEND | O_WRONLY;
	int writeFd		   = -1;
	//int tempRet			   = 0;
	//tempRet 			   =ret;
    removeFile(fileName, false);                                                       //删除同名文件

    _writeFileName = fileName;

    writeFd       = openFile(fileName.c_str(), _flags|flags, _fileLock,devIpv4);
    if (writeFd != -1)
    {
        _fileNameList.push_back(fileName);
		if(devIpv4 == NULL){
			printfs(LOG_INFO, "文件列表长度 = %d", _fileNameList.size());
		}
		else{
			printfs(LOG_INFO, "%s:文件列表长度 = %d\n", devIpv4, _fileNameList.size());
		}
        return writeFd;
    }
    return -1;
}
/*******************************************************************************
@ Function Name : openWriteFile,重载
@ Description   : 以写的方式打开文件
@ Input         : fileName: 文件名
@ Output        : None
@ Return        : None
*******************************************************************************/
int FileManage::openWriteFile(const string &fileName, int ret)
{
    int flags          = O_CREAT | O_APPEND | O_WRONLY;
	int writeFd		   = -1;
	//int tempRet			   = 0;
	//tempRet 			   =ret;
    removeFile(fileName, false);                                                       //删除同名文件

    _writeFileName = fileName;

    writeFd       = openFile(fileName.c_str(), _flags|flags, _fileLock);
    if (writeFd != -1)
    {
        _fileNameList.push_back(fileName);
		printfs(LOG_INFO, "文件列表长度 = %d\n", _fileNameList.size());
        return writeFd;
    }
    return -1;
}
/*******************************************************************************
@ Function Name : writeFile
@ Description   : 写文件
@ Input         : data: 数据
                  len : 长度
@ Output        : None
@ Return        : None
*******************************************************************************/
size_t FileManage::writeFile(unsigned char *data, unsigned int len)
{
    return write(_writeFd, data, len);
}
/*******************************************************************************
@ Function Name : writeFile,重载
@ Description   : 写文件
@ Input         : data: 数据
                  len : 长度
@ Output        : None
@ Return        : None
*******************************************************************************/
size_t FileManage::writeFile(int fd, unsigned char *data, unsigned int len)
{
    return write(fd, data, len);
}
/*******************************************************************************
@ Function Name : getWriteLength
@ Description   : 获取已写入文件的长度
@ Input         : none
@ Output        : 长度
@ Return        : None
*******************************************************************************/
size_t FileManage::getWriteLength()
{
    return (size_t)::lseek(_writeFd, 0, SEEK_CUR);
}
/*******************************************************************************
@ Function Name : getWriteLength,重载
@ Description   : 获取已写入文件的长度
@ Input         : none
@ Output        : 长度
@ Return        : None
*******************************************************************************/
size_t FileManage::getWriteLength(int fd)
{
    return (size_t)::lseek(fd, 0, SEEK_CUR);
}
/*******************************************************************************
@ Function Name : writeEnd
@ Description   : 写文件完成
@ Input         : none
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::writeEnd()
{
    if (_fileLock)
    {
        flock(_writeFd, LOCK_UN);
    }
    close(_writeFd);

    _change  = true;
//	_FileListChangeCounter += 1;
    _writeFd = -1;
}
/*******************************************************************************
@ Function Name : writeEnd
@ Description   : 写文件完成
@ Input         : none
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::writeEnd(int fd)
{
    if (_fileLock)
    {
        flock(fd, LOCK_UN);
    }
    close(fd);

    _change  = true;
    fd = -1;
}
#if 0
/*******************************************************************************
@ Function Name : writeEnd
@ Description   : 写文件完成
@ Input         : none
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::writeEnd(int fd,bool fileListChangeCounterEnable)
{
    if (_fileLock)
    {
        flock(fd, LOCK_UN);
    }
    close(fd);

    _change  = true;
	_FileListChangeCounter ++; 
    fd = -1;
}
#endif
/*******************************************************************************
@ Function Name : openReadFile
@ Description   : 以只读方式打开文件
@ Input         : fileName: 文件名
@ Output        : None
@ Return        : None
*******************************************************************************/
bool FileManage::openReadFile(const string &fileName)
{
    if (!findFile(fileName))
    {
        _readFd = -1;
    }

    _readFileName      = fileName;
    _readFd            = openFile(fileName, O_RDONLY, _fileLock);

    return (_readFd != -1);
}/*******************************************************************************
@ Function Name : isFileNameListEmpty
@ Description   : 判断FileNameList是否为空
@ Input         : 无
@ Output        : None
@ Return        : None
*******************************************************************************/
bool FileManage::isFileNameListEmpty()
{
	if(0 == _fileNameList.size())
		return true;
	return false;
}
/*******************************************************************************
@ Function Name : readFile
@ Description   : 读取文件数据
@ Input         : data: 目标数据缓存区
                  len : 读取长度
@ Output        : None
@ Return        : None
*******************************************************************************/
size_t FileManage::readFile(unsigned char *data, unsigned int len)
{
    return read(_readFd, data, len);
}

/*******************************************************************************
@ Function Name : isEOF
@ Description   : 是否读到文件尾
@ Input         : none
@ Output        : true or false
@ Return        : None
*******************************************************************************/
bool FileManage::isEOF()
{
    off_t offset = ::lseek(_readFd, 0, SEEK_CUR);
    off_t endOffet = ::lseek(_readFd, 0, SEEK_END);

    ::lseek(_readFd, offset, SEEK_SET);
    
    return (offset == endOffet);
}

/*******************************************************************************
@ Function Name : readEnd
@ Description   : 读取文件结束
@ Input         : none
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::readEnd()
{
    if (_fileLock)
    {
        flock(_readFd, LOCK_UN);
    }
    close(_readFd);

    _readFd = -1;
}

/*******************************************************************************
@ Function Name : getReadLength
@ Description   : 获取读取位置
@ Input         : none
@ Output        : 读取位置
@ Return        : None
*******************************************************************************/
size_t FileManage::getReadLength()
{
    return ::lseek(_readFd, 0, SEEK_CUR);
}

/*******************************************************************************
@ Function Name : setReadPos
@ Description   : 设置读取位置
@ Input         : none
@ Output        : None
@ Return        : None
*******************************************************************************/
void FileManage::setReadPos(size_t pos)
{
    ::lseek(_readFd, (off_t)pos, SEEK_SET);
}

/*******************************************************************************
@ Function Name : openFile
@ Description   : 打开文件
@ Input         : fileName :文件名
                  flags:    文件打开选项
                  fileLock: 是否加锁
@ Output        : None;
@ Return        : 成功打开的文件描述符
*******************************************************************************/
int FileManage::openFile(const string &fileName, int flags, bool fileLock,char *devIpv4)
{
    int fd = -1;
    string filePath    = _directoryName + fileName;

    fd = open(filePath.c_str(), flags);
    if (fd == -1)
    {
        printfs(LOG_ERROR, "%-16s:创建文件:%s失败:errno=%d\n",devIpv4, fileName.c_str(), errno);
        return fd;
    }
    if (fileLock)
    {
        if (flock(fd, LOCK_EX|LOCK_NB) == -1)
        {
            printfs(LOG_ERROR, "%-16s:flock失败: %d\n",devIpv4, errno);
            close(fd);
            
            return -1;
        }
    }

    return fd;
}
/*******************************************************************************
@ Function Name : openFile
@ Description   : 打开文件
@ Input         : fileName :文件名
                  flags:    文件打开选项
                  fileLock: 是否加锁
@ Output        : None;
@ Return        : 成功打开的文件描述符
*******************************************************************************/
int FileManage::openFile(const string &fileName, int flags, bool fileLock)
{
    int fd = -1;
    string filePath    = _directoryName + fileName;

    fd = open(filePath.c_str(), flags);
    if (fd == -1)
    {
        printfs(LOG_ERROR, "打开文件:%s失败:errno=%d\n",fileName.c_str(), errno);
        return fd;
    }
	#if 0		
    if (fileLock)
    {
        if (flock(fd, LOCK_EX|LOCK_NB) == -1)
        {
            printfs(LOG_ERROR, "flock失败: %d\n", errno);
            close(fd);
            
            return -1;
        }
    }
	#endif
    return fd;
}
/*******************************************************************************
@ Function Name : openFile
@ Description   : 打开文件
@ Input         : fileName :文件名
                  flags:    文件打开选项
                  fileLock: 是否加锁
@ Output        : None;
@ Return        : 成功打开的文件描述符
*******************************************************************************/
bool FileManage::findFile(const string &fileName, int * pos)
{
    for (unsigned int it=0; it<_fileNameList.size(); ++it)
    {
        if (!(_fileNameList[it].compare(fileName)))
        {
            if (pos)
            {
                *pos = it;
            }
            return true;
        }
    }

    return false;
}

/*******************************************************************************
@ Function Name : RollingFile
@ Description   : 构造函数
@ Input         : fileDir: 文件目录(路径)
                  filePre: 文件前缀名. 文件名=fileDir+filePre+backupIndex
                  maxFileSize: 文件最大存储大小
                  maxBackIndex:文件最大回卷数目
                  append:  文件是否以追加方式写入
                  fileLock:文件是否添加互斥锁
@ Output        : None;
@ Return        : None;
*******************************************************************************/
RollSizeFile::RollSizeFile(const string& fileDir, 
                         const string& filePre,
                         int   flags,
                         size_t maxFileSize, 
                         int    maxBackupIndex,
                         bool append,
                         bool fileLock) :
              FileManage(fileDir, flags, append, fileLock)
{
    _fileName       = filePre;
    _maxFileSize    = maxFileSize;
    _maxBackupIndex = maxBackupIndex;
    _maxBackupIndexWidth = (_maxBackupIndex > 0) ? log10((float)_maxBackupIndex)+1 : 1;

    createDirectory(fileDir);
        
    _writeFd             = openWriteFile(_fileName);
}

/*******************************************************************************
@ Function Name : createDirectory
@ Description   : 覆盖基类接口
@ Input         : directoryPath:目录名称
@ Output        : None;
@ Return        : None
*******************************************************************************/
void RollSizeFile::createDirectory(const string &directoryPath)
{
    struct stat dirSt;

    if ((stat(directoryPath.c_str(), &dirSt) == -1) && (errno == ENOENT))             //目录不存在
    {
        ProcessService process;
        string cmd = string("mkdir ") + directoryPath;
        process.Vsystem(cmd.c_str());
    }
    else
    {
        DIR *dir = NULL;
        struct dirent *ptr = NULL;

        dir = opendir(directoryPath.c_str());
        while ((ptr = readdir(dir)))
        {
            if (strncmp(ptr->d_name, _fileName.c_str(), _fileName.length()))
            {
                continue;
            }

            vector<string>::iterator it;
            for (it=_fileNameList.begin(); it!=_fileNameList.end(); it++)
            {
                if (strcmp(ptr->d_name, it->c_str()) > 0)
                {
                    _fileNameList.insert(it, ptr->d_name);
                    break;
                }
            }
            if (it == _fileNameList.end())
            {
                _fileNameList.push_back(ptr->d_name);
            }
        }
        closedir(dir);
    }
}

/*******************************************************************************
@ Function Name : writeFile
@ Description   : 向打开的文件写入数据
@ Input         : data: 待写入的数据
                  len : 数据长度
@ Output        : None;
@ Return        : None
*******************************************************************************/
size_t RollSizeFile::writeFile(unsigned char *data, unsigned int len)
{
    ::write(_writeFd, data, len);
    
    off_t offset = ::lseek(_writeFd, 0, SEEK_END);

    if (offset < 0) 
    {
        printfs(LOG_ERROR, "lseek文件失败: %d", errno);
    } 
    else 
    {
        if (static_cast<size_t>(offset) >= _maxFileSize) 
        {
            rollOver();
        }
    }

    return len;
}
/*******************************************************************************
@ Function Name : rollOver
@ Description   : 文件回卷管理处理
@ Input         : 
@ Output        : None;
@ Return        : None
*******************************************************************************/
void RollSizeFile::rollOver()
{
    writeEnd();
    if (_maxBackupIndex > 0) 
    {
        std::ostringstream filename_stream;
    	filename_stream << _fileName << "." << std::setw( _maxBackupIndexWidth ) 
                        << std::setfill( '0' ) << _maxBackupIndex << std::ends;
    	// remove the very last (oldest) file
    	std::string last_log_filename = filename_stream.str();
        std::cout << last_log_filename << std::endl;

        removeFile(last_log_filename, false);
        
        // rename each existing file to the consequent one
        for (unsigned int i = _maxBackupIndex; i > 1; i--) 
        {
            filename_stream.str(std::string());
            filename_stream << _fileName << '.' << std::setw( _maxBackupIndexWidth ) 
                            << std::setfill( '0' ) << i - 1 << std::ends;  // set padding so the files are listed in order
            renameFile(filename_stream.str(), last_log_filename);
            last_log_filename = filename_stream.str();
        }
        // new file will be numbered 1
        renameFile(_fileName, last_log_filename);
    }
    
    _writeFd = openWriteFile(_fileName);
}

/*******************************************************************************
@ Function Name : RollTimeFile
@ Description   : 构造函数
@ Input         : fileDir: 文件目录(路径)
                  maxBackIndex:文件最大回卷数目
                  append:  文件是否以追加方式写入
                  fileLock:文件是否添加互斥锁
@ Output        : None;
@ Return        : None;
*******************************************************************************/
RollTimeFile::RollTimeFile(const string &fileDirectroy,
                 int           flags,
                 int           maxBackupIndex,
                 bool          append,
                 bool          fileLock) :
                 FileManage(fileDirectroy, flags, append, fileLock),
                 _maxBackupIndex(maxBackupIndex)
{
    createDirectory(fileDirectroy);
}

/*******************************************************************************
@ Function Name : createDirectory
@ Description   : 若目录不存在则新建目录，否则将已存在的文件按时间建立先后顺序
                    添加到list
@ Input         : directoryList
@ Output        : None;
@ Return        : None;
*******************************************************************************/
void RollTimeFile::createDirectory(const string &directoryPath)
{
    struct stat dirSt;

    if ((stat(directoryPath.c_str(), &dirSt) == -1) && (errno == ENOENT))             //目录不存在
    {
        ProcessService process;
        string cmd = string("mkdir ") + directoryPath;
        process.Vsystem(cmd.c_str());
    }
    else
    {
        DIR *dir = NULL;
        struct dirent *ptr = NULL;
        struct stat   buf;
        vector<time_t> timeList;
        
        dir = opendir(directoryPath.c_str());
        while ((ptr = readdir(dir)))
        {
            if (!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, ".."))
            {
                continue;
            }

            if (!stat(string(directoryPath+string(ptr->d_name)).c_str(), &buf))  //按时间先后顺序插入
            {
                vector<time_t>::iterator it;
                int pos = 0;

                for (it=timeList.begin(); it!=timeList.end(); it++, pos++)
                {
                    if (buf.st_mtime < *it)
                    {
                        timeList.insert(it, buf.st_mtime);
                        _fileNameList.insert(_fileNameList.begin()+pos, string(ptr->d_name));
                        break;
                    }
                }
                
                if (it == timeList.end())
                {
                    timeList.push_back(buf.st_mtime);
                    _fileNameList.push_back(string(ptr->d_name));
                }
            }
        }
        closedir(dir);
    }
}

/*******************************************************************************
@ Function Name : openWriteFile
@ Description   : 覆盖基类的openWriteFile,添加按时间管理文件个数
@ Input         : fileDir: 文件目录(路径)
                  maxBackIndex:文件最大回卷数目
                  append:  文件是否以追加方式写入
                  fileLock:文件是否添加互斥锁
@ Output        : None;
@ Return        : None;
*******************************************************************************/
bool RollTimeFile::openWriteFile(const string & fileName)
{
	bool ret = 0;
	f_mutex.Lock();
    if (_fileNameList.size() >= (unsigned int)_maxBackupIndex)
    {
        rollOver();
    }
	ret = FileManage::openWriteFile(fileName);
	f_mutex.UnLock();
    return ret;
}
/*******************************************************************************
@ Function Name : openWriteFile
@ Description   : 覆盖基类的openWriteFile,添加按时间管理文件个数
@ Input         : fileDir: 文件目录(路径)
                  maxBackIndex:文件最大回卷数目
                  append:  文件是否以追加方式写入
                  fileLock:文件是否添加互斥锁
@ Output        : None;
@ Return        : None;
*******************************************************************************/
int RollTimeFile::openWriteFile(const string & fileName, int ret,char *devIpv4)
{
	int tempRet =0;
	tempRet     = ret;
	int writeFd =  -1;
	f_mutex.Lock();
    while (_fileNameList.size() >= (unsigned int)_maxBackupIndex)
    {
        rollOver();
    }
	writeFd     = FileManage::openWriteFile(fileName,tempRet,devIpv4);
	f_mutex.UnLock();
    return writeFd;
	
}
/*******************************************************************************
@ Function Name : openWriteFile 重载
@ Description   : 覆盖基类的openWriteFile,添加按时间管理文件个数
@ Input         : fileDir: 文件目录(路径)
                  maxBackIndex:文件最大回卷数目
                  append:  文件是否以追加方式写入
                  fileLock:文件是否添加互斥锁
@ Output        : None;
@ Return        : None;
*******************************************************************************/
int RollTimeFile::openWriteFile(const string & fileName, int ret)
{
	int tempRet =0;
	tempRet     = ret;
	int writeFd =  -1;
	f_mutex.Lock();
    while (_fileNameList.size() >= (unsigned int)_maxBackupIndex)
    {
        rollOver();
    }
	writeFd     = FileManage::openWriteFile(fileName,tempRet);
	f_mutex.UnLock();
    return writeFd;
	
}
/*******************************************************************************
@ Function Name : rollOver
@ Description   : 根据时间先后顺序删除最旧的文件即list.begain();
@ Input         : none
@ Output        : None;
@ Return        : None;
*******************************************************************************/
void RollTimeFile::rollOver()
{
    removeFile(*_fileNameList.begin(), false);
}

