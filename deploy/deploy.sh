#!/bin/bash
# =============================================================================
# deploy.sh — dataServer (10_dataServer) ARM64 真机部署脚本
#
# 部署策略: 全新发布目录 /mnt/App/arm64/release2/ 下自包含部署, 无软链。
#           结构参考 02_videoServer/deploy/deploy.sh 同模式。
#
#   部署布局 (运行 cwd = /mnt/App/arm64/release2/dataServer/bin/):
#     /mnt/App/arm64/release2/
#     └── dataServer/              自包含目录
#         ├── bin/
#         │   └── data_server      新二进制 (cwd 在此, ../conf/ ../log/ 生效)
#         ├── conf/                ../conf/ 解析 (7 个配置)
#         └── log/                 ../log/ 解析 (日志实际写入位置)
#
#   配置来源: 全部 ← 项目版 conf/ (用户验证通过; 本程序独立运行, 不与其他
#             平台服务组合运行, 无需与 /mnt/App/conf/ 保持一致)
#     LogConfig.xml 为 ../log/ 相对路径 → 日志写部署目录 log/
#
#   旧进程: pkill data_server 与 rtuServer (本项目旧版二进制, 防抢 ZMQ 5562
#           端口与 ModBus 串口); release/rtuServer 文件保留作回滚。
#
#   权限: 必须 root 运行。串口 /dev/ttyS* 为 root:dialout; 与参考脚本一致。
# =============================================================================
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN_SRC="${PROJECT_DIR}/build_arm/data_server"
RELEASE_DIR="/mnt/App/arm64/release2"
DEPLOY_DIR="${RELEASE_DIR}/dataServer"
BIN_DIR="${DEPLOY_DIR}/bin"                # 运行 cwd
CONF_DIR="${DEPLOY_DIR}/conf"              # ../conf/
LOG_DIR="${DEPLOY_DIR}/log"                # ../log/
STAMP="$(date +%Y%m%d%H%M%S)"

if [ "$(id -u)" != "0" ]; then
    echo "✗ 需要 root 权限运行 (串口 /dev/ttyS* 为 root:dialout)"
    echo "  请执行: sudo bash $(readlink -f "$0")"
    exit 1
fi

echo "== [1/8] 检查构建产物"
[ -f "${BIN_SRC}" ] || { echo "✗ 未找到 ${BIN_SRC}, 请先执行 build_arm 构建"; exit 1; }
echo "  源: ${BIN_SRC} ($(ls -lh ${BIN_SRC} | awk '{print $5}'))"

echo "== [2/8] 杀旧进程 (data_server + 旧版 rtuServer)"
pkill -9 -x data_server 2>/dev/null || true
pkill -9 -x rtuServer 2>/dev/null || true
sleep 1

echo "== [3/8] 准备部署目录"
mkdir -p "${BIN_DIR}" "${CONF_DIR}" "${LOG_DIR}"

echo "== [4/8] 备份现有部署"
if [ -f "${BIN_DIR}/data_server" ]; then
    cp "${BIN_DIR}/data_server" "${BIN_DIR}/data_server.bak.${STAMP}"
    echo "  备份二进制 -> bin/data_server.bak.${STAMP}"
fi

echo "== [5/8] 拷贝二进制"
cp "${BIN_SRC}" "${BIN_DIR}/data_server"
chmod +x "${BIN_DIR}/data_server"
echo "  部署: ${BIN_DIR}/data_server ($(ls -lh ${BIN_DIR}/data_server | awk '{print $5}'))"

echo "== [6/8] 同步配置 (全部 ← 项目版)"
for f in LogConfig ZmqCommConfig ModBusMasterConfig ACConfig \
         DownSideDataConfig UpSideDataConfig FileManageConfig; do
    if [ -f "${PROJECT_DIR}/conf/${f}.xml" ]; then
        cp "${PROJECT_DIR}/conf/${f}.xml" "${CONF_DIR}/${f}.xml"
        echo "  ${f}.xml             ← 项目版"
    else
        echo "  ⚠ ${PROJECT_DIR}/conf/${f}.xml 不存在, 跳过 (程序容忍缺失)"
    fi
done

echo "== [7/8] 启动"
cd "${BIN_DIR}"
nohup ./data_server >/dev/null 2>&1 &

echo "== [8/8] 验证"
PID="$(pgrep -x data_server | head -1 || true)"
if [ -n "${PID}" ]; then
    echo "  ✓ 进程存活 pid=${PID}"
    ZMQ=""
    for i in 1 2 3 4 5; do
        sleep 2
        ZMQ="$(ss -tln 2>/dev/null | grep ':5562' || true)"
        [ -n "${ZMQ}" ] && break
    done
    if [ -n "${ZMQ}" ]; then
        echo "  ✓ ZMQ pub 端口 (DOWNSIDE):"
        echo "${ZMQ}" | sed 's/^/    /'
    else
        echo "  ⚠ ZMQ 5562 未监听 (可能模块未使能, 属正常状态)"
    fi
    echo "  最新日志 (log/log):"
    tail -5 "${LOG_DIR}/log" 2>/dev/null | sed 's/^/    /' || echo "    (日志尚未写入, 稍候再查)"
else
    echo "  ✗ 启动失败! 日志尾部:"
    tail -20 "${LOG_DIR}/log" 2>/dev/null | sed 's/^/    /' || true
    exit 1
fi

echo ""
echo "完成。回滚: pkill -9 -x data_server; rm -rf ${DEPLOY_DIR}; 旧版 rtuServer 仍保留于 /mnt/App/arm64/release/rtuServer"
