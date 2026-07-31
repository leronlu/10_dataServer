#include "VideoParam.h"
#include <iostream> 
#include <cstdio>
#include <cstring>
#ifdef Q_OS_LINUX
#include "xml_parser.h"
#endif //
// Initialize static instance
using namespace std;
VideoParam* VideoParam::instance = nullptr;

VideoParam::VideoParam() : width(1280), height(960), sampleRate(96000), 
    prpdWidth(425), prpdHeight(340),
    waveWidth(425), waveHeight(340),
    spectrumWidth(195), spectrumHeight(1140),
    cameraWidth(1920), cameraHeight(1200), modeName("online"){
    loadConfig();
}

VideoParam* VideoParam::getInstance() {
    if (instance == nullptr) {
        instance = new VideoParam();
    }
    return instance;
}

void VideoParam::loadConfig() {
#ifdef Q_OS_LINUX
    XmlNodeParser f_XmlNodeParser((int8_t *)VIDEO_CONFIG_FILE, (int8_t *)"/MPPConfig");
    int32_t value = 0;
    int8_t text[128] = "";
    int8_t         nodePath[128] = "";
    
    cout << "加载VideoParam配置...\n";

    f_XmlNodeParser.FindNode((int8_t *)"/MPPConfig");
	int nodeNum = f_XmlNodeParser.GetChildCounter("mode");
    for(uint8_t i=0; i<nodeNum; i++) {

        sprintf((char *)nodePath, "/MPPConfig/mode[%d]", i+1);
		f_XmlNodeParser.FindNode(nodePath);

        bool enable = false;
        if(f_XmlNodeParser.GetProperty((int8_t *)"enable", text)){
			enable = f_XmlNodeParser.StrToBoolean(text);
		}
        if(!enable) {
            continue;
        }

		if(f_XmlNodeParser.GetProperty((int8_t *)"name", text)){
			modeName = (char *)text;
		}
        cout << "声学成像仪工作模式: " << modeName << endl;

        sprintf((char *)nodePath, "/MPPConfig/mode[%d]/video", i+1);
        if (!f_XmlNodeParser.FindNode(nodePath)) {
            cout << "未找到video配置节点，使用默认值 width=" << width << ", height=" << height << endl;
        } else {
            cout << "找到video配置节点" << endl;

            if (f_XmlNodeParser.GetChildContent((int8_t *)"width", value)) {
                width = value;
                cout << "video width: " << width << endl;   
            } else{
                width = 1280;
            }
            if (f_XmlNodeParser.GetChildContent((int8_t *)"height", value)) {
                height = value;
                cout << "video height: " << height << endl;   
            } else{
                height = 960;
            }
            if (f_XmlNodeParser.GetChildContent((int8_t *)"sampleRate", value)) {
                sampleRate = value;
                cout << "sampleRate: " << sampleRate << endl;   
            } else{
                sampleRate = 96000;
            }
            cout << "视频参数加载成功: width=" << width << ", height=" << height << endl;
        }
        sprintf((char *)nodePath, "/MPPConfig/mode[%d]/prpd", i+1);
        if (!f_XmlNodeParser.FindNode(nodePath)) {
            cout << "未找到prpd配置节点，使用默认值 width=" << prpdWidth << ", height=" << prpdHeight << endl;
        } else {
            cout << "找到prpd配置节点" << endl;
            if (f_XmlNodeParser.GetChildContent((int8_t *)"width", value)) {
                prpdWidth = value;
                cout << "prpd width: " << prpdWidth << endl;   
            } else{
                prpdWidth = 660;
            }
            if (f_XmlNodeParser.GetChildContent((int8_t *)"height", value)) {
                prpdHeight = value;
                cout << "prpd height: " << prpdHeight << endl;   
            } else{
                prpdHeight = 160;
            }
        }
        sprintf((char *)nodePath, "/MPPConfig/mode[%d]/wave", i+1);
        if (!f_XmlNodeParser.FindNode(nodePath)) {
            cout << "未找到wave配置节点，使用默认值 width=" << waveWidth << ", height=" << waveHeight << endl;
        } else {
            cout << "找到wave配置节点" << endl;
            if (f_XmlNodeParser.GetChildContent((int8_t *)"width", value)) {
                waveWidth = value;
                cout << "wave width: " << waveWidth << endl;
            } else{
                waveWidth = 660;
            }
            if (f_XmlNodeParser.GetChildContent((int8_t *)"height", value)) {
                waveHeight = value;
                cout << "wave height: " << waveHeight << endl;
            } else{
                waveHeight = 160;
            }
        }
        sprintf((char *)nodePath, "/MPPConfig/mode[%d]/spectrum", i+1);
        if (!f_XmlNodeParser.FindNode(nodePath)) {
            cout << "未找到spectrum配置节点，使用默认值 width=" << spectrumWidth << ", height=" << spectrumHeight << endl;
        } else {
            cout << "找到spectrum配置节点" << endl;
            if (f_XmlNodeParser.GetChildContent((int8_t *)"width", value)) {
                spectrumWidth = value;
                cout << "spectrum width: " << spectrumWidth << endl;   
            } else{
                spectrumWidth = 120;
            }
            if (f_XmlNodeParser.GetChildContent((int8_t *)"height", value)) {
                spectrumHeight = value;
                cout << "spectrum height: " << spectrumHeight << endl;   
            } else{
                spectrumHeight = 550;
            }
        }
        sprintf((char *)nodePath, "/MPPConfig/mode[%d]/camera", i+1);
        if (!f_XmlNodeParser.FindNode(nodePath)) {
            cout << "未找到camera配置节点，使用默认值 width=" << cameraWidth << ", height=" << cameraHeight << endl;
        } else {
            cout << "找到camera配置节点" << endl;
            if (f_XmlNodeParser.GetChildContent((int8_t *)"width", value)) {
                cameraWidth = value;
                cout << "camera width: " << cameraWidth << endl;   
            } else{
                cameraWidth = 1280;
            }
            if (f_XmlNodeParser.GetChildContent((int8_t *)"height", value)) {
                cameraHeight = value;
                cout << "camera height: " << cameraHeight << endl;   
            } else{
                cameraHeight = 960;
            }
        }
    }
#else
    width = 1920;
    height = 1200;
    sampleRate = 192000;
    prpdWidth = 425;
    prpdHeight = 340;
    waveWidth = 425;
    waveHeight = 340;
    spectrumWidth = 195;
    spectrumHeight = 1140;
    cameraWidth = 2048;
    cameraHeight = 1536;
#endif
}
