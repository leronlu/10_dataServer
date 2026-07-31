#ifndef _MINMEAPROTOCOL_H_
#define _MINMEAPROTOCOL_H_

#include <string>
#include <mutex>
#include <vector>
#include <cstring>
#include "serial_module.h"
#include "minmea.h"

// 定位信息结构体
struct PositionInfo {
    float latitude;   // 纬度
    float longitude;  // 经度
    bool hasFix;     // 是否有有效定位
    
    // 构造函数
    PositionInfo() : latitude(0.0f), longitude(0.0f), hasFix(false) {}
    PositionInfo(float lat, float lon, bool fix) : latitude(lat), longitude(lon), hasFix(fix) {}
};

class MinmeaProtocol : public SerialProtocol {
public:
    MinmeaProtocol();
    ~MinmeaProtocol();
    
    void process() override;
    bool recvFrame(const SerialNetBuf &frame) override;
    bool sendFrame(SerialNetBuf &frame) override { return false; }
    
    // 获取定位信息接口
    PositionInfo getPositionInfo() const;
    
private:
    void applySystemTime(minmea_sentence_zda &zda);
    
private:
    PositionInfo positionInfo;
};

#endif // _MINMEAPROTOCOL_H_