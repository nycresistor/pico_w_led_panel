#include <stdio.h>
#include "pico/stdlib.h"
#include "sntp_rtc_sync.h"

#include "pico/cyw43_arch.h"
#include "hardware/rtc.h"
#include "lwip/apps/sntp.h"
#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include <string.h>



SyncState synchronization_state = RTC_SYNC_NONE;
int tz_off_s = 0;

/// The LWIP SNTP implementation calls set_system_time when it recieves the time from
/// an SNTP server. We ordinarily have this halt further synchronization and run off
/// the RTC once synchronization has been achieved.

extern "C" {
    void set_system_time(u32_t secs, u32_t frac);
}

#define UNIX_EPOCH 2208988800UL
#define EPOCH_2000 946684800UL

#define DAYS_PER_4_YEARS        (3 * 365 + 366)
#define DAYS_PER_YEAR           365

void set_system_time(u32_t secs, u32_t frac){
    static int month_len[12] = {
        31, 28, 31, 30,
        31, 30, 31, 31,
        30, 31, 30, 31, };
    
    time_t t = secs + tz_off_s - (UNIX_EPOCH + EPOCH_2000); // seconds since NYD 2000
    datetime_t dt;

    uint32_t time_of_day = t % (60*60*24);
    uint32_t day_number = t / (60*60*24);

    dt.sec = time_of_day % 60;
    dt.min = (time_of_day / 60) % 60;
    dt.hour = time_of_day / 3600;

    dt.dotw = (day_number + 6) % 7; // 2020 started on a Saturday
    uint32_t years_since_2000 = (day_number - day_number/(4*365)) / 365;
    dt.year = 2000 + years_since_2000;
    day_number = day_number - (365 * years_since_2000 + years_since_2000/4);
    int month = 0;
    while (day_number >= month_len[month]) {
        if (month == 1 && day_number == 28 && (years_since_2000 % 4) == 0) break; // leap year
        day_number -= month_len[month];
        month++;
    }
    dt.month = month + 1;
    dt.day = day_number + 1;

    //printf("Time is %d:%d:%d\n",dt.hour,dt.min,dt.sec);
    synchronization_state = RTC_SYNC_GOOD;
    rtc_init();
    rtc_set_datetime(&dt);
}


// Retrieve and set time from an NTP server; returns true on success.
void start_synchronization(int tz_off, bool blocking) {
    tz_off_s = tz_off * 60 * 60;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_init();
    synchronization_state = RTC_SYNC_TRYING;
    while (synchronization_state == RTC_SYNC_TRYING) {
        // TODO: time out eventually
        sleep_ms(1);
    }
    printf("Got it.\n");
    sntp_stop();
}

