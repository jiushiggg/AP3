#ifndef _COREHANDLECMD_H_
#define _COREHANDLECMD_H_

#include "core.h"

void Core_HandleSoftReboot(void);
void Core_HandleEslUpdataReq(core_task_t *task);
void Core_HandleG3Heartbeat(core_task_t *task);
void Core_HandleQuerySoftVer(core_task_t *task);
void Core_HandleQueryStatus(core_task_t *task);
void Core_SetDebugLevel(core_task_t *task);
void Core_SetRfLog(core_task_t *task);
void Core_BackToIdel(core_task_t *task);
void Core_HandleRummanTest(core_task_t *task);
void Core_HandleFTBerTest(core_task_t *task);
void Core_HandleScanBG(core_task_t *task);
void Core_HandleRcReqRequest(core_task_t *task);
void Core_HandleScanWkup(core_task_t *task);
void Core_HandleAssAck(core_task_t *task);
void core_handle_rf_txrx(core_task_t *task);
void Core_HandleScanAck(core_task_t *task);
void Core_HandleCalibratePower(core_task_t *task);
void Core_HandleCalibrateFreq(core_task_t *task);

#endif
