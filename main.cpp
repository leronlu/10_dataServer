#include <unistd.h>
#include <sys/syscall.h>
using namespace std;
#include "msgProtocol.h"

#include "log_manage.h"
#include "DownSideDataModule.h"

TimerService *timerService = NULL;

void onFrameDownSide1(vector<uint8_t> &data) {
    // reserved - callback used by DownSideDataModule
}

int main(int argc, const char * argv[]) {
    DebugLog *d_log = new DebugLog();
#ifdef X86_BUILD
    d_log->initLogger("../conf/LogConfig.xml");
#else
    d_log->initLogger("conf/LogConfig.xml");
#endif
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

    while(true){
#ifdef X86_BUILD
        usleep(50000);
#else
        usleep(5000);
#endif
    }
}
