#include <unistd.h>
using namespace std;
#include <dirent.h>
#include "zhaoHuaFile.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include <iostream>

// ---------------- Helper Functions ----------------
static std::string getCurrentDate() {
    std::time_t now = std::time(nullptr);
    std::tm tm_info = *std::localtime(&now);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d", &tm_info);
    return std::string(buffer);
}

static std::string getCurrentDateTime() {
    std::time_t now = std::time(nullptr);
    std::tm tm_info = *std::localtime(&now);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &tm_info);
    return std::string(buffer);
}

static bool dirExists(const std::string &path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

static bool createDir(const std::string &path) {
    if (dirExists(path)) return true;
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0;
#else
    return mkdir(path.c_str(), 0755) == 0;
#endif
}

std::string getLatestCaptureDir(SDMsg* sdMsg) {
    std::string basePath = sdMsg->getSDCardPath() + "/capture";
    createDir(basePath);

    std::string currentDate = getCurrentDate();
    int maxIndex = 0;

    DIR* dir = opendir(basePath.c_str());
    if(dir) {
        struct dirent* entry;
        while((entry = readdir(dir)) != nullptr) {
            std::string name(entry->d_name);
            if(name.find(currentDate + "_") == 0) {
                auto pos = name.find('_');
                if(pos != std::string::npos) {
                    int idx = std::stoi(name.substr(pos + 1));
                    if(idx > maxIndex) maxIndex = idx;
                }
            }
        }
        closedir(dir);
    }

    std::ostringstream oss;
    oss << std::setw(3) << std::setfill('0') << maxIndex;
    std::string newDir = basePath + "/" + currentDate + "_" + oss.str();
    createDir(newDir);
    return newDir;
}

// ---------------- ZhaoHuaFile Implementation ----------------
ZhaoHuaFile::ZhaoHuaFile() : recordState(0) {
	sdMsg = new SDMsg();
}

ZhaoHuaFile::ZhaoHuaFile(string deviceModel, string serialNumber){
    sdMsg = new SDMsg();
	createMsginiFile(deviceModel, serialNumber);
}

ZhaoHuaFile::~ZhaoHuaFile() {
    outFile.close();
    delete sdMsg;
}

void ZhaoHuaFile::setRecordState(int state) {
    recordState = state;
}

std::string ZhaoHuaFile::getNewCaptureDir() {
    return getLatestCaptureDir(sdMsg);
}

bool ZhaoHuaFile::createInfoDatFile(InfoDataHead &dataHead) {
    std::string dirPath = getNewCaptureDir();
    std::string filename = dirPath + "/info.dat";

    outFile.open(filename, std::ios::out | std::ios::app);
    if(!outFile.is_open()) return false;

    std::string now = getCurrentDateTime();
    outFile << now << ".000 headLine:12\n";
    outFile << "Version:" << dataHead.version << "\n";
    outFile << "ProductId:" << dataHead.productId << "\n";
    outFile << "DeviceName:" << dataHead.deviceName << "\n";
    outFile << "rx1:\nrx2:\nrx3:\nry1:\nry2:\nry3:\n";
    outFile << "PDDLength:240\n";
    outFile << "seqNumber\tfreqBand_L\tfreqBand_H\tdynamic\tmaxdBSpl\tposition_X\tposition_Y\tUltrasonic\thasDotHue\tdischargeType\n";
    return true;
}

bool ZhaoHuaFile::appendInfoDatFile(InfoDatMsg &msg, bool compFlag) {
    infoDatMsgList.push_back(msg);
    if(!compFlag && infoDatMsgList.size() < 100) return false;
    if(!outFile.is_open()) return false;

    static int seqNumber = 0;
    for(auto &m : infoDatMsgList) {
        outFile << ++seqNumber << "\t"
                << m.freqBand_L << "\t"
                << m.freqBand_H << "\t"
                << m.dynamic << ".0\t"
                << m.maxdBSpl << ".0\t"
                << m.position_X << "\t"
                << m.position_Y << "\t"
                << m.maxdBSpl << ".0\t"
                << m.hasDotHue << "\t"
                << m.dischargeType << "\n";
    }
    infoDatMsgList.clear();
    if(compFlag) {
        seqNumber = 0;
        outFile.close();
    }
    return true;
}

bool ZhaoHuaFile::createTagFile(TagFileMsg &msg) {
    std::string dirPath = getNewCaptureDir() + "/tag";
    createDir(dirPath);
    std::string filename = dirPath + "/02.txt";

    tagFile.open(filename, std::ios::out | std::ios::app);
    if(!tagFile.is_open()) return false;
    tagFile << msg.remark << "\n";
    tagFile.close();
    return true;
}

bool ZhaoHuaFile::createItemFile(ItemFileMsg &msg) {
    std::string dirPath = getNewCaptureDir();
    std::string filename = dirPath + "/itemInfo.ini";

    itemFile.open(filename, std::ios::out | std::ios::app);
    if(!itemFile.is_open()) return false;

    itemFile << "[ElectricLeak]\n";
    itemFile << "distance=0\n";
    itemFile << "elecUnit=kV\n";
    itemFile << "faultSite=\n";
    itemFile << "voltage=0\n\n";

    itemFile << "[GasLeak]\n";
    itemFile << "airPressure=\n";
    itemFile << "airType=\n";
    itemFile << "airUnit=\n";
    itemFile << "faultSite=\n\n";

    itemFile << "[itemInfo]\n";
    itemFile << "faultSite=1\n";

    itemFile.close();
    return true;
}

void ZhaoHuaFile::videoRecord(TagFileMsg &tagFileMsg, ItemFileMsg &itemFileMsg,
                               InfoDataHead &infoDataHead, InfoDatMsg &infoDatMsg) {
    if(recordState == 1) {
        infoDatMsgList.clear();
        createInfoDatFile(infoDataHead);
    } else if(recordState == 2) {
        appendInfoDatFile(infoDatMsg, false);
    } else if(recordState == 3) {
        appendInfoDatFile(infoDatMsg, true);
        createTagFile(tagFileMsg);
        createItemFile(itemFileMsg);
        recordState = 0;
    } else {
        recordState = 0;
    }
}

bool ZhaoHuaFile::createMsginiFile(string deviceModel, string serialNumber) {
	struct stat buffer;
    const std::string filename = sdMsg->getSDCardPath()+"/capture"+"/msg.ini";

    if (stat(filename.c_str(), &buffer) == 0) {
		//printfs(LOG_INFO, "文件: %s已存在", filename.c_str());
    } else {
        std::ofstream file(filename);
        if (!file.is_open())  {
			//printfs(LOG_ERROR, "无法创建文件");
            return false;
        }

		auto now = std::time(nullptr);
	    auto tm_info = *std::localtime(&now);

	    std::ostringstream oss;
	    oss << std::put_time(&tm_info, "%Y-%m-%d %H:%M:%S");

        file << "[DeviceInfo]\n";
        file << "model=" + deviceModel + "\n";
        file << "sn=" + serialNumber + "\n\n";
        file << "[TestInfo]\n";
        file << "datetime="+ oss.str() + "\n";

        file.close();
		//printfs(LOG_INFO, "文件: %s创建并写入成功", filename);
    }
    return true;
}

// ---------------- ZhaoHuaReportFile Implementation ----------------

ZhaoHuaReportFile::ZhaoHuaReportFile() {
    sdMsg = new SDMsg();
}

ZhaoHuaReportFile::~ZhaoHuaReportFile() {
    delete sdMsg;
}

std::string ZhaoHuaReportFile::getNewCaptureDir() {
    return getLatestCaptureDir(sdMsg);
}

string GastTypeArr[]={
	"空气", 
	"乙炔", 
	"氨气", 
	"氩气", 
	"二氧化碳", 
	"一氧化碳", 
	"氯气", 
	"乙烷",
	"乙烯", 
	"氦气", 
	"氢气", 
	"硫化氢",  
	"甲烷", 
	"氦气", 
	"一氧化氮", 
	"氮气", 
	"一氧化二氮", 
	"氧气", 
	"丙烷", 
	"二氧化硫", 
	"水蒸气",
	"六氟化硫", 
	"制冷剂R134*" 
};

string LeakMode[] = {
	"默认",
	"快连接",
	"软管",
	"螺纹",
	"开口/端口",
	"微小孔",
	"真空内漏",
	"第二代"  
};

string LeakErrorIndex[] = {
	"重大缺陷",
	"一般缺陷",
	"轻微缺陷",
	"无缺陷"
};

string LeakErrorProcess[] = {
	"缺陷比较重大，但设备仍可在短期内继续安全运行 但需要尽快处理.",
	"对短期的安全运行影响不大，但需要列入年、季度检修。",
	"对短期的安全运行影响不大，可以继续观察，等待下次停机时维修。",
	""
};

string PrpdTypeIndex[] = {
        "",
        "电晕放电",
        "沿面放电",
        "悬浮放电"
    };

bool ZhaoHuaReportFile::createDataJsonFile(std::vector<LeakResult>& results){
    for (size_t i = 0; i < results.size(); ++i) {
        auto& leakResult = results[i];

        if(leakResult.leakValue >= 30.0) {
            leakResult.severity = LeakErrorIndex[0];
            leakResult.analysis = "检测到的声源可能是气体泄漏, 严重程度为严重缺陷, 漏量可能是"
                + leakResult.leak_rate +", 造成每年损失可能是" + leakResult.loss + "CNY";
            leakResult.processing  = LeakErrorProcess[0];
        } else if((leakResult.leakValue >= 15.0) && (leakResult.leakValue < 30.0)){
            leakResult.severity = LeakErrorIndex[1];
            leakResult.analysis = "检测到的声源可能是气体泄漏, 严重程度为一般缺陷, 漏量可能是"
                + leakResult.leak_rate +", 造成每年损失可能是" + leakResult.loss + "CNY";
            leakResult.processing  = LeakErrorProcess[1];
        } else if((leakResult.leakValue >= 5.0) && (leakResult.leakValue < 15.0)){
            leakResult.severity = LeakErrorIndex[2];
            leakResult.analysis = "检测到的声源可能是气体泄漏, 严重程度为轻微缺陷, 漏量可能是"
                + leakResult.leak_rate +", 造成每年损失可能是" + leakResult.loss + "CNY";
            leakResult.processing  = LeakErrorProcess[2];
        } else {
            leakResult.severity = LeakErrorIndex[3];
            leakResult.analysis = "未检测到气体泄漏, 设备无缺陷, 无需处理";
            leakResult.processing  = LeakErrorProcess[3];
        }
        if(leakResult.gasValue < (int)(sizeof(GastTypeArr)/sizeof(GastTypeArr[0]))) {
            leakResult.gas_type = GastTypeArr[leakResult.gasValue];
        }
        std::time_t now = std::time(nullptr);
        std::tm tm_info = *std::localtime(&now);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);
        leakResult.test_time = std::string(buffer);

        string dirPath = getLatestCaptureDir(sdMsg);
        string imagePath = dirPath + "/tag/tag.jpg";
        leakResult.image.imagePath = imagePath;
        leakResult.image.width_mm = 150;
        if(leakResult.prpdType < (int)(sizeof(PrpdTypeIndex) / sizeof(PrpdTypeIndex[0]))){
            leakResult.dischargeType = PrpdTypeIndex[leakResult.prpdType];
        }
    }
    return createDataJsonFileProc(results);
}

