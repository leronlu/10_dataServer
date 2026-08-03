#include <unistd.h>
#include <csignal>
#include <sys/syscall.h>
using namespace std;
#include "msgProtocol.h"

#include "log_manage.h"
#include "DownSideDataModule.h"

TimerService *timerService = NULL;

static void onSignal(int) {
    // 直接退出：库线程在 exit 清理阶段存在竞态（曾触发段错误），_exit 可确定性终止
    _exit(0);
}

void onFrameDownSide1(vector<uint8_t> &data) {
    // reserved - callback used by DownSideDataModule
}

int main(int argc, const char * argv[]) {
    DebugLog *d_log = new DebugLog();
    d_log->initLogger("../conf/LogConfig.xml");
    d_log->loadConfig();
    d_log->run();

    timerService = new TimerService();
    timerService->StartSysCount();

    DataManager *d_dataManager = new DataManager();
    d_dataManager->loadConfig();
    usleep(2000);

    DownSideDataModule *d_downSideData = NULL;
    d_downSideData = new DownSideDataModule();
    d_downSideData->setCallbackFunction(onFrameDownSide1);
    d_downSideData->initModule();

    BaseDataConfig m_dataConfig;
    m_dataConfig = d_downSideData->getDataConfig();

    d_downSideData->run();

    // 必须在所有模块初始化之后注册，否则会被库的 handler 覆盖
    struct sigaction sa;
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    while(true){
#ifdef X86_BUILD
        usleep(50000);
#else
        usleep(5000);
#endif
    }

    return 0;
}
