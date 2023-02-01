#include "pico/stdlib.h"
#include "hardware/rtc.h"

typedef enum {
    RTC_SYNC_NONE = 0,
    RTC_SYNC_TRYING = 1,
    RTC_SYNC_FAILED = 2,
    RTC_SYNC_GOOD = 3
} SyncState;

SyncState get_sync_state();
// tz_off is in hours
void start_synchronization(int tz_off, bool blocking);