bool ZhaoHuaReportFile::createDataJsonFileProc(std::vector<LeakResult>& results) {
    std::string dirPath = getNewCaptureDir();
    std::string filename = dirPath + "/data.json";

    std::ofstream jsonFile(filename, std::ios::out | std::ios::trunc);
    if (!jsonFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    jsonFile << "{\n";
    jsonFile << "    \"results\": [\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& item = results[i];
        jsonFile << "        {\n";
        jsonFile << "            \"id\": \"" << item.id << "\",\n";
        jsonFile << "            \"station\": \"" << item.station << "\",\n";
        jsonFile << "            \"device\": \"" << item.device << "\",\n";
        jsonFile << "            \"freqBand_L\": \"" << item.freqBand_L << "\",\n";
        jsonFile << "            \"freqBand_H\": \"" << item.freqBand_H << "\",\n";
        jsonFile << "            \"dynamic\": \"" << item.dynamic << "\",\n";
        jsonFile << "            \"position_X\": \"" << item.position_X << "\",\n";
        jsonFile << "            \"position_Y\": \"" << item.position_Y << "\",\n";
        jsonFile << "            \"hasDotHue\": \"" << item.hasDotHue << "\",\n";
        jsonFile << "            \"dischargeType\": \"" << item.dischargeType << "\",\n";
        jsonFile << "            \"confidence\": \"" << item.confidence << "\",\n";

        jsonFile << "            \"type\": \"" << item.type << "\",\n";
        jsonFile << "            \"asset_name\": \"" << item.asset_name << "\",\n";
        jsonFile << "            \"test_time\": \"" << item.test_time << "\",\n";
        jsonFile << "            \"asset_id\": \"" << item.asset_id << "\",\n";
        jsonFile << "            \"gas_type\": \"" << item.gas_type << "\",\n";
        jsonFile << "            \"gas_pressure\": \"" << item.gas_pressure << "\",\n";
        jsonFile << "            \"sound_pressure\": \"" << item.sound_pressure << "\",\n";
        jsonFile << "            \"leak_rate\": \"" << item.leak_rate << "\",\n";
        jsonFile << "            \"power_usage\": \"" << item.power_usage << "\",\n";
        jsonFile << "            \"loss\": \"" << item.loss << "\",\n";
        jsonFile << "            \"carbon_emission\": \"" << item.carbon_emission << "\",\n";
        jsonFile << "            \"distance\": \"" << item.distance << "\",\n";
        jsonFile << "            \"severity\": \"" << item.severity << "\",\n";
        jsonFile << "            \"severity_color\": \"" << item.severity_color << "\",\n";
        jsonFile << "            \"weather\": \"" << item.weather << "\",\n";
        jsonFile << "            \"image\": { \"$image\": \"" << item.image.imagePath << "\", \"width_mm\": " << item.image.width_mm << " },\n";
        
        jsonFile << "            \"analysis\": \"" << item.analysis << "\",\n";
        jsonFile << "            \"processing\": \"" << item.processing << "\",\n";
        
        jsonFile << "            \"repair_suggestion\": {\n";
        jsonFile << "                \"needed\": \"" << item.repair_suggestion.needed << "\",\n";
        jsonFile << "                \"priority\": \"" << item.repair_suggestion.priority << "\",\n";
        jsonFile << "                \"note\": \"" << item.repair_suggestion.note << "\"\n";
        jsonFile << "            }\n";

        jsonFile << "        }";
        if (i < results.size() - 1) {
            jsonFile << ",";
        }
        jsonFile << "\n";
    }

    jsonFile << "    ]\n";
    jsonFile << "}\n";

    jsonFile.close();
    return true;
}

