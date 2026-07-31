#ifndef _QTPARAM_H_
#define _QTPARAM_H_

#include <algorithm>
#include <iostream>
#include <vector>
#include <string>


#define QVIDEO_BUFF_LEN  320000
#define QT_FFT_LEN		256
#define USERDIR  "/mnt/App/userFile/"
#define QLABEL_HEIGHT	60

//事件状态
enum WebSorcketType
{
    WebSorcketVideo,
    WebSorcketInfread,
    WebSorcketSpectrum,
    WebSorcketInfreadYuv420,
    WebSorketWave,
    webSorketPrpd,
    WebSorcketInfreadYUV2,
    WebSorcketBeam,
    WebSorcketCryBeam,
    WebSorcketCryBeam1,
    WebSorcketCryPrpd,
    WebSorcketCrySpectrum,
    WebSorcketTypeEnd
};

enum VideoFusionYcIndex
{
	YcIndexMaxPressure,															//彩虹点最大声压值
	YcIndexMinPressure,															//彩虹点最小声压值
	YcIndexSDCapacity,															//SD容量
	YCIndexCpuTemp,																//CPU温度
	YCIndexDistant,																//距离
	YcIndexMaxPressureX,														//最强点X坐标
	YcIndexMaxPressureY,														//最强点Y坐标
	YcIndexMinPressureX,														//最弱点X坐标
	YcIndexMinPressureY,														//最弱点Y坐标
	YcIndexPrpdType,															//放电类型
	YcIndexConfidence,															//放电置信度
	YcIndexLRQ,																	//LRQ
	YcIndexLeak,																//泄漏量
	YcIndexGasEconomicLoss,														//气经济损失
	YcIndexEnergyEconomicLoss													//能量经济损失
};


struct QVideoBuf
{
    QVideoBuf()
	{
		BufLen = 0;
		type = 0;
		memset(Buf, 0, sizeof(Buf));
	}

    uint8_t   Buf[QVIDEO_BUFF_LEN];
    int  BufLen;
	int type;
};

struct QVideoBuf1
{
    QVideoBuf1()
	{
		type = 0;
	}

    std::vector<uint8_t> data;
	int type;
};


struct QPointPos
{
	QPointPos(){
		x = 0;
		y = 0;
		posX = 0;
		posY = 0;
		radius = 20;
		count = 1.0;
	}
	uint16_t x;
	uint16_t y;
    int posX;
    int posY;
    int radius;
    float count;
};

struct QPointCap
{
	QPointCap()
	{
		//num = 0;
		memset(num, 0, sizeof(num));
	}
	QPointPos offset[3];
	QPointPos pt[3][128];
	int num[3];
};


struct Component
{
	Component(){
		id = 0;
	}
	int id;
	std::string description;
	std::string comp;
};


struct snapFileMsg
{
	snapFileMsg()
	{

	}
	std::string type;
	std::string path;
};

enum QTSevDataTypeIndex
{
	QTSevDataTypeSingleYx = 0,
	QTSevDataTypeDoubleYx,
	QTSevDataTypeYc,
	QTSevDataTypeFloatYc,
	QTSevDataTypeYm,
	QTSevDataTypeYt,
	QTSevDataTypeDoubleCoil,
	QTSevDataTypeSysmsg
};

enum DoubleCoilIndex{
    YkChargeEn,
    YkOpenLed,
    YkPowerOn,
    YkResume,
    YkShutDown,
	YkIndexEnd
};

