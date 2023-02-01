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

#define DAYS_PER_YEAR 365
#define DAYS_PER_4_YEARS (4*DAYS_PER_YEAR + 1)

// It's everybody's favorite code to implement and test: date handling!
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

    // We are going to ignore 100-year leap day skips. This code will have
    // a Y2.1K problem.
    int y4s = day_number / DAYS_PER_4_YEARS;
    day_number -= y4s * DAYS_PER_4_YEARS;
    dt.year = 2000 + 4*y4s;
    if (day_number > DAYS_PER_YEAR) { // handle leap year
        day_number -= DAYS_PER_YEAR+1; dt.year++;
    }
    int ys = day_number / DAYS_PER_YEAR;
    dt.year += ys; // this completes the year computation
    day_number -= ys * DAYS_PER_YEAR; 
    int month = 0;
    while (day_number >= month_len[month]) {
        if (month == 1 && day_number == 28 && (dt.year % 4) == 0) break; // leap year
        day_number -= month_len[month];
        month++;
    }
    // Correct for expected 1-indexing
    dt.month = month + 1;
    dt.day = day_number + 1;

    rtc_init();
    rtc_set_datetime(&dt);
    synchronization_state = RTC_SYNC_GOOD;
}


// Retrieve and set time from an NTP server; returns true on success.
void start_synchronization(int tz_off, bool blocking) {
    tz_off_s = tz_off * 60 * 60;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_init();
    synchronization_state = RTC_SYNC_TRYING;
    if (blocking) {
        while (synchronization_state == RTC_SYNC_TRYING) {
            // TODO: time out eventually
            sleep_ms(1);
        }
    }
    printf("Got it.\n");
    sntp_stop();
}

