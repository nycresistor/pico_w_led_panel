#include <stdio.h>
#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"
#include "hardware/rtc.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/sntp.h"
#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include <string.h>


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

void cb_connection(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    printf("CB_CONNECTION");
}

bool done = false;
void cb_complete(void* arg, err_t err) {
    printf("CB_COMPLETE");
    done = true;
}


bool sntp_done = false;

extern "C" {
    void set_system_time(u32_t secs);
}

void set_system_time(u32_t secs){
    time_t t = secs;
    datetime_t dt;
    // set: year, month, day, hour, min, sec-- do we need dotw?
    dt.year = 1900; // NTP epoch is jan 1 1900
    dt.day = 1;
    dt.month = 1;
    uint32_t time_of_day = t % (60*60*24);
    uint32_t day_number = t / (60*60*24);
    dt.sec = time_of_day % 60;
    dt.min = (time_of_day / 60) % 60;
    dt.hour = time_of_day / 3600;

    printf("Time is %d:%d:%d\n",dt.hour,dt.min,dt.sec);
    sntp_done = true;
    rtc_set_datetime(&dt);
}

// Retrieve and set time from an NTP server; returns true on success.
bool retrieve_ntp_time() {
    sntp_done = false;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_init();
    printf("Starting to retrieve time...\n");
    while (!sntp_done) {
        // TODO: time out eventually
        sleep_ms(1);
    }
    printf("Got it.\n");
    sntp_stop();
    return sntp_done;
}


static void mqtt_found(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    int *status = (int*)arg;
    if (ipaddr == NULL) {
        printf("Resolution failed.\n");
        *status = -1;
    } else {
        printf("Resolution succeeded.\n");
        *status = 1;
    }
}


// Main loop
int main()
{
    stdio_init_all();

    sleep_ms(3000);
    /// Connect to the wifi.
    /// TODO: user-facing reporting that wifi connection has failed
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        printf("Failed to initialise wireless architecture; giving up.\n");
        return 1;
    }
    printf("Initialised CYW43 module. Attempting to log in to AP " PW_SSID ".\n");
    cyw43_arch_enable_sta_mode();
    while (cyw43_arch_wifi_connect_timeout_ms(wfssid, wfpass, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Failed to connect to " PW_SSID "; retrying.\n");
    }
    printf("Successfully connected to network.");

    /// Configure the RTC and retrieve time from NTP server.
    rtc_init();
    retrieve_ntp_time();

    /// Attempt to connect to the MQTT server.
    ip_addr_t mqtt_ip;
    while (true)
    {
        printf("Looking up " MQTT_HOSTNAME "...");
        int status = 0;
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
 
    mqtt_client_t* client = mqtt_client_new();
    mqtt_connect_client_info_t client_info = {
        .client_id = "CLIENT-ID-UNIQ",
        .client_user = NULL, // no password by default
        .client_pass = NULL,
        .keep_alive = 0,   // keepalive in seconds
        .will_topic = NULL, // no will topic
    };
    cyw43_arch_lwip_begin();
    err_t err = mqtt_client_connect(client, &mqtt_ip, TCP_PORT, cb_connection, NULL, &client_info);
    cyw43_arch_lwip_end();
    if (err != ERR_OK) { // handle error
        printf("MQTT connect error %d\n",err);
    } else {
        while (!mqtt_client_is_connected(client)) {
            sleep_ms(1);
        }
        printf("Connected!\n");
        cyw43_arch_lwip_begin();
        err_t err = mqtt_publish(client, "house/silly", "Pico W online!", 14, 2, 0, cb_complete, NULL);
        cyw43_arch_lwip_end();
        if (err != ERR_OK) {
            printf("MQTT publish error %d\n",err);
        }
    } 
    while (!done) {
        sleep_ms(1);
    }
    printf("all done\n");
}
