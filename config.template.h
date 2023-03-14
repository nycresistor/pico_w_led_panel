#ifndef __CONFIG_H__
#define __CONFIG_H__

/**
 * This is the template configuration file. You'll either want to copy this to
 * "config.h" and edit it, or soft-link your own file to config.h.
 */

/**
 * If you're looking for the SNTP configuration, please look at the CMakeLists.txt
 * file.
 */

#define AP_SSID "YourNetwork"
#define AP_PASS "YourNetworkPassphrase"

// The port for the MQTT broker. The ordinary default is 1883. You will
// only need to define this if you're using another port.
// #define MQTT_PORT 1883

#define MQTT_HOSTNAME "YourMQTTBroker.local"

// The IP of the MQTT broker. If this is defined, the MQTT broker hostname will
// be ignored.
// #define MQTT_IP "127.0.0.1"

// If you are using authentication, define these.
// #define MQTT_USER "UserName"
// #define MQTT_PASS "Password"

#endif // __CONFIG_H__