#if 0
enum QTViewParamIndex
{
	//ParamQtViewModule = 0,															//检测模式
	ParamQtViewRainbowMaxColor = 1,													//彩虹点最大值颜色
	ParamQtViewRainbowMinColor,													//彩虹点最小值颜色
	ParamQtViewUpperFreq,														//频率上限值
	ParamQtViewLowerFreq,														//频率下限值
	ParamQtViewFocusMode,														//聚焦模式
	ParamQtViewUpperDynamic,														//动态范围上限值
	ParamQtViewLowerDynamic,														//动态范围下限值
	ParamQtViewBrightness,														//画面亮度
	ParamQtViewRainbowDotHue,													//彩虹点色调
	ParamQtViewIdleShutdownTime,													//无操作关机时间
	ParamQtViewIdleSleepTime,														//无操作休眠时间
	ParamQtViewPrpdEn,															//prpd开启/关闭
	ParamQtViewInfraredEn,														//红外开启/关闭
	ParamQtViewAutoShutDownEn,													//自动关机使能
	ParamQtViewLanguageIndex,													//语言中文/英文
	ParamQtSoundNum,															//声源点数目
	ParamQtViewModule,															//检测模式
	ParamQtBlendMode,															//融合模式
	ParamQtBusinessMode,														//业务模式
	ParamQtLeakageDist,															//泄漏点距离
	ParamQtLeakageLp,															//泄漏管道压力值
	ParamQtLeakageCn,															//泄漏量Cn
	ParamQtBrightness,															//亮度
	ParamQtContrast,															//对比度
	ParamQtHue,																	//色调
	ParamQtSaturation,															//饱和度
	ParamQtSharpness,															//锐度
	ParamQtGamma,																//伽马
	ParamQtWhiteBalance,														//白平衡
	ParamQtBacklightCompensation,												//逆光对比度
	ParamQtGain,																//增益
	ParamQtFocusAbsolute,														//焦距
	ParamQtExposure,															//曝光度
	ParamQtAutoWhiteBalance,													//自动白平衡
	ParamQtAutoFocus,															//自动对焦
	ParamQtAutoExposure,														//自动曝光
	ParamQtPowerLineFrequency,													//视频信号频率
	ParamQtExposureAbsolute,													//曝光时间
	ParamQtViewSpectrumEn,														//Spectrum开启/关闭
	ParamQtViewEnd
};
#else
enum VideoFusionParamIndex
{
	ParamVideoRainbowMaxColor = 1,												//1 彩虹点最大值颜色
	ParamVideoRainbowMinColor,													//彩虹点最小值颜色
	ParamVideoUpperFreq,														//频率上限值
	ParamVideoLowerFreq,														//频率下限值
	ParamVideoFocusMode,														//聚焦模式
	ParamVideoUpperDynamic,														//动态范围上限值
	ParamVideoRecord,															//录像
	ParamVideoBrightness,														//画面亮度
	ParamVideoRainbowDotHue,													//彩虹点色调
	ParamVideoIdleShutdownTime,													//10 无操作关机时间
	ParamVideoIdleSleepTime,													//无操作休眠时间
	ParamViewPrpdEn,															//prpd开启/关闭
	ParamViewInfraredEn,														//红外开启/关闭
	ParamViewAutoShutDownEn,													//自动关机使能
	ParamViewLanguageIndex,														//默认语言中文/英文
	ParamViewSoundNum,															//声源点数目
	ParamVideoModule,															//检测模式 瞬态/稳态
	ParamBlendMode,																//融合模式
	ParamBusinessMode,															//业务模式
	ParamLeakageDist,															//20 泄漏点距离
	ParamLeakageLp,																//泄漏管道压力值
	ParamLeakageCn,																//泄漏管道Cn
	ParamBrightness,															//亮度
	ParamContrast,																//对比度
	ParamSaturation,															//饱和度
	ParamHue,																	//色调
	ParamBacklightCompensation,													//背光补偿
	ParamGain,																	//增益
	ParamSharpness,																//锐度
	ParamPowerLineFrequency,													//30 视频信号频率
	ParamGamma,																	//伽马
	ParamWhiteBalance,															//白平衡
	ParamExposure,																//曝光度
	ParamExposureAbsolute,														//曝光绝对值
	ParamAutoWhiteBalance,														//自动白平衡
	ParamAutoExposure,															//自动曝光
	ParamAutoFocus,																//自动对焦
	ParamFocusAbsolute,															//焦距
	ParamViewSpectrumEn,														//Spectrum开启/关闭
	ParamImrMinPreSdLever,														//40 最小声压级	(db)
	ParamDistanceMeasure,														//开启自动测距
	ParamPickupMode,															//拾音模式。0 不开启拾音， 1 中心拾音模式，拾取屏幕中心位置， 2 自动模式，拾取最大声压级位置， 3 手动模式，拾取手动设置的位置
	ParamGasPressure,															//气体压力值
	ParamLeakMode,																//泄漏孔径类型
	ParamGasType,																//气体类型
	ParamGasCost,																//气体价值（￥/L）
	ParamPowerToFlowRateRatio,													//功率流量率比(kW/(m^3/min))
    ParamGasWorkTime,                                                           //年工作时长
    ParamGasDistance,                                                           //距离
    ParamGasFactor,                                                             //50 泄露量修正值
    ParamGasAbnormal,                                                           //异常泄露 边界值
    ParamGasMinorFault,                                                         //一般故障泄露 边界值
    ParamGasCriticalFault,                                                      //严重故障泄露 边界值
	ParamViewWaveEn,															//波形显示开关
	ParamViewPrpsEn,															//PRPS开启/关闭
    ParamThemeType,																//主题类型
	ParamApEn,																	//开启/关闭 热点
	ParamVideoEnd
};
#endif //0

struct QtParam
{
	QtParam()
	{
		upperFreq = 0;
		lowerFreq = 0;
		focusMode = 0;
		upperDynamic = 0;
		lowerDynamic = 0;
		brightness = 0;
		rainbowDotHue = 0;
		idleShutdownTime = 0;
		idleSleepTime = 0;
		prpdEn = false;
		infraredEn = false;
		soundNum = 0;
		viewModule = 0;
		blendMode = 0;
		businessMode = 0;
		leakageDist = 0;
		leakagePressure = 0.0;
		leakageCn = -40.0;
		waveEn = false;
	}
	int upperFreq;
	int lowerFreq;
	int focusMode;
	int upperDynamic;
	int lowerDynamic;
	int brightness;
	int rainbowDotHue;
	int idleShutdownTime;
	int idleSleepTime;
	bool prpdEn;
	bool infraredEn;
	int soundNum;
	int viewModule;
	int blendMode;
	int businessMode;
	int leakageDist;
	float leakagePressure;
	float leakageCn;
	bool waveEn;
};


#endif //_QTPARAM_H_
