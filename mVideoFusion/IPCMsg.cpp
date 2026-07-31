#include <unistd.h>
using namespace std;
#include "IPCMsg.h"


/*******************************************************************************
@ Function Name     : sndIPCMsg
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int getIPCMsg(int type, const char *pathname)
{
	key_t key = ftok(pathname, type);  											// 生成唯一key

	if (key == -1) {
	    std::cerr << "ftok failed: " << strerror(errno) << std::endl;
	} else {
	    std::cout << "Generated key: " << key << std::endl;
	}
	int msgid = msgget(key, 0666 | IPC_CREAT);									// 创建消息队列
	if (msgid == -1) {
		std::cerr << "msgget failed: " << strerror(errno) << std::endl;
	} else {
		std::cout << "Message queue ID: " << msgid << std::endl;
	}
	return msgid;
}

/*******************************************************************************
@ Function Name     : sndIPCMsg
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int sndIPCMsg(int msgid, vector<uint16_t> &data)
{
	RTUMessage message;
	message.mtype = 1;															// 设置消息类型
	memcpy(message.data, data.data(), sizeof(message.data));

	msgsnd(msgid, &message, sizeof(message.data), 0);  							// 发送消息

	return 0;
};

/*******************************************************************************
@ Function Name     : sndIPCMsg1
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int sndIPCMsg1(int msgid, vector<uint16_t> &data)
{
    RTUMessage message;
    message.mtype = 1;  														// 设置消息类型 
    memcpy(message.data, data.data(), sizeof(message.data));

    int retry_count = 3; 														// 最大重试次数

    while (retry_count > 0) {
        int result = msgsnd(msgid, &message, sizeof(message.data), IPC_NOWAIT);
        
        if (result == 0) 
		{
        	return 0;
        } else {
            if (errno == EAGAIN) {
                //std::cerr << "消息队列已满，等待重试..." << std::endl;
                usleep(1*1000); 
            } else {
                std::cerr << "msgsnd 失败，错误码: " << errno << std::endl;
                break; 
            }
        }
        retry_count--;
    }
    //std::cerr << "消息发送失败，重试次数已用完" << std::endl;
    return -1; // 返回失败
}

/*******************************************************************************
@ Function Name     : sndPrpdMsg1
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int sndPrpdMsg1(int msgid, vector<float> &data)
{
    PrpdMessage message;
    message.mtype = 1;  														// 设置消息类型 
    memcpy(message.data, data.data(), sizeof(message.data));

    int retry_count = 3; 														// 最大重试次数

    while (retry_count > 0) {
        int result = msgsnd(msgid, &message, sizeof(message.data), IPC_NOWAIT);
        
        if (result == 0) 
		{
        	return 0;
        } else {
            if (errno == EAGAIN) {
                //std::cerr << "消息队列已满，等待重试..." << std::endl;
                usleep(1*1000); 
            } else {
                std::cerr << "msgsnd 失败，错误码: " << errno << std::endl;
                break; 
            }
        }
        retry_count--;
    }
    //std::cerr << "消息发送失败，重试次数已用完" << std::endl;
    return -1; // 返回失败
}


/*******************************************************************************
@ Function Name     : sndIPCMsg
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int rcvIPCMsg(int msgid, RTUMessage &message)
{
	return msgrcv(msgid, &message, sizeof(message.data), 1, IPC_NOWAIT);  				// 接收消息
}


/*******************************************************************************
@ Function Name     : rcvPrpdMsg
@ Description       : 
@ Input             : frame  
@ Output            : None;
@ Return            : None;
*******************************************************************************/
int rcvPrpdMsg(int msgid, PrpdMessage &message)
{
	return msgrcv(msgid, &message, sizeof(message.data), 1, IPC_NOWAIT);  				// 接收消息
}



