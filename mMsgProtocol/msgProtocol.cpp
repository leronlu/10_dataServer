using namespace std;
#include "msgProtocol.h"
#include "direct_zmq_msg_manage.h"

void initDirectZmqMsgManage(DirectZmqMsgManage &mgr) {
    mgr.setConfigPath("../conf/ZmqCommConfig.xml");
    mgr.loadConfig();
}

uint16_t getMapUnitAddr(const IecMessage &msg, size_t idx, const std::string &srcIp) {
    if (idx >= msg.units.size()) return 0xffff;
    DataManager dm;
    return dm.getUpMapAddr(msg.typ, srcIp, msg.units[idx].addr);
}

Unit setMapUnitAddr(IecMessage &msg, size_t idx, uint16_t rawAddr) {
    DataManager dm;
    Unit u = dm.getDownMapAddr(msg.typ, (uint8_t)rawAddr);
    if (u.nodeId != 0xffff && idx < msg.units.size())
        msg.units[idx].addr = u.addr;
    return u;
}

AddrMap         DataManager::addrMap;

/*******************************************************************************
@ Function Name     : DataManager
@ Description       : 构造函数
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
DataManager::DataManager()
{

}

/*******************************************************************************
@ Function Name     : ~DataManager
@ Description       : 析构函数
@ Input             : None
@ Output            : None;
@ Return            : None;
*******************************************************************************/
DataManager::~DataManager()
{

}


/*******************************************************************************
@ Function Name     : AddrMapInit
@ Description       : 初始化地址映射
@ Input             : None
@ Output            :
                    :
@ Return            : None;
*******************************************************************************/
void DataManager::AddrMapInit(
	uint16_t addr,
	uint16_t mapAddr,
	uint16_t num,
	Unit unit,
	string type)
{
	char key[256]={0};
	char mapKey[256]={0};

	for(uint16_t m=0; m < num; m++)
	{
		if(type == string("singleyx")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "singleyx", (unit.addr));
			addrMap.singleYxUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d", "singleyx", (mapAddr+m));
			addrMap.singleYxDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
		else if(type == string("doubleyx")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "doubleyx", (unit.addr));
			addrMap.doubleYxUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d",  "doubleyx", (mapAddr+m));
			addrMap.doubleYxDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
		else if(type == string("yk")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "yk", (unit.addr));
			addrMap.ykUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d", "yk", (mapAddr+m));
			addrMap.ykDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
		else if(type == string("yc")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "yc", (unit.addr));
			addrMap.ycUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d", "yc", (mapAddr+m));
			addrMap.ycDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
		else if(type == string("ym")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "ym", (unit.addr));
			addrMap.ymUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d", "ym", (mapAddr+m));
			addrMap.ymDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
		else if(type == string("yt")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "yt", (unit.addr));
			addrMap.ytUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d", "yt", (mapAddr+m));
			addrMap.ytDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
		else if(type == string("boardcmd")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "boardcmd", (unit.addr));
			addrMap.boardCmdUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d", "boardcmd", (mapAddr+m));
			unit.ip="BROADIP";
			addrMap.boardCmdDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
		else if(type == string("localcmd")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "localcmd", (unit.addr));
			addrMap.localCmdUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d", "localcmd", (mapAddr+m));
			unit.ip="127.0.0.1";
			addrMap.localCmdDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
		else if(type == string("cmd")){
			unit.addr = addr+m;
			sprintf(key, "%s_%s_%d", unit.ip.c_str(), "cmd", (unit.addr));
			addrMap.cmdUp.insert(std::pair<string, uint16_t>(string(key), mapAddr+m));

			sprintf(mapKey, "%s_%d", "cmd", (mapAddr+m));
			unit.ip="127.0.0.1";
			addrMap.cmdDown.insert(std::pair<string, Unit>(string(mapKey), unit));
		}
	}
}

