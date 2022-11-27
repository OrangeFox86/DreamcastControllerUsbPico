#include "tusb_config.h"
#include "hal/System/MutexInterface.hpp"
#include "hal/System/LockGuard.hpp"

// CDC is used to create a debug serial interface

void cdc_init(MutexInterface* cdcMutex);
void cdc_task();
