#ifndef _ZHAOHUAFILE_STD_H_
#define _ZHAOHUAFILE_STD_H_

#include <string>
#include <vector>
#include <fstream>

#include "sdMsg.h"

using namespace std;

struct InfoDatMsg {
    InfoDatMsg() : freqBand_L(0), freqBand_H(0), dynamic(0), maxdBSpl(0),
                   position_X(0), position_Y(0), hasDotHue(0), dischargeType(0) {}
    int freqBand_L;
    int freqBand_H;
    int dynamic;
    int maxdBSpl;
    int position_X;
    int position_Y;
    int hasDotHue;
    int dischargeType;
};

struct InfoDataHead {
    std::string version;
    std::string productId;
    std::string deviceName;
};

struct TagFileMsg {
    std::string remark;
};

struct ItemFileMsg {
    std::string remark;
};

struct RepairSuggestion {
    std::string needed;                                     //维修建议
    std::string priority;                                   //是否维修
    std::string note;                                       //维修优先级
};

struct ImageInfo {
    std::string imagePath;                                  //图片路径
    int width_mm;                                           //宽度
};

struct LeakResult {
    std::string id;
    std::string station;                                    //站所
    std::string device;                                     //设备
    std::string component;                                  //组件
    std::string type;                                       //测试类型
    std::string asset_name;                                 //资产名称
    std::string test_time;                                  //测试时间
    std::string asset_id;                                   //资产ID
    std::string gas_type;                                   //气体类型
    std::string gas_pressure;                               //气体压力
    std::string sound_pressure;                             //声压
    std::string leak_rate;                                  //漏量大小
    std::string power_usage;                                //用电量
    std::string loss;                                       //损失
    std::string carbon_emission;                            //碳排放
    std::string distance;                                   //测试距离
    std::string severity;                                   //严重程度
    std::string severity_color;                             //
    std::string weather;                                    //天气说明
    ImageInfo image;                                        //
    std::string analysis;                                   //故障分析
    std::string processing;                                 //故障处理
    RepairSuggestion repair_suggestion;                     //维修建议

    std::string freqBand_L;                                 //频率下限
    std::string freqBand_H;                                 //频率上限
    std::string dynamic;                                    //动态
    std::string position_X;                                 //彩虹点位置x坐标
    std::string position_Y;                                 //彩虹点位置y坐标
    std::string hasDotHue;                                  //是否具有彩虹点
    std::string dischargeType;                              //放电类型
    std::string confidence;                                 //置信度

    int prpdType;
    float leakValue;                                        //泄漏值
    int gasValue;
};

struct DataFile {
    std::string file;
};

struct TemplateConfig {
    std::string name;
    std::string file;
    std::vector<DataFile> data;
    std::string output_docx;
    std::string merge_output;
};

std::string getLatestCaptureDir(SDMsg* sdMsg);

class ZhaoHuaFileInter {
public:
    virtual void videoRecord(TagFileMsg &tagFileMsg, ItemFileMsg &itemFileMsg,
                             InfoDataHead &infoDataHead, InfoDatMsg &infoDatMsg) = 0;
    virtual void setRecordState(int state) = 0;
    virtual ~ZhaoHuaFileInter() = default;
};

class ZhaoHuaFile : public ZhaoHuaFileInter {
public:
    ZhaoHuaFile();
    ZhaoHuaFile(std::string deviceModel, std::string serialNumber);
    ~ZhaoHuaFile();

    void setRecordState(int state) override;
    void videoRecord(TagFileMsg &tagFileMsg, ItemFileMsg &itemFileMsg,
                     InfoDataHead &infoDataHead, InfoDatMsg &infoDatMsg) override;

private:
    int recordState;
    std::ofstream outFile;
    std::ofstream tagFile;
    std::ofstream itemFile;
    std::vector<InfoDatMsg> infoDatMsgList;
    SDMsg *sdMsg{nullptr};
private:
    std::string getNewCaptureDir();
    bool createInfoDatFile(InfoDataHead &dataHead);
    bool appendInfoDatFile(InfoDatMsg &msg, bool compFlag);
    bool createTagFile(TagFileMsg &msg);
    bool createItemFile(ItemFileMsg &msg);
	bool createMsginiFile(std::string deviceModel, std::string serialNumber);
};

class ZhaoHuaReportFileInter {
public:
    virtual bool createDataJsonFile(std::vector<LeakResult>& results) = 0;
    virtual bool createConfigJsonFile(const std::vector<TemplateConfig>& templates) = 0;
    virtual ~ZhaoHuaReportFileInter() = default;
};

class ZhaoHuaReportFile : public ZhaoHuaReportFileInter {
public:
    ZhaoHuaReportFile();
    ~ZhaoHuaReportFile();

    bool createDataJsonFile(std::vector<LeakResult>& results) override;
    bool createConfigJsonFile(const std::vector<TemplateConfig>& templates) override;

private:
    bool createDataJsonFileProc(std::vector<LeakResult>& results);
private:
    SDMsg *sdMsg{nullptr};
    std::string getNewCaptureDir();
};

#endif //_ZHAOHUAFILE_STD_H_
