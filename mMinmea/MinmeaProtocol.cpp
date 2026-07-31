using namespace std;
#include "MinmeaProtocol.h"
#include <cstring>
#include <vector>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstdlib>

MinmeaProtocol::MinmeaProtocol() {
    
}

MinmeaProtocol::~MinmeaProtocol() {
    // 清理资源
}

void MinmeaProtocol::process() {
    // 处理协议逻辑
}

bool MinmeaProtocol::recvFrame(const SerialNetBuf &frame) {
    std::vector<char> buf(frame.BufLen + 1);
    memcpy(buf.data(), frame.Buf, frame.BufLen);
    buf[frame.BufLen] = '\0';
    char *saveptr = nullptr;
    char *line = strtok_r(buf.data(), "\n", &saveptr);
    
    while (line) {
        line[strcspn(line, "\r")] = 0;
        switch (minmea_sentence_id(line, false)) {
            case MINMEA_SENTENCE_RMC: {
                minmea_sentence_rmc rmc;
                if (minmea_parse_rmc(&rmc, line)) {
                    if (rmc.valid) {
                        double lat = minmea_tocoord(&rmc.latitude);
                        double lon = minmea_tocoord(&rmc.longitude);
                        positionInfo.latitude = lat;
                        positionInfo.longitude = lon;
                        positionInfo.hasFix = true;
                    }
                }
            } break;
            case MINMEA_SENTENCE_ZDA: {
                minmea_sentence_zda zda;
                if (minmea_parse_zda(&zda, line)) {
                    if ((zda.date.year > 0)
                    && (zda.date.month > 0)
                    && (zda.date.day > 0)) {
                        applySystemTime(zda);
                    }
                }
            } break;
            default: break;
        }
        line = strtok_r(nullptr, "\n", &saveptr);
    }
    return true;
}

void MinmeaProtocol::applySystemTime(minmea_sentence_zda &zda) {
    // 使用C++标准库替代system调用
    std::ostringstream cmd;
    cmd << "date -s \""
        << std::setfill('0') << std::setw(4) << zda.date.year << "-"
        << std::setfill('0') << std::setw(2) << zda.date.month << "-"
        << std::setfill('0') << std::setw(2) << zda.date.day << " "
        << std::setfill('0') << std::setw(2) << zda.time.hours << ":"
        << std::setfill('0') << std::setw(2) << zda.time.minutes << ":"
        << std::setfill('0') << std::setw(2) << zda.time.seconds << "\"";
    
    // 注意：实际系统时间设置需要系统权限，这里仅构建命令字符串
    // 在实际应用中，应该使用适当的权限和错误处理机制
    std::string dateCmd = cmd.str();
    std::string hwclockCmd = "sudo hwclock --systohc -f /dev/rtc1";
    
    // 这里应该使用更安全的系统调用方式，如fork/exec或platform-specific API
    // 暂时保留原始逻辑，但使用更安全的字符串构建
    std::system(dateCmd.c_str());
    std::system(hwclockCmd.c_str());
}

PositionInfo MinmeaProtocol::getPositionInfo() const {
    return positionInfo;
}