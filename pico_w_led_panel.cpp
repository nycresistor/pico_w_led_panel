#include "hardware/rtc.h"
#include "lwip/apps/mqtt.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "sntp_rtc_sync.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "matrix/led_matrix.h"

//// CONFIGURATION

// SNTP is configured in CMakefiles.txt.

// WiFi network configuration for connecting to the AP. Default credentials are for the guest
// network at NYCR.
#ifndef PW_SSID
#define PW_SSID "NYCR24"
#endif

#ifndef PW_PASS
#error Please define the network password by setting the PW_PASS environment variable when running cmake.
#endif

// The MQTT server name.
#ifndef MQTT_HOSTNAME
#error Please define the network password by setting the MQTT_HOSTNAME environment variable when running cmake.
#endif

// The MQTT port number.
#ifndef TCP_PORT
#define TCP_PORT 1883
#endif

char wfssid[] = PW_SSID;
char wfpass[] = PW_PASS;

const char* mqtt_hostname = MQTT_HOSTNAME;

void cb_connection(mqtt_client_t* client, void* arg, mqtt_connection_status_t status)
{
    printf("CB_CONNECTION");
}

bool done = false;
void cb_complete(void* arg, err_t err)
{
    printf("CB_COMPLETE");
    done = true;
}

static void mqtt_found(const char* hostname, const ip_addr_t* ipaddr, void* arg)
{
    int* status = (int*)arg;
    if (ipaddr == NULL) {
        printf("Resolution failed.\n");
        *status = -1;
    } else {
        printf("Resolution succeeded.\n");
        *status = 1;
    }
}

void core1()
{
    ledmatrix_setup();
    while (1) {
        ledmatrix_draw();
    }
}

void debug_msg(const char* format, ...)
{
    char buf[16];
    va_list args;
    va_start(args, format);
    printf(format, args);
    va_end(args);
    va_start(args, format);
    snprintf(buf, 16, format, args);
    va_end(args);
    draw_clear();
    draw_string(0, buf);
    swap_buffer();
}

// Main loop
int main()
{
    stdio_init_all();
    sleep_ms(50);
    multicore_launch_core1(core1);
    /// Connect to the wifi.
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        debug_msg("WiFi init fail");
        printf("Failed to initialise wireless architecture; giving up.\n");
        return 1;
    }
    debug_msg("Joining " PW_SSID);
    printf("Initialised CYW43 module. Attempting to log in to AP " PW_SSID ".\n");
    cyw43_arch_enable_sta_mode();
    while (cyw43_arch_wifi_connect_timeout_ms(wfssid, wfpass, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Failed to connect to " PW_SSID "; retrying.\n");
    }
    debug_msg("Joined " PW_SSID ".");
    printf("Successfully connected to network.");

    debug_msg("Getting time...");
    /// Configure the RTC and retrieve time from NTP server.
    start_synchronization(-5, true);

    debug_msg("Reaching MQTT...");

    /// Attempt to connect to the MQTT server.
    ip_addr_t mqtt_ip;
    int retries = 10;
    int status = 0;
    while (retries-- > 0) {
        printf("Looking up " MQTT_HOSTNAME "...");
        err_t err = dns_gethostbyname(mqtt_hostname, &mqtt_ip,
            mqtt_found, &status);
        while (status == 0) {
            sleep_ms(1);
        }
        if (status == 1) {
            printf("Finished lookup.\n");
            break;
        } else {
            printf("Retrying.\n");
        }
    }
    if (status == 1) {

        mqtt_client_t* client = mqtt_client_new();
        mqtt_connect_client_info_t client_info = {
            .client_id = "CLIENT-ID-UNIQ",
            .client_user = NULL, // no password by default
            .client_pass = NULL,
            .keep_alive = 0, // keepalive in seconds
            .will_topic = NULL, // no will topic
        };
        cyw43_arch_lwip_begin();
        err_t err = mqtt_client_connect(client, &mqtt_ip, TCP_PORT, cb_connection, NULL, &client_info);
        cyw43_arch_lwip_end();
        if (err != ERR_OK) { // handle error
            printf("MQTT connect error %d\n", err);
        } else {
            int retries = 10;
            while (!mqtt_client_is_connected(client)) {
                sleep_ms(100);
                if (--retries == 0)
                    break;
            }
            if (mqtt_client_is_connected(client)) {
                printf("Connected!\n");
                cyw43_arch_lwip_begin();
                err_t err = mqtt_publish(client, "house/silly", "Pico W online!", 14, 2, 0, cb_complete, NULL);
                cyw43_arch_lwip_end();
                if (err != ERR_OK) {
                    printf("MQTT publish error %d\n", err);
                }
                while (!done) {
                    sleep_ms(1);
                }
                printf("all done\n");
            } else {
                printf("MQTT not connected.\n");
            }
        }
    }
    while (1) {
        sleep_ms(100);
        draw_clear();
        static datetime_t dt;
        rtc_get_datetime(&dt);
        static char datetime_buf[256];
        static char* datetime_str = datetime_buf;
        char buf[8];
        snprintf(buf, 8, "%2d", dt.hour);
        int col = draw_string(0, buf, false);
        col = draw_string(col, ":", true);
        snprintf(buf, 8, "%02d", dt.min);
        col = draw_string(col + 1, buf, false);
        swap_buffer();
    }
}
