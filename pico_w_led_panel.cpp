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
// Configuration is done in the config.h header. For more information, look at the
// "config.template.h" file.
#include "config.h"

// SNTP is configured in CMakefiles.txt.
#ifndef AP_SSID
#error Please define the SSID of the network in config.h.
#endif

// Handle defaults.
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_USER
#define MQTT_USER NULL
#endif

#ifndef MQTT_PASS
#define MQTT_PASS NULL
#endif

// The MQTT server name.
#ifndef MQTT_HOSTNAME
#error Please define the network password in config.h.
#endif

const char* ap_ssid = AP_SSID;
const char* ap_pass = AP_PASS;

const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASS;

const char* mqtt_hostname = MQTT_HOSTNAME;

const char* mqtt_config = R"({ 
    "command_topic":"pico_w_led_panel/power", 
    "name":"beeping clock" 
})";

static void cb_connection(mqtt_client_t* client, void* arg, mqtt_connection_status_t status)
{
    printf("CB_CONNECTION\n");
}

bool done = false;
void cb_complete(void* arg, err_t err)
{
    switch (err) {
    ERR_OK: printf("pub ok\n"); break;
    ERR_TIMEOUT: printf("timed out\n"); break;
    ERR_ABRT: printf("sub/unsub failed\n"); break;
    default: printf("Unknown error %d\n",err); break;
    }
    done = true;
}


static void dns_found(const char* hostname, const ip_addr_t* ip_addr, void* arg)
{
    ip_addr_t* returned_ip = (ip_addr_t*)arg;
    if (ip_addr == NULL) {
        printf("Resolution failed for %s.\n",hostname);
        ip_addr_set_zero(returned_ip);
    } else {
        printf("Resolution succeeded for %s.\n",hostname);
        *returned_ip = *ip_addr;
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

const char* month_abbrevs[12] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


int main()
{
    /// Set up basic Pico functionality and then sleep for one second (50ms is fine,
    /// but as a convenience for debugging).
    stdio_init_all();
    sleep_ms(1000);
    /// Start core1, which refreshes the LED display.
    multicore_launch_core1(core1);
    
    /// Connect to the wireless AP.
    // First, initialize the cyw43 module.
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        debug_msg("WiFi init fail");
        printf("Failed to initialise wireless architecture; giving up.\n");
        return 1;
    }
    debug_msg("Joining " AP_SSID);
    printf("Initialised CYW43 module. Attempting to connect to " AP_SSID ".\n");
    // Enable station (AP client) mode.
    cyw43_arch_enable_sta_mode();
    //
    while (cyw43_arch_wifi_connect_timeout_ms(ap_ssid, ap_pass, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Failed to connect to " AP_SSID "; retrying.\n");
        debug_msg( AP_SSID " retry..." );
        sleep_ms(5000);
    }
    debug_msg("Joined " AP_SSID ".");
    printf("Successfully connected to network.\n");

    /*
    debug_msg("Getting time...");
    printf("Retrieving time via SNTP.\n");
    /// Configure the RTC and retrieve time from NTP server.
    start_synchronization(-5, true);
    printf("SNTP complete.\n");
    */

    debug_msg("MQTT connect...");

    /// Attempt to connect to the MQTT server.
    int retries = 10;
    ip_addr_t mqtt_ip;
    ip_addr_set_zero(&mqtt_ip);
    ip_addr_t zero_ip;
    ip_addr_set_zero(&zero_ip);
    printf("Looking up " MQTT_HOSTNAME ".\n");
    switch (dns_gethostbyname(mqtt_hostname, &mqtt_ip, dns_found, (void*)&mqtt_ip)) {
    case ERR_OK:
        break;
    case ERR_INPROGRESS:
        while(retries--) {
            if (ip_addr_cmp(&mqtt_ip,&zero_ip)) {
                sleep_ms(100);
            } else { break; }
        }
        if (retries == 0) { printf("Failed to resolve DNS.\n"); }
        break;
    case ERR_ARG:
        printf("DNS lookup argument error!\n");
        break;
    }
    
    if (!ip_addr_cmp(&mqtt_ip,&zero_ip)) {
        printf("Got nonzero addresss %d.%d.%d.%d, proceeding.\n",
               (mqtt_ip.addr >> 0) & 0xff,
               (mqtt_ip.addr >> 8) & 0xff,
               (mqtt_ip.addr >> 16) & 0xff,
               (mqtt_ip.addr >> 24) & 0xff
               );
        mqtt_client_t* client = mqtt_client_new();
        mqtt_connect_client_info_t client_info = {
            .client_id = "pico_w",
            .client_user = MQTT_USER, // no password by default
            .client_pass = MQTT_PASS,
            .keep_alive = 0, // keepalive in seconds
            .will_topic = NULL, // no will topic
        };
        printf("connecting to client... \n");
        cyw43_arch_lwip_begin();
        err_t err = mqtt_client_connect(client, &mqtt_ip, MQTT_PORT, cb_connection, NULL, &client_info);
        cyw43_arch_lwip_end();
        printf("client connect called.\n");
        if (err != ERR_OK) { // handle error
            printf("MQTT connect error %d\n", err);
        } else {
            printf("waiting for connect\n");
            int retries = 100;
            while (retries--) {
                cyw43_arch_lwip_begin();
                if (mqtt_client_is_connected(client)) break;
                cyw43_arch_lwip_end();
                sleep_ms(100);
            }
            printf("waited for connect\n");
            if (mqtt_client_is_connected(client)) {
                printf("start pub\n");
                // Send home automation message
                cyw43_arch_lwip_begin();
                err_t err = mqtt_publish(client, "homeassistant/light/pico_clock/config", mqtt_config, strlen(mqtt_config), 1, 0, cb_complete, NULL);
                cyw43_arch_lwip_end();
                switch(err) {
                case ERR_ARG: printf("[arg error]\n"); break;
                case ERR_CONN: printf("[conn error]\n"); break;
                case ERR_MEM: printf("[mem error]\n"); break;
                }
                printf("end pub\n");
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
        char buf[16];
        /*
        snprintf(buf, 8, "%2d", dt.hour);
        int col = draw_string(0, buf, false);
        col = draw_string(col, ":", true);
        snprintf(buf, 8, "%02d", dt.min);
        col = draw_string(col, buf, false);
        col += 10;
        snprintf(buf, 16, "%d %s %02d", dt.day, month_abbrevs[dt.month], dt.year%100);
        col = draw_string(col, buf, true);
        */
        swap_buffer();
    }
}