bool ZhaoHuaReportFile::createConfigJsonFile(const std::vector<TemplateConfig>& templates) {
    std::string basePath = sdMsg->getSDStoragePath() + "/report";
    createDir(basePath);
    std::string filename = basePath + "/config.json";

    std::ofstream jsonFile(filename, std::ios::out | std::ios::trunc);
    if (!jsonFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    jsonFile << "{\n";
    jsonFile << "  \"templates\": [\n";

    for (size_t i = 0; i < templates.size(); ++i) {
        const auto& tmpl = templates[i];
        jsonFile << "    {\n";
        jsonFile << "      \"name\": \"" << tmpl.name << "\",\n";
        jsonFile << "      \"file\": \"" << tmpl.file << "\",\n";
        jsonFile << "      \"data\": [\n";
        for (size_t j = 0; j < tmpl.data.size(); ++j) {
            jsonFile << "        {\n";
            jsonFile << "          \"file\": \"" << tmpl.data[j].file << "\"\n";
            jsonFile << "        }";
            if (j < tmpl.data.size() - 1) {
                jsonFile << ",";
            }
            jsonFile << "\n";
        }
        jsonFile << "      ],\n";
        jsonFile << "      \"output_docx\": \"" << tmpl.output_docx << "\",\n";
        jsonFile << "      \"merge_output\": \"" << tmpl.merge_output << "\"\n";
        jsonFile << "    }";
        if (i < templates.size() - 1) {
            jsonFile << ",";
        }
        jsonFile << "\n";
    }

    jsonFile << "  ]\n";
    jsonFile << "}\n";

    jsonFile.close();
    return true;
}
