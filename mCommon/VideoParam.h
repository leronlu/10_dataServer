#ifndef VIDEO_PARAM_H
#define VIDEO_PARAM_H

//// #include "typedef.h" removed
//#include "xml_parser.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>

#define VIDEO_CONFIG_FILE "../conf/MPPConfig.xml"


#define    ONLINEMODE               "online"
#define    PORTABLEMODE             "portable"

class VideoParam {
private:
    // Private constructor for singleton
    VideoParam();
    
    // Prevent copying
    VideoParam(const VideoParam&) = delete;
    VideoParam& operator=(const VideoParam&) = delete;
    
    // Video parameters
    uint32_t width;
    uint32_t height;
    uint32_t sampleRate;

    uint32_t prpdWidth;
    uint32_t prpdHeight;

    uint32_t waveWidth;
    uint32_t waveHeight;

    uint32_t spectrumWidth;
    uint32_t spectrumHeight;

    uint32_t cameraWidth;
    uint32_t cameraHeight;

    std::string modeName;
    
    // Load configuration from XML file
    void loadConfig();
    
    // Static instance
    static VideoParam* instance;
    
public:
    // Get singleton instance
    static VideoParam* getInstance();
    
    // Get video width
    uint32_t getWidth() const { return width; }
    
    // Get video height
    uint32_t getHeight() const { return height; }

    uint32_t getSampleRate() const { return sampleRate; }   
    
    uint32_t getPrpdWidth() const { return prpdWidth; }
    
    uint32_t getPrpdHeight() const { return prpdHeight; }

    uint32_t getWaveWidth() const { return waveWidth; }
    uint32_t getWaveHeight() const { return waveHeight; }
    
    uint32_t getSpectrumWidth() const { return spectrumWidth; }
    
    uint32_t getSpectrumHeight() const { return spectrumHeight; }
    
    uint32_t getCameraWidth() const { return cameraWidth; }
    
    uint32_t getCameraHeight() const { return cameraHeight; }
    
    std::string getModeName() const { return modeName; }
};

#endif // VIDEO_PARAM_H