/*******************************************************************************
@ Function Name     : loadConfig
@ Description       : 解析配置文件
@ Input             : None
@ Output            : m_configTable: 配置表
                    : m_configTableNum: 配置表数目
@ Return            : None;
*******************************************************************************/
void DataManager::loadConfig()
{
    XmlNodeParser f_XmlNodeParser((int8_t *)UPSIDEDATACONF, (int8_t *)"/UpSideDataConfig");
    int8_t         nodePath[128] = "";
    int8_t         text[MAX_STRING_LEN] = "";
	int32_t 		value;

	//f_XmlNodeParser.FindNode((int8_t *)"/UpSideDataConfig/");

	uint8_t nodeLinkNum = f_XmlNodeParser.GetChildCounter("nodeList");

    for (uint8_t i=0; i<nodeLinkNum; i++)
    {
    	string ipv4;
		//uint32_t port;
    	sprintf((char *)nodePath, "/UpSideDataConfig/nodeList[%d]", i+1);
		f_XmlNodeParser.FindNode(nodePath);

		if(f_XmlNodeParser.GetProperty((int8_t *)"ip", text)){
			ipv4 = (char *)text;
		}
		uint8_t dataListNum = f_XmlNodeParser.GetChildCounter("dataList");
		for(uint8_t j = 0; j < dataListNum; j++)
		{
			string type;
			sprintf((char *)nodePath, "/UpSideDataConfig/nodeList[%d]/dataList[%d]", i+1, j+1);
			f_XmlNodeParser.FindNode(nodePath);

			if(f_XmlNodeParser.GetProperty((int8_t *)"type", text))
			{
				type = (char *)text;
			}

			uint8_t dataNum = f_XmlNodeParser.GetChildCounter("data");
			for(uint16_t k = 0; k < dataNum; k++){
				Unit unit;
				uint16_t addr = 0;
				uint16_t mapAddr = 0;
				uint16_t num = 0;
				sprintf((char *)nodePath, "/UpSideDataConfig/nodeList[%d]/dataList[%d]/data[%d]", i+1, j+1,k+1);
				f_XmlNodeParser.FindNode(nodePath);

				if(f_XmlNodeParser.GetProperty((int8_t *)"addr", value))
				{
					addr = value;
				}

				if(f_XmlNodeParser.GetProperty((int8_t *)"mapaddr", value))
				{
					mapAddr = value;
				}
				unit.addr 	= addr;
				unit.ip 	= ipv4;
				unit.nodeId	= i;
				if(f_XmlNodeParser.GetProperty((int8_t *)"num", value))
				{
					num = value;
					AddrMapInit(addr, mapAddr, num, unit, type);
				}
			}
		}
    }
}

/*******************************************************************************
@ Function Name     : getUpMapAddr
@ Description       : 获取上行映射数据地址
@ Input             : None
@ Output            : m_configTable: 配置表
                    : m_configTableNum: 配置表数目
@ Return            : None;
*******************************************************************************/
uint16_t DataManager::getUpMapAddr(uint8_t type, string srcIp, uint16_t infoAddr)
{
	string keyAddr;
	char key[256]={0};
	switch (type)
	{
		case MSG_M_SP_NA_1:
		case MSG_M_SP_TB_1:
			sprintf(key, "%s_%s_%d", srcIp.c_str(), "singleyx", infoAddr);
			keyAddr = string(key);
			if(addrMap.singleYxUp.find(keyAddr) != addrMap.singleYxUp.end())
			{
				return addrMap.singleYxUp[keyAddr];
			}
			break;
		case MSG_M_DP_NA_1:
		case MSG_M_DP_TB_1:
			sprintf(key, "%s_%s_%d", srcIp.c_str(), "doubleyx", infoAddr);
			keyAddr = string(key);
			if(addrMap.doubleYxUp.find(keyAddr) != addrMap.doubleYxUp.end())
			{
				return addrMap.doubleYxUp[keyAddr];
			}
			break;
		case MSG_M_ME_NB_1:
		case MSG_M_ME_TE_1:
		case MSG_M_ME_NC_1:
			sprintf(key, "%s_%s_%d", srcIp.c_str(), "yc", infoAddr);
			keyAddr = string(key);
			if(addrMap.ycUp.find(keyAddr) != addrMap.ycUp.end())
			{
				return addrMap.ycUp[keyAddr];
			}
			break;
		case MSG_M_IT_NA_1:
			break;
		case MSG_C_SC_NA_1:
		case MSG_C_DC_NA_1:
			sprintf(key, "%s_%s_%d", srcIp.c_str(), "yk", infoAddr);
			keyAddr = string(key);
			if(addrMap.ykUp.find(keyAddr) != addrMap.ykUp.end())
			{
				return addrMap.ykUp[keyAddr];
			}
			break;
		case MSG_P_ME_NB_2:
			sprintf(key, "%s_%s_%d", srcIp.c_str(), "yt", infoAddr);
			keyAddr = string(key);
			if(addrMap.ytUp.find(keyAddr) != addrMap.ytUp.end())
			{
				return addrMap.ytUp[keyAddr];
			}
			break;
		case MSG_C_IC_NA_1:
		case MSG_C_CS_NA_1:
			sprintf(key, "%s_%s_%d", srcIp.c_str(), "boardcmd", infoAddr);
			keyAddr = string(key);
			if(addrMap.boardCmdUp.find(keyAddr) != addrMap.boardCmdUp.end())
			{
				return addrMap.boardCmdUp[keyAddr];
			}
			break;
		case MSG_C_RD_NA_1:
		case MSG_C_RD_NB_1:
		case MSG_C_UP_NA_1:
		case MSG_C_RD_NC_1:
		case MSG_C_ST_NA_1:
		case MSG_C_MW_NA_1:
		case MSG_C_SV_NA_1:
		case MSG_C_MD_NA_1:
		case MSG_M_IC_NA_1:
		case MSG_M_UP_NA_1:
		case MSG_M_EC_NA_1:
			sprintf(key, "%s_%s_%d", srcIp.c_str(), "localcmd", infoAddr);
			keyAddr = string(key);
			if(addrMap.localCmdUp.find(keyAddr) != addrMap.localCmdUp.end())
			{
				return addrMap.localCmdUp[keyAddr];
			}
			break;
		default:
			sprintf(key, "%s_%s_%d", srcIp.c_str(), "cmd", infoAddr);
			keyAddr = string(key);
			if(addrMap.cmdUp.find(keyAddr) != addrMap.cmdUp.end())
			{
				return addrMap.cmdUp[keyAddr];
			}
			break;
	}
	return 0xffff;
}


