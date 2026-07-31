# 10_dataServer — 下行数据采集与分发服务

基于 C++14 的嵌入式数据服务器，运行于 Jetson 平台（支持 X86 交叉编译），负责 ModBus RTU 下行设备数据采集、IEC104 协议转换、ZeroMQ 消息分发、GPS NMEA 解析、文件管理和视频数据融合。

## 项目结构

```
conf/           XML 配置文件（串口、ModBus、ZMQ、日志、波束等 11 个）
lib/            硬件驱动层（GPIO、按键、LCD1602 显示）
mCommon/        公共模块（共享内存、系统消息、视频参数、文件操作）
mFileManage/    文件管理模块
mMinmea/        GPS NMEA 协议解析（基于 minmea C 库）
mModBus/        ModBus RTU 协议处理（核心模块，230K+）
mMsgProtocol/   消息协议处理
mVideoFusion/   视频融合（IPC 进程间通信、麦克风数据）
main.cpp        程序入口
DownSideDataModule.cpp/h  下行数据管理核心模块
RtuBaseClass.h  RTU 基类定义
build/CMakeLists.txt       CMake 构建配置（唯一纳入版本控制的 build 文件）
```

## 构建

### 系统依赖

- log4cpp, sqlite3, libzmq, libjpeg, libxml2, libiconv（必需的）
- gdal, xlsreader（可选的）
- 外部库: `10_lib`（位于 `/home/lw/WORK_Claude/10_lib`，提供日志、定时器、串口、ZMQ 组件、XML 解析等）

### 编译

```bash
cd build && cmake . && make -j$(nproc)
```

产物: `build/data_server`

### 10_lib 链接组件

```
log_level crcmake datemanage mutexmanage processmanage timermanage
CharSetConv Sqlite3DB STLPackage XmlParser logmanage zmq_component
iec_message uartmanage SerialModule
```

<!-- AUTO-GENERATED from build/CMakeLists.txt -->

### 当前环境状态

- 所有依赖库已安装在 `/usr/local/lib/`（非标准路径）
- CMakeLists.txt 中 `find_library` 会搜索系统默认路径和 `/usr/local/lib`
- 编译定义 `X86_BUILD` 用于 x86 桌面测试环境
- 编译选项: `-Wall -g -fpermissive`，C++14 标准，Debug 构建

## 配置文件

<!-- AUTO-GENERATED from conf/ -->

| 文件 | 用途 |
|------|------|
| `SysConfig.xml` | 设备序列号、型号、厂家、软件版本 |
| `LogConfig.xml` | 日志输出级别、存储级别、Socket 服务 |
| `ZmqCommConfig.xml` | ZMQ 节点拓扑与发布/订阅路由 |
| `ModBusMasterConfig.xml` | ModBus RTU 串口参数、设备模板、帧解析规则 |
| `ACConfig.xml` | AC 电源配置 |
| `BeamConfig.xml` | 波束配置 |
| `DownSideDataConfig.xml` | 下行数据采集配置 |
| `UpSideDataConfig.xml` | 上行数据上报配置 |
| `InnerCommConfig.xml` | 内部通信配置 |
| `HCConfig.xml` | HC 红外配置 |
| `FileManageConfig.xml` | 文件管理（波形文件等）配置 |

## ZMQ 通信拓扑

各模块通过 ZeroMQ P2P(发布/订阅) 模式通信，本节点为 `DOWNSIDE`（`tcp://127.0.0.1:5561`）：

```
DOWNSIDE ──→ pub ──→ QTServer, WebServer
WebServer ──→ pub ──→ DOWNSIDE, Audio, Infrared, V4L2
QTServer  ──→ pub ──→ DOWNSIDE, Audio, Infrared, V4L2, G500Bridge
Audio     ──→ pub ──→ WebServer, QTServer
Infrared  ──→ pub ──→ WebServer, QTServer
V4L2      ──→ pub ──→ WebServer, QTServer
G500Bridge──→ pub ──→ QTServer
```

<!-- AUTO-GENERATED from conf/ZmqCommConfig.xml -->

## ModBus RTU 协议

串口默认参数: 9600-8-N-1，轮询周期 1000ms。

支持的数据帧类型:
- `singleYx` / `doubleYx` — 遥信（单点/双点）
- `yc` / `int32Yc` / `floatYc` — 遥测（短整/32位整/浮点）
- `ym` — 遥脉（电量累计）
- `param` — 参数（定值）
- `yk` / `multiYk` / `ykSelect` — 遥控（单点/多点/选择）
- `write` — 写寄存器
- `mix` — 混合帧（多子帧组合）

支持两种解析模式: 光芒模式(=1) / 南凯模式(=0)

<!-- AUTO-GENERATED from conf/ModBusMasterConfig.xml -->

## 运行

**硬约束：X86 环境必须从 `build/` 目录执行程序**，因为 `#ifdef X86_BUILD` 下配置文件路径为 `../conf/`，该相对路径依赖于 `build/` 作为当前工作目录。

```bash
cd build && ./data_server
```

| 环境 | 工作目录 | 配置路径 | 原因 |
|------|---------|---------|------|
| X86 | `build/` | `../conf/` | 编译定义 `X86_BUILD` |
| ARM (Jetson) | 项目根目录 | `conf/` | 嵌入式部署 |

在其他目录运行会导致配置文件加载失败，日志、ZMQ、ModBus 等模块无法正常初始化。

## 关键依赖关系

- **10_lib**: 外部 C++ 工具库，提供日志管理、定时器、CRC 计算、串口通信、ZMQ 组件、XML 解析、SQLite3 封装等
- **DownSideDataModule**: 核心业务模块，组合 ModBus、文件管理、消息协议等子模块，处理下行数据采集与上行数据上报
- **ZMQ (ZeroMQ)**: 进程间通信，当前监听 `tcp://127.0.0.1:5562`，使用 P2P(发布/订阅) 模式

## Git

- 远程: `git@github.com:leronlu/10_dataServer.git`
- build 产物和日志不纳入版本控制（见 `.gitignore`）
