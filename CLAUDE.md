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

### 当前环境状态

- 所有依赖库已安装在 `/usr/local/lib/`（非标准路径）
- CMakeLists.txt 中 `find_library` 会搜索系统默认路径和 `/usr/local/lib`
- 编译定义 `X86_BUILD` 用于 x86 桌面测试环境

## 运行

```bash
cd build && ./data_server
```

程序需要 `conf/` 目录下的 XML 配置文件，路径在 `main.cpp` 中通过 `#ifdef X86_BUILD` 区分（X86: `../conf/`，ARM: `conf/`）。

## 关键依赖关系

- **10_lib**: 外部 C++ 工具库，提供日志管理、定时器、CRC 计算、串口通信、ZMQ 组件、XML 解析、SQLite3 封装等
- **DownSideDataModule**: 核心业务模块，组合 ModBus、文件管理、消息协议等子模块，处理下行数据采集与上行数据上报
- **ZMQ (ZeroMQ)**: 进程间通信，当前监听 `tcp://127.0.0.1:5562`，使用 P2P(发布/订阅) 模式

## Git

- 远程: `git@github.com:leronlu/10_dataServer.git`
- build 产物和日志不纳入版本控制（见 `.gitignore`）