/*******************************************************************************
@ Function Name     : getDownMapAddr
@ Description       : 获取下行映射数据地址
@ Input             : None
@ Output            : m_configTable: 配置表
                    : m_configTableNum: 配置表数目
@ Return            : None;
*******************************************************************************/
Unit DataManager::getDownMapAddr(uint8_t type, uint8_t infoAddr)
{
	string keyAddr;
	char key[256]={0};
	switch (type)
	{
		case MSG_M_SP_NA_1:
		case MSG_M_SP_TB_1:
			sprintf(key, "%s_%d",  "singleyx", infoAddr);
			keyAddr = string(key);
			if(addrMap.singleYxDown.find(keyAddr) != addrMap.singleYxDown.end())
			{
				return addrMap.singleYxDown[keyAddr];
			}
			break;
		case MSG_M_DP_NA_1:
		case MSG_M_DP_TB_1:
			sprintf(key, "%s_%d",  "doubleyx", infoAddr);
			keyAddr = string(key);
			if(addrMap.doubleYxDown.find(keyAddr) != addrMap.doubleYxDown.end())
			{
				return addrMap.doubleYxDown[keyAddr];
			}
			break;
		case MSG_M_ME_NB_1:
		case MSG_M_ME_TE_1:
		case MSG_M_ME_NC_1:
			sprintf(key, "%s_%d", "yc", infoAddr);
			if(addrMap.ycDown.find(keyAddr) != addrMap.ycDown.end())
			{
				return addrMap.ycDown[keyAddr];
			}
			break;
		case MSG_M_IT_NA_1:
			break;
		case MSG_C_SC_NA_1:
		case MSG_C_DC_NA_1:
			sprintf(key, "%s_%d",  "yk", infoAddr);
			keyAddr = string(key);
			if(addrMap.ykDown.find(keyAddr) != addrMap.ykDown.end())
			{
				return addrMap.ykDown[keyAddr];
			}
			break;
		case MSG_P_ME_NB_2:
			sprintf(key, "%s_%d",  "yt", infoAddr);
			keyAddr = string(key);
			if(addrMap.ytDown.find(keyAddr) != addrMap.ytDown.end())
			{
				return addrMap.ytDown[keyAddr];
			}
			break;
		case MSG_C_IC_NA_1:
		case MSG_C_CS_NA_1:
			sprintf(key, "%s_%d", "boardcmd", infoAddr);
			keyAddr = string(key);
			if(addrMap.boardCmdDown.find(keyAddr) != addrMap.boardCmdDown.end())
			{
				return addrMap.boardCmdDown[keyAddr];
			}
			break;
		case MSG_C_RD_NA_1:
		case MSG_C_RD_NB_1:
		case MSG_C_UP_NA_1:
		case MSG_C_RD_NC_1:
		case MSG_C_ST_NA_1:
		case MSG_C_MW_NA_1:
		case MSG_C_SV_NA_1:
		case MSG_C_MD_NA_1:
		case MSG_M_IC_NA_1:
		case MSG_M_UP_NA_1:
		case MSG_M_EC_NA_1:
			sprintf(key, "%s_%d",  "localcmd", infoAddr);
			keyAddr = string(key);
			if(addrMap.localCmdDown.find(keyAddr) != addrMap.localCmdDown.end())
			{
				return addrMap.localCmdDown[keyAddr];
			}
			break;
		default:
			sprintf(key, "%s_%d",  "cmd", infoAddr);
			keyAddr = string(key);
			if(addrMap.cmdDown.find(keyAddr) != addrMap.cmdDown.end())
			{
				return addrMap.cmdDown[keyAddr];
			}
			break;
	}

	Unit unit;
	return unit;
}
