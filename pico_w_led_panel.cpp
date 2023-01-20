#include <stdio.h>
#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"
#include "pico/cyw43_arch.h"
#include "hardware/rtc.h"
#include "lwip/apps/mqtt.h"
#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include <string.h>

// WiFi network configuration for connecting to the AP. Default credentials are for the guest
// network at NYCR.
#ifndef PW_SSID
#define PW_SSID "NYCR24"
#endif

#ifndef PW_PASS
#error Please define the network password by setting the PW_PASS environment variable when running cmake.
#endif

// Pool.ntp.org is a good place to get our time from; don't spam it.
#ifndef NTP_HOSTNAME
#define NTP_HOSTNAME "pool.ntp.org"
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
#define IP_ADDR_S "127.0.0.1"

const char* ntp_hostname = NTP_HOSTNAME;
const char* mqtt_hostname = MQTT_HOSTNAME;

void cb_connection(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    printf("CB_CONNECTION");
}

bool done = false;
void cb_complete(void* arg, err_t err) {
    printf("CB_COMPLETE");
    done = true;
}

// Main loop
int main()
{
    stdio_init_all();

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

    ip_addr_t mqtt_ip;
    if (ipaddr_aton(IP_ADDR_S,&mqtt_ip) == 0) {
        printf("Failed to convert MQTT IP, aborting.\n"); return 1;
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
