/* mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// ****************************************************************************
//  Workplace Environmental Monitor
//
//  This is a reference deployment application utilizing mbed cloud 1.2.
//
//  By the ARM Reference Design Team
// ****************************************************************************
#include <mbed.h>
#include "compat.h"

#include "commander.h"
#include "displayman.h"
#include "fs.h"
#include "keystore.h"
#include "lcdprogress.h"
#include "m2mclient.h"

#include "rapidjson/allocators.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <algorithm> /* std::min */
#include <errno.h>
#include <factory_configurator_client.h>
#include <fcc_defs.h>
#include <mbed_stats.h>
#include <mbedtls/sha256.h>
#include <mbed-trace-helper.h>
#include <mbed-trace/mbed_trace.h>

#include <OdinWiFiInterface.h>

#include "TSL2591.h"
#include "Sht31/Sht31.h"

#define TRACE_GROUP "main"

// Convert the value of a C macro to a string that can be printed.  This trick
// is straight out of the GNU C documentation.
// (https://gcc.gnu.org/onlinedocs/gcc-4.9.0/cpp/Stringification.html)
#define xstr(s) str(s)
#define str(s) #s

#ifndef DEVTAG
#error "No dev tag created"
#endif

namespace json = rapidjson;

// ****************************************************************************
// DEFINEs and type definitions
// ****************************************************************************
#define MACADDR_STRLEN 18

#define SSID_KEY "wifi.ssid"
#define PASSWORD_KEY "wifi.key"
#define SECURITY_KEY "wifi.encryption"
#define APP_LABEL_KEY "app.label"

#define APP_LABEL_SENSOR_NAME "Label"
#ifndef MBED_CONF_APP_APP_LABEL
#define MBED_CONF_APP_APP_LABEL "dragonfly"
#endif

#define GEO_LAT_KEY "geo.lat"
#define GEO_LONG_KEY "geo.long"
#define GEO_ACCURACY_KEY "geo.accuracy"

#ifndef MBED_CONF_APP_MAX_REPORTED_APS
#define MBED_CONF_APP_MAX_REPORTED_APS 8
#endif

#define JSON_MEM_POOL_INC 64

#define WEM_VERBOSE_PRINTF(type, fmt, ...) \
    do {\
        if (wem_ ##type ## _verbose_enabled) {\
            cmd.printf(fmt, __VA_ARGS__); \
        }\
    } while(0)

enum WEM_THREADS {
    WEM_THREAD_DISPLAY = 0,
    WEM_THREAD_SENSOR_LIGHT,
    WEM_THREAD_DHT,
    WEM_THREAD_COUNT
};

struct dht_sensor {
    uint8_t h_id;
    uint8_t t_id;
    Sht31 *sensor;

    M2MResource *h_res;
    M2MResource *t_res;
};

struct light_sensor {
    uint8_t id;
    TSL2591 *sensor;
    M2MResource *res;
};

struct sensors {
    int event_queue_id_light, event_queue_id_dht;
    struct dht_sensor dht;
    struct light_sensor light;
};

// ****************************************************************************
// Globals
// ****************************************************************************
static DisplayMan display;
static M2MClient *m2mclient;
static NetworkInterface *net;
static EventQueue evq;
static struct sensors sensors;
/* used to stop auto display refresh during firmware downloads */
static int display_evq_id;
static bool wem_sensors_verbose_enabled = false;

static I2C i2c(I2C_SDA, I2C_SCL);
static TSL2591 tsl2591(i2c, TSL2591_ADDR);
static Sht31 sht31(I2C_SDA, I2C_SCL);

//our serial interface cli class
Commander cmd;

// ****************************************************************************
// Generic Helpers
// ****************************************************************************
/**
 * Processes an event queue callback for updating the display
 */
static void display_refresh(DisplayMan *display)
{
    display->refresh();
}

/**
 * Sets the app label on the LCD and Mbed Client
 */
static void set_app_label(M2MClient *m2m, const char *label)
{
    display.set_sensor_status(APP_LABEL_SENSOR_NAME, label);
    m2m->set_resource_value(M2MClient::M2MClientResourceAppLabel, label);
}

// ****************************************************************************
// Sensors
// ****************************************************************************
/**
 * Inits the light sensor object
 */
static void light_init(struct light_sensor *s, M2MClient *mbed_client)
{
    /* add to the display */
    s->id = display.register_sensor("Light", IND_LIGHT);

    /* init the driver */
    s->sensor = &tsl2591;
    s->sensor->init();
    s->sensor->enable();

    s->res = m2mclient->get_resource(M2MClient::M2MClientResourceLightValue);
    m2mclient->set_resource_value(s->res, "0", 1);
}

/**
 * Converts light sensor reading to Lux units
 *
 * Empirical measurement against a light meter under 17
 * different lighting conditions led to the following
 * conversion table
 * Reading     Lux
 * 0.128          392
 * 0.211          767
 * 0.264         1145
 * 0.292         1294
 * 0.317         1407
 * 0.349         1665
 * 0.402         1959
 * 0.457         2580
 * 0.517         2690
 * 0.570         3540
 * 0.592         3770
 * 0.628         4310
 * 0.702         5040
 * 0.816         5880
 * 0.856         6150
 * 0.917         7610
 * 0.958         8330
 *
 * This data is best fit by the power equation
 * Lux = 8251*(reading)^1.5108
 * This equation fits with an R^2 value of 0.9962
 */

unsigned int light_sensor_to_lux(float reading) {
    return lroundf(8250.0 * pow(reading, 1.51));
}

/**
 * Reads a value from the light sensor and publishes to the display
 */
static void light_read(struct light_sensor *s)
{
    size_t size;
    char res_buffer[33] = {0};
    unsigned int lux;

    s->sensor->getALS();
    s->sensor->calcLux();
    //light sensor uses a multiplier to adjust for the lightpipe
    lux = s->sensor->lux*3.7;
    WEM_VERBOSE_PRINTF(sensors, "light: %u\n", lux);
    size = snprintf(res_buffer, sizeof(res_buffer), "%u lux", lux);

    display.set_sensor_status(s->id, res_buffer);
    m2mclient->set_resource_value(s->res, res_buffer, size);
}

/**
 * Inits the temp/humidity combo sensor
 */
static void dht_init(struct dht_sensor *s, M2MClient *mbed_client)
{
    /* add to the display */
    s->t_id = display.register_sensor("Temp", IND_TEMP);
    s->h_id = display.register_sensor("Humidity", IND_HUMIDITY);

    /* init the driver */
    s->sensor = &sht31;

    s->t_res = mbed_client->get_resource(
                    M2MClient::M2MClientResourceTempValue);
    s->h_res = mbed_client->get_resource(
                    M2MClient::M2MClientResourceHumidityValue);

    /* set default values */
    display.set_sensor_status(s->t_id, "0");
    mbed_client->set_resource_value(s->t_res, "0", 1);

    display.set_sensor_status(s->h_id, "0");
    mbed_client->set_resource_value(s->h_res, "0", 1);
}

/**
 * Reads temp and humidity values publishes to the display
 */
static void dht_read(struct dht_sensor *dht)
{
    int size = 0;
    float temperature, humidity;
    char res_buffer[33] = {0};

    //temp and humidity have multiplier to adjust for the case
    temperature = dht->sensor->readTemperature() * .68;
    humidity = dht->sensor->readHumidity() * 1.9;
    tr_debug("DHT: temp = %fC, humi = %f%%\n", temperature, humidity);

    /* verbose printing to screen of sensor values */
    WEM_VERBOSE_PRINTF(sensors, "DHT: temp = %.2fC, humidity = %.2f%%\n", temperature, humidity);

    size = snprintf(res_buffer, sizeof(res_buffer), "%.1f C", temperature);
    m2mclient->set_resource_value(dht->t_res, res_buffer, size);
    display.set_sensor_status(dht->t_id, (char *)res_buffer);

    size = snprintf(res_buffer, sizeof(res_buffer), "%.0f%%", humidity);
    m2mclient->set_resource_value(dht->h_res, res_buffer, size);
    display.set_sensor_status(dht->h_id, (char *)res_buffer);
}

/**
 * Inits all sensors, making them ready to read
 */
static void sensors_init(struct sensors *sensors, M2MClient *mbed_client)
{
    dht_init(&sensors->dht, mbed_client);
    light_init(&sensors->light, mbed_client);
}

/**
 * Starts the periodic sampling of sensor data
 */
static void sensors_start(struct sensors *s, EventQueue *q)
{
    cmd.printf("starting all sensors\n");
    // the periods are prime number multiples so that the LED flashing is more appealing
    s->event_queue_id_light = q->call_every(4700, light_read, &s->light);
    s->event_queue_id_dht = q->call_every(5300, dht_read, &s->dht);
}

/**
 * Stops the periodic sampling of sensor data
 */
static void sensors_stop(struct sensors *s, EventQueue *q)
{
    cmd.printf("stopping all sensors\n");
    q->cancel(s->event_queue_id_light);
    q->cancel(s->event_queue_id_dht);
    s->event_queue_id_light = 0;
    s->event_queue_id_dht = 0;
}

// ****************************************************************************
// Network
// ****************************************************************************
static void network_disconnect(NetworkInterface *net)
{
    net->disconnect();
}

static char *network_get_macaddr(NetworkInterface *net, char *macstr)
{
    memcpy(macstr, net->get_mac_address(), MACADDR_STRLEN);
    return macstr;
}

static nsapi_security_t wifi_security_str2sec(const char *security)
{
    if (0 == strcmp("WPA/WPA2", security)) {
        return NSAPI_SECURITY_WPA_WPA2;

    } else if (0 == strcmp("WPA2", security)) {
        return NSAPI_SECURITY_WPA_WPA2;

    } else if (0 == strcmp("WPA", security)) {
        return NSAPI_SECURITY_WPA;

    } else if (0 == strcmp("WEP", security)) {
        return NSAPI_SECURITY_WEP;

    } else if (0 == strcmp("NONE", security)) {
        return NSAPI_SECURITY_NONE;

    } else if (0 == strcmp("OPEN", security)) {
        return NSAPI_SECURITY_NONE;
    }

    cmd.printf("warning: unknown wifi security type (%s), assuming NONE\n",
           security);
    return NSAPI_SECURITY_NONE;
}

/**
 * brings up wifi
 * */
static WiFiInterface *new_wifi_interface()
{
    return new OdinWiFiInterface();
}

static WiFiInterface *network_create(void)
{
    Keystore k;
    string ssid;

    ssid = MBED_CONF_APP_WIFI_SSID;

    k.open();
    if (k.exists("wifi.ssid")) {
        ssid = k.get("wifi.ssid");
    }
    k.close();

    display.init_network("WiFi");
    display.set_network_status(ssid);

    return new_wifi_interface();
}

/** Scans the wireless network for nearby APs.
 *
 * @param net The network interface to scan on.
 * @param mbed_client The mBed Client cloud interface for uploading data.
 * @return Returns the number of nearby APs on success,
 *         -errno for failure.
 */
static int network_scan(NetworkInterface *net, M2MClient *mbed_client)
{
    int reported;
    int available;
    WiFiAccessPoint *ap;
    WiFiInterface *wifi = (WiFiInterface *)net;

    /* scan for a list of available APs */
    available = wifi->scan(NULL, 0);

    /* cap the number of APs reported */
    reported = min(available, MBED_CONF_APP_MAX_REPORTED_APS);

    /* allocate and scan again */
    ap = new WiFiAccessPoint[reported];
    reported = wifi->scan(ap, reported);

    cmd.printf("Found %d devices, reporting info on %d (max=%d)\n",
               available, reported, MBED_CONF_APP_MAX_REPORTED_APS);

    /* setup the json document and custom allocator */
    json::MemoryPoolAllocator<json::CrtAllocator> allocator(JSON_MEM_POOL_INC);
    json::Document doc(&allocator);
    doc.SetArray();

    /* create a json record for each AP which contains a
     * macAddress and signalStrength key.
     */
    for (int idx = 0; idx < reported; ++idx)
    {
        char macaddr[MACADDR_STRLEN] = {0};

        snprintf(macaddr, sizeof(macaddr), "%02X:%02X:%02X:%02X:%02X:%02X",
            ap[idx].get_bssid()[0], ap[idx].get_bssid()[1], ap[idx].get_bssid()[2],
            ap[idx].get_bssid()[3], ap[idx].get_bssid()[4], ap[idx].get_bssid()[5]);

        /* not the prettiest thing in the world, but it avoids having to create
         * a number of variables to hold some these values.
         * The first argument is a JSON object to be pushed back: this object has
         * two members, the first of which is the macAddress key-value pair; the
         * second member is the signalStrength key-value pair.
         * Both the PushBack and AddMember calls require allocators which are
         * retrieved from the JSON doc.
         */
        doc.PushBack(
            json::Value(json::kObjectType).
            AddMember(
                "macAddress",
                json::Value().SetString(
                    macaddr,
                    strlen(macaddr),
                    doc.GetAllocator()),
                doc.GetAllocator()).
            AddMember(
                "signalStrength",
                ap[idx].get_rssi(),
                doc.GetAllocator()
            ),
            doc.GetAllocator()
        );
    }

    /* We need a StringBuffer and Writer to generate the JSON output that will be sent */
    json::StringBuffer buf;
    json::Writer<json::StringBuffer> writer(buf);
    doc.Accept(writer);

#if MBED_CONF_APP_WIFI_DEBUG
    cmd.printf("%s\n", buf.GetString());
#endif

    /* update the M2MClient resource for network data and send it as a JSON array */
    M2MResource *res = mbed_client->get_resource(
                    M2MClient::M2MClientResourceNetwork);

    m2mclient->set_resource_value(res, buf.GetString(), buf.GetLength());

    /* cleanup */
    delete []ap;

    return reported;
}

static int network_connect(NetworkInterface *net)
{
    int ret;
    char macaddr[MACADDR_STRLEN];
    WiFiInterface *wifi;

    /* code is compiled -fno-rtti so we have to use C cast */
    wifi = (WiFiInterface *)net;

    //wifi login info set to default values
    string ssid     = MBED_CONF_APP_WIFI_SSID;
    string pass     = MBED_CONF_APP_WIFI_PASSWORD;
    string security = MBED_CONF_APP_WIFI_SECURITY;

    //keystore db access
    Keystore k;

    //read the current state
    k.open();

    //use the keystore for ssid?
    if (k.exists(SSID_KEY)) {
        cmd.printf("Using %s from keystore\n", SSID_KEY);
        ssid = k.get(SSID_KEY);
    } else {
        cmd.printf("Using default %s\n", SSID_KEY);
    }

    //use the keystore for pass?
    if (k.exists(PASSWORD_KEY)) {
        cmd.printf("Using %s from keystore\n", PASSWORD_KEY);
        pass = k.get(PASSWORD_KEY);
    } else {
        cmd.printf("Using default %s\n", PASSWORD_KEY);
    }

    //use the keystor for security?
    if (k.exists(SECURITY_KEY)) {
        cmd.printf("Using %s from keystore\n", SECURITY_KEY);
        security = k.get(SECURITY_KEY);
    } else {
        cmd.printf("Using default %s\n", SECURITY_KEY);
    }

    display.set_network_status(ssid);
    cmd.printf("[WIFI] connecting: mac=%s, ssid=%s, encryption=%s\n",
               network_get_macaddr(wifi, macaddr),
               ssid.c_str(),
               security.c_str());

    ret = wifi->connect(ssid.c_str(),
                        pass.c_str(),
                        wifi_security_str2sec(security.c_str()));
    if (0 != ret) {
        cmd.printf("[WIFI] Failed to connect to: %s (%d)\n",
                   ssid.c_str(), ret);
        return ret;
    }

    cmd.printf("[WIFI] connected: mac=%s, ssid=%s, ip=%s, netmask=%s, gateway=%s\n",
               network_get_macaddr(net, macaddr),
               ssid.c_str(),
               net->get_ip_address(),
               net->get_netmask(),
               net->get_gateway());

    return 0;
}

/**
 * Continually attempt network connection until successful
 */
static void sync_network_connect(NetworkInterface *net)
{
    int ret;
    do {
        display.set_network_connecting();
        ret = network_connect(net);
        if (0 != ret) {
            display.set_network_fail();
            cmd.printf("WARN: failed to init network, retrying...\n");
            Thread::wait(2000);
        }
    } while (0 != ret);
}

// ****************************************************************************
// Cloud
// ****************************************************************************
static void mbed_client_keep_alive(M2MClient *m2m)
{
    if (m2m->is_client_registered()) {
        m2m->keep_alive();
    }
}

/**
 * Handles a M2M PUT request on the app label resource
 */
static void mbed_client_handle_put_app_label(M2MClient *m2m)
{
    Keystore k;
    std::string label;

    label = m2m->get_resource_value_str(M2MClient::M2MClientResourceAppLabel);
    if (label.length() == 0) {
        return;
    }

    k.open();
    k.set(APP_LABEL_KEY, label);
    k.write();
    k.close();

    set_app_label(m2m, label.c_str());
}

/**
 * Handles a M2M PUT request on Geo Latitude
 */
static void
mbed_client_handle_put_geo_lat(M2MClient *m2m)
{
    Keystore k;
    std::string val;

    val = m2m->get_resource_value_str(M2MClient::M2MClientResourceGeoLat);
    if (val.length() == 0) {
        return;
    }

    k.open();
    /* special case '-' means delete */
    if (val.length() == 1 && val[0] == '-') {
        k.del(GEO_LAT_KEY);
    } else {
        k.set(GEO_LAT_KEY, val);
    }
    k.write();
    k.close();
}

/**
 * Handles a M2M PUT request on Geo Longitude
 */
static void
mbed_client_handle_put_geo_long(M2MClient *m2m)
{
    Keystore k;
    std::string val;

    val = m2m->get_resource_value_str(M2MClient::M2MClientResourceGeoLong);
    if (val.length() == 0) {
        return;
    }

    k.open();
    /* special case '-' means delete */
    if (val.length() == 1 && val[0] == '-') {
        k.del(GEO_LONG_KEY);
    } else {
        k.set(GEO_LONG_KEY, val);
    }
    k.write();
    k.close();
}

/**
 * Handles a M2M PUT request on Geo Accuracy
 */
static void
mbed_client_handle_put_geo_accuracy(M2MClient *m2m)
{
    Keystore k;
    std::string val;

    val = m2m->get_resource_value_str(M2MClient::M2MClientResourceGeoAccuracy);
    if (val.length() == 0) {
        return;
    }

    k.open();
    /* special case '-' means delete */
    if (val.length() == 1 && val[0] == '-') {
        k.del(GEO_ACCURACY_KEY);
    } else {
        k.set(GEO_ACCURACY_KEY, val);
    }
    k.write();
    k.close();
}

/**
 * Readies the app for a firmware download
 */
void fota_auth_download(M2MClient *mbed_client)
{
    cmd.printf("Firmware download requested\n");

    sensors_stop(&sensors, &evq);
    /* we'll need to manually refresh the display until the firmware
     * update is complete.  it seems that doing *anything* outside of
     * the firmware download's thread context will result in a failed
     * download. */
    evq.cancel(display_evq_id);
    display_evq_id = 0;
    display.set_downloading();
    display.refresh();
    mbed_client->update_authorize(MbedCloudClient::UpdateRequestDownload);

    cmd.printf("Authorization granted\n");
}

/**
 * Readies the app for a firmware install
 */
void fota_auth_install(M2MClient *mbed_client)
{
    cmd.printf("Firmware install requested\n");

    display.set_installing();

    /* firmware download is complete, restart the auto display updates */
    display_evq_id = evq.call_every(DISPLAY_UPDATE_PERIOD_MS,
                                    display_refresh,
                                    &display);

    mbed_client->set_fota_install_requested();
    mbed_client->close();
}

/**
 * Handles authorization requests from the mbed firmware updater
 */
void mbed_client_on_update_authorize(int32_t request)
{
    switch (request) {
        /* Cloud Client wishes to download new firmware. This can have a
         * negative impact on the performance of the rest of the system.
         *
         * The user application is supposed to pause performance sensitive tasks
         * before authorizing the download.
         *
         * Note: the authorization call can be postponed and called later.
         * This doesn't affect the performance of the Cloud Client.
         * */
        case MbedCloudClient::UpdateRequestDownload:
            m2mclient->set_fota_download_requested();
            evq.call(fota_auth_download, m2mclient);
            break;

        /* Cloud Client wishes to reboot and apply the new firmware.
         *
         * The user application is supposed to save all current work before
         * rebooting.
         *
         * Note: the authorization call can be postponed and called later.
         * This doesn't affect the performance of the Cloud Client.
         * */
        case MbedCloudClient::UpdateRequestInstall:
            m2mclient->set_fota_install_requested();
            evq.call(fota_auth_install, m2mclient);
            break;

        default:
            cmd.printf("ERROR: unknown request\n");
            led_set_color(IND_FWUP, IND_COLOR_FAILED);
            led_post();
            break;
    }
}

/**
 * Handles progress updates from the mbed firmware updater
 */
void mbed_client_on_update_progress(uint32_t progress, uint32_t total)
{
    uint32_t percent = progress * 100 / total;
    static uint32_t last_percent = 0;
    const char dl_message[] = "Downloading...";
    const char done_message[] = "Saving (10s)...";

    display.set_progress(dl_message, progress, total);

    if (last_percent < percent) {
        cmd.printf("Downloading: %lu\n", percent);
    }

    if (progress == total) {
        cmd.printf("%s\n", done_message);
        display.set_progress(done_message, 0, 100);
        display.set_download_complete();
    }

    display.refresh();
    last_percent = percent;
}

static void mbed_client_on_registered(void *context)
{
    cmd.printf("mbed client registered\n");
    display.set_cloud_registered();
}

static void mbed_client_on_unregistered(void *context)
{
    M2MClient *m2m;

    m2m = (M2MClient *)context;

    if (m2m->is_fota_install_requested()) {
        cmd.printf("Disconnecting network...\n");
        network_disconnect(net);

        m2m->update_authorize(MbedCloudClient::UpdateRequestInstall);
        cmd.printf("Authorization granted\n");
    }

    cmd.printf("mbed client unregistered\n");
    display.set_cloud_unregistered();
}

static void mbed_client_on_error(void *context, int err_code,
                                 const char *err_name, const char *err_desc)
{
    M2MClient *m2m;

    m2m = (M2MClient *)context;

    cmd.printf("ERROR: mbed client (%d) %s\n", err_code, err_name);
    cmd.printf("    Error details : %s\n", err_desc);
    display.set_cloud_error();
    if ((err_code == MbedCloudClient::ConnectNetworkError) ||
        (err_code == MbedCloudClient::ConnectDnsResolvingFailed)) {
        if (m2m->is_fota_install_requested()) {
            cmd.printf("Ignoring network error due to fota install\n");
            return;
        }
        network_disconnect(net);
        display.set_network_fail();
        display.set_cloud_unregistered();
        cmd.printf("Network connection failed.  Attempting to reconnect.\n");
        /* Because we are running in the mbed client thread context
         * we want to disable our sensors from modifying the mbed
         * client queue while we mess with the network.  This will
         * allow the main context to continue to refresh the display.
         *
         * Holding on to this thread context until netork connection
         * is re-established will prevent the mbed client from backing
         * off the time between connection retries.
         */
        sensors_stop(&sensors, &evq);
        sync_network_connect(net);
        display.set_network_success();
        sensors_start(&sensors, &evq);
        /* CLoud client will automatically try to reconnect.*/
        display.set_cloud_in_progress();
    }
}

static void
mbed_client_on_resource_updated(void *context,
                                M2MClient::M2MClientResource resource)
{
    M2MClient *m2m;
    M2MResource *res;

    m2m = (M2MClient *)context;

    switch (resource) {
    case M2MClient::M2MClientResourceAutoGeoLat:
    case M2MClient::M2MClientResourceAutoGeoLong:
    case M2MClient::M2MClientResourceAutoGeoAccuracy:
        cmd.printf("INFO: auto geolocation data received\n");
        break;
    case M2MClient::M2MClientResourceAppLabel:
        evq.call(mbed_client_handle_put_app_label, m2m);
        break;
    case M2MClient::M2MClientResourceGeoLat:
        evq.call(mbed_client_handle_put_geo_lat, m2m);
        break;
    case M2MClient::M2MClientResourceGeoLong:
        evq.call(mbed_client_handle_put_geo_long, m2m);
        break;
    case M2MClient::M2MClientResourceGeoAccuracy:
        evq.call(mbed_client_handle_put_geo_accuracy, m2m);
        break;
    default:
        res = m2m->get_resource(resource);
        if (NULL != res) {
            cmd.printf("WARN: unsupported PUT request: resource=%d, uri_path=%s\n",
                       resource, res->uri_path());
        } else {
            cmd.printf("WARN: unsupported PUT request on unregistered resource=%d\n",
                       resource);
        }
        break;
    }
}

static int register_mbed_client(NetworkInterface *iface, M2MClient *mbed_client)
{
    mbed_client->on_registered(NULL, mbed_client_on_registered);
    mbed_client->on_unregistered(mbed_client, mbed_client_on_unregistered);
    mbed_client->on_error(mbed_client, mbed_client_on_error);
    mbed_client->on_update_authorize(mbed_client_on_update_authorize);
    mbed_client->on_update_progress(mbed_client_on_update_progress);
    mbed_client->on_resource_updated(mbed_client,
                                     mbed_client_on_resource_updated);

    display.set_cloud_in_progress();
    mbed_client->call_register(iface);

    /* set up a keep-alive interval to send registration updates to the
     * mbed cloud server to avoid deregistration/registration issues. */
    evq.call_every((MBED_CLOUD_CLIENT_LIFETIME / 4) * 1000,
                   mbed_client_keep_alive,
                   mbed_client);
    return 0;
}

static int init_fcc(void)
{
    fcc_status_e ret;

    ret = fcc_init();
    if (ret != FCC_STATUS_SUCCESS) {
        cmd.printf("ERROR: fcc init failed: %d\n", ret);
        return ret;
    }

    ret = fcc_developer_flow();
    if (ret == FCC_STATUS_KCM_FILE_EXIST_ERROR) {
        cmd.printf("fcc: developer credentials already exists\n");
    } else if (ret != FCC_STATUS_SUCCESS) {
        cmd.printf("ERROR: fcc failed to load developer credentials\n");
        return ret;
    }

    ret = fcc_verify_device_configured_4mbed_cloud();
    if (ret != FCC_STATUS_SUCCESS) {
        cmd.printf("ERROR: fcc device not configured for mbed cloud\n");
        return ret;
    }

    return 0;
}

// ****************************************************************************
// Generic Helpers
// ****************************************************************************
static int platform_init(void)
{
    int ret;

    /* setup the display */
    display.init(MBED_CONF_APP_VERSION);
    display.refresh();

#if MBED_CONF_MBED_TRACE_ENABLE
    /* Create mutex for tracing to avoid broken lines in logs */
    if (!mbed_trace_helper_create_mutex()) {
        cmd.printf("ERROR: Mutex creation for mbed_trace failed!\n");
        return -EACCES;
    }

    /* Initialize mbed trace */
    mbed_trace_init();
    mbed_trace_mutex_wait_function_set(mbed_trace_helper_mutex_wait);
    mbed_trace_mutex_release_function_set(mbed_trace_helper_mutex_release);
#endif

    /* init the keystore */
    ret = Keystore::init();
    if (0 != ret) {
        printf("ERROR: keystore init failed: %d\n", ret);
        return ret;
    }
    printf("keystore init OK\n");

    return 0;
}

static void platform_shutdown()
{
    /* stop the EventQueue */
    evq.break_dispatch();
    Keystore::shutdown();
}

// ****************************************************************************
// call back handlers for commandline interface
// ****************************************************************************
static void print_sha256(uint8_t *sha)
{
    for (size_t i = 0; i < 32; ++i) {
        cmd.printf("%02x", sha[i]);
    }
}

static void print_hex(uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len;) {
        cmd.printf("%02x ", buf[i]);
        if (++i % 16 == 0) cmd.printf("\n");
    }
}

static void cmd_cb_kcmls(vector<string>& params)
{
    uint8_t *buf;
    size_t real_size = 0;
    const size_t buf_size = 2048;
    uint8_t sha[32]; /* SHA256 outputs 32 bytes */

    buf = (uint8_t *)malloc(buf_size);
    if (buf == NULL) {
        cmd.printf("ERROR: failed to allocate tmp buffer\n");
        return;
    }

#define PRINT_CONFIG_ITEM(x) \
    do { \
        memset(buf, 0, buf_size); \
        int pcpret = get_config_parameter(x, buf, buf_size, &real_size); \
        if (pcpret == CCS_STATUS_SUCCESS) { \
            cmd.printf("%s: %s\n", x, buf); \
        } else { \
            cmd.printf("%s: FAIL (%d)\n", x, pcpret); \
        } \
    } while (false);

#define PRINT_CONFIG_CERT(x) \
    do { \
        memset(buf, 0, buf_size); \
        int pccret = get_config_certificate(x, buf, buf_size, &real_size); \
        if (pccret == CCS_STATUS_SUCCESS) { \
            cmd.printf("%s: \n", x); \
            cmd.printf("sha="); \
            mbedtls_sha256(buf, std::min(real_size, buf_size), sha, 0); \
            print_sha256(sha); \
            cmd.printf("\n"); \
            print_hex(buf, std::min(real_size, buf_size)); \
            cmd.printf("\n"); \
        } else { \
            cmd.printf("%s: FAIL (%d)\n", x, pccret); \
        } \
    } while (false)

#define PRINT_CONFIG_KEY(x) \
    do { \
        memset(buf, 0, buf_size); \
        int pccret = get_config_private_key(x, buf, buf_size, &real_size); \
        if (pccret == CCS_STATUS_SUCCESS) { \
            cmd.printf("%s: \n", x); \
            cmd.printf("sha="); \
            mbedtls_sha256(buf, std::min(real_size, buf_size), sha, 0); \
            print_sha256(sha); \
            cmd.printf("\n"); \
            print_hex(buf, std::min(real_size, buf_size)); \
            cmd.printf("\n"); \
        } else { \
            cmd.printf("%s: FAIL (%d)\n", x, pccret); \
        } \
    } while (false)

    /**
    * Device general info
    */
    PRINT_CONFIG_ITEM(g_fcc_use_bootstrap_parameter_name);
    PRINT_CONFIG_ITEM(g_fcc_endpoint_parameter_name);
    PRINT_CONFIG_ITEM(KEY_INTERNAL_ENDPOINT); /*"mbed.InternalEndpoint"*/
    PRINT_CONFIG_ITEM(KEY_ACCOUNT_ID); /* "mbed.AccountID" */

    /**
    * Device meta data
    */
    PRINT_CONFIG_ITEM(g_fcc_manufacturer_parameter_name);
    PRINT_CONFIG_ITEM(g_fcc_model_number_parameter_name);
    PRINT_CONFIG_ITEM(g_fcc_device_type_parameter_name);
    PRINT_CONFIG_ITEM(g_fcc_hardware_version_parameter_name);
    PRINT_CONFIG_ITEM(g_fcc_memory_size_parameter_name);
    PRINT_CONFIG_ITEM(g_fcc_device_serial_number_parameter_name);
    PRINT_CONFIG_ITEM(KEY_DEVICE_SOFTWAREVERSION);/* "mbed.SoftwareVersion" */

    /**
    * Time Synchronization
    */
    PRINT_CONFIG_ITEM(g_fcc_current_time_parameter_name);
    PRINT_CONFIG_ITEM(g_fcc_device_time_zone_parameter_name);
    PRINT_CONFIG_ITEM(g_fcc_offset_from_utc_parameter_name);

    /**
    * Bootstrap configuration
    */
    PRINT_CONFIG_CERT(g_fcc_bootstrap_server_ca_certificate_name);
    PRINT_CONFIG_ITEM(g_fcc_bootstrap_server_crl_name);
    PRINT_CONFIG_ITEM(g_fcc_bootstrap_server_uri_name);
    PRINT_CONFIG_CERT(g_fcc_bootstrap_device_certificate_name);
    PRINT_CONFIG_CERT(g_fcc_bootstrap_device_private_key_name);

    /**
    * LWm2m configuration
    */
    PRINT_CONFIG_CERT(g_fcc_lwm2m_server_ca_certificate_name);
    PRINT_CONFIG_ITEM(g_fcc_lwm2m_server_crl_name);
    PRINT_CONFIG_ITEM(g_fcc_lwm2m_server_uri_name);
    PRINT_CONFIG_CERT(g_fcc_lwm2m_device_certificate_name);
    PRINT_CONFIG_KEY(g_fcc_lwm2m_device_private_key_name);

    /**
    * Firmware update
    */
    PRINT_CONFIG_CERT(g_fcc_update_authentication_certificate_name);

    free(buf);
}

#if MBED_STACK_STATS_ENABLED == 1 || MBED_HEAP_STATS_ENABLED == 1
static void cmd_cb_mstat(vector<string>& params)
{
#if MBED_HEAP_STATS_ENABLED == 1
    mbed_stats_heap_t heap_stats;

    mbed_stats_heap_get(&heap_stats);
    /* heap_stats.current_size: bytes allocated currently */
    cmd.printf("heap used: %lu\n", heap_stats.current_size);
    /* heap_stats.max_size: max bytes allocated at a given time */
    cmd.printf("heap high: %lu\n", heap_stats.max_size);
    /* heap_stats.reserved_size: current bytes allocated for the heap */
    cmd.printf("heap total: %lu\n", heap_stats.reserved_size);
    /* heap_stats.total_size: cumulative sum of bytes ever allocated */
    cmd.printf("heap cumulative: %lu\n", heap_stats.total_size);
    /* heap_stats.alloc_cnt: current number of allocations */
    cmd.printf("heap allocs: %lu\n", heap_stats.alloc_cnt);
    /* heap_stats.alloc_fail_cnt: number of failed allocations */
    cmd.printf("heap fails: %lu\n", heap_stats.alloc_fail_cnt);
#endif

#if MBED_STACK_STATS_ENABLED == 1
    int count;
    mbed_stats_stack_t *stats;

    count = osThreadGetCount();
    stats = (mbed_stats_stack_t *)malloc(count * sizeof(*stats));

    count = mbed_stats_stack_get_each(stats, count);
    for (int i = 0; i < count; i++) {
        cmd.printf("thread[%d] id: 0x%08x\n", i, stats[i].thread_id);
        cmd.printf("thread[%d] stack high: %u\n", i, stats[i].max_size);
        cmd.printf("thread[%d] stack total: %u\n", i, stats[i].reserved_size);
    }

    free(stats);
    stats = NULL;
#endif
}
#endif

static void cmd_cb_del(vector<string>& params)
{
    //check params
    if (params.size() >= 2) {
        //the db
        Keystore k;

        //read the file
        k.open();

        //delete the given key
        k.del(params[1]);

        //write the changes back out
        k.write();
        k.close();

        //let user know
        cmd.printf("Deleted key %s\n",
                   params[1].c_str());
    } else {
        cmd.printf("Not enough arguments!\n");
    }
}

static void cmd_cb_get(vector<string>& params)
{
    //check params
    if (params.size() >= 1) {
        //database
        Keystore k;

        //don't show all keys by default
        bool ball = false;

        //read current values
        k.open();

        //if no param set to *
        if (params.size() == 1) {
            ball = true;
        } else if (params[1] == "*") {
            ball = true;
        }

        //show all keys?
        if (ball) {
            //get all keys
            vector<string> keys = k.keys();

            //walk the keys
            for (unsigned int n = 0; n < keys.size(); n++) {
                //get value
                string val = k.get(keys[n]);

                //format for display
                cmd.printf("%s=%s\n",
                           keys[n].c_str(),
                           val.c_str());
            }
        } else {

            // if not get one key
            string val = k.get(params[1]);

            //return just the value
            cmd.printf("%s\n",
                       val.c_str());
        }
    } else {
        cmd.printf("Not enough arguments!\n");
    }
}

static void cmd_cb_set(vector<string>& params)
{
    //check params
    if (params.size() >= 2) {

        //db
        Keystore k;

        //read the file into db
        k.open();

        //default to empty
        string strvalue = "";

        //create our value
        for (size_t x = 2; x < params.size(); x++) {
            //don't prepend space on 1st word
            if (x != 2) {
                strvalue += " ";
            }
            strvalue += params[x];
        }

        //make the change
        k.set(params[1], strvalue);

        //write the file back out
        k.write();
        k.close();

        //return just the value
        cmd.printf("%s=%s\n",
                   params[1].c_str(),
                   strvalue.c_str());

    } else {
        cmd.printf("Not enough arguments!\n");
    }
}

static void cmd_cb_reboot(vector<string>& params)
{
    cmd.printf("\nRebooting...");
    NVIC_SystemReset();
}

static void cmd_cb_format(vector<string>& params)
{
    int ret;
    string type;

    string usage = "usage: format <type>\n"
                   "    supported types: fat";

    if (params.size() < 2) {
        cmd.printf("ERROR: missing fs type\n");
        return;
    }

    if (params.size() > 2) {
        cmd.printf("ERROR: too many parameters\n");
        return;
    }

    type = params[1];
    if (type == "fat") {
        fs_unmount();
        ret = fs_format();
        fs_mount();

        if (0 != ret) {
            cmd.printf("ERROR: keystore format failed: %d\n", ret);
            return;
        }

        cmd.printf("SUCCESS\n");

    } else if (type == "-h" || type == "--help") {
        cmd.printf("%s\n", usage.c_str());

    } else {
        cmd.printf("ERROR: unsupported fs type: %s\n", params[1].c_str());
        cmd.printf("%s\n", usage.c_str());
    }
}

static void cmd_cb_test(vector<string>& params)
{
    fs_test();
}

static void cmd_cb_ls(vector<string>& params)
{
    std::string path;

    if (params.size() > 1) {
        path = params[1];
    } else {
        path = "/";
    }

    fs_ls(path);
}

static void cmd_cb_cat(vector<string>& params)
{
    if (params.size() <= 1) {
        cmd.printf("Not enough arguments!\n");
        return;
    }

    fs_cat(params[1]);
}

static void cmd_cb_rm(vector<string>& params)
{
    if (params.size() <= 1) {
        cmd.printf("Not enough arguments!\n");
        return;
    }

    fs_remove(params[1]);
}

static void cmd_cb_mkdir(vector<string>& params)
{
    if (params.size() <= 1) {
        cmd.printf("Not enough arguments!\n");
        return;
    }

    fs_mkdir(params[1]);
}

static void cmd_cb_reset(vector<string>& params)
{
    Keystore k;

    //default to delete nothing
    bool bcerts   = false;
    bool boptions = false;

    //check params
    if (params.size() > 1) {

        //set up the users options
        if (params[1] == "certs") {
            bcerts = true;
        } else if (params[1] == "options") {
            boptions = true;
        } else if (params[1] == "all") {
            bcerts = true;
            boptions = true;
        } else {
            cmd.printf("Unknown parameters.\n");
        }
    } else {
        //no params defaults options
        boptions = true;
    }

    //delete fcc certifications?
    if (bcerts) {
        int ret = fcc_storage_delete();

        if (ret != FCC_STATUS_SUCCESS) {
            cmd.printf("ERROR: fcc delete failed: %d\n", ret);
        }
    }

    //delete from keystore?
    if (boptions) {
        k.kill_all();
    }
}

static void cmd_cb_verbose(vector<string>& params)
{
    if (params.size() < 2) {
        cmd.printf("ERROR: Invalid usage of verbose!\n");
        cmd.printf("Usage: verbose <type> [off|on], defaults to off.\n");
        cmd.printf("    Current types supported: sensors\n");
        return;
    }

    if (params[1] == "sensors") {
        /* print the current status of verbosity */
        if (params.size() < 3) {
            cmd.printf("Sensor verbosity is currently %s\n",
                    wem_sensors_verbose_enabled ? "enabled" : "disabled");
            return;
        }

        /* if an additional parameter was supplied the check that */
        if (params[2] == "on") {
            wem_sensors_verbose_enabled = true;
            cmd.printf("verbose sensor printing enabled\n");
        } else if (params[2] == "off") {
            wem_sensors_verbose_enabled = false;
            cmd.printf("verbose sensor printing disabled\n");
        } else {
            cmd.printf("ERROR: Invalid parameter supplied! %s\n", params[1].c_str());
            return;
        }
    } else {
        /* if we add more than just sensors we should use
         * this as a catch all to enable or disable all
         */
        cmd.printf("ERROR: unsupported option %s!\n", params[1].c_str());
    }
}

static void cmd_pump(Commander *cmd)
{
    cmd->pump();
}

void cmd_on_ready(void)
{
    evq.call(cmd_pump, &cmd);
}

/**
 * Sets up the command shell
 */
void init_commander(void)
{
    cmd.on_ready(cmd_on_ready);

    // add our callbacks
    cmd.add("get",
            "Get the value for the given configuration option. Usage: get [option] defaults to *=all",
            cmd_cb_get);

    cmd.add("set",
            "Set a configuration option to a the given value. Usage: set <option> <value>",
            cmd_cb_set);

    cmd.add("del",
            "Delete a configuration option from the store. Usage: del <option>",
            cmd_cb_del);

    cmd.add("reboot",
            "Reboot the device. Usage: reboot",
            cmd_cb_reboot);

    cmd.add("reset",
            "Reset configuration options and/or certificates. Usage: reset [options|certs|all] defaults to options",
            cmd_cb_reset);

#if MBED_STACK_STATS_ENABLED == 1 || MBED_HEAP_STATS_ENABLED == 1
    cmd.add("mstat",
            "Show runtime heap and stack statistics.",
            cmd_cb_mstat);
#endif

    cmd.add("verbose",
            "Enables verbose printing of sensor values when set 'on'. Usage: verbose <type> [off|on], defeaults to off",
            cmd_cb_verbose);

    cmd.add("format",
            "Format the internal file system. Usage: format <fs-type>",
            cmd_cb_format);

    cmd.add("ls",
            "List directory entries. Usage: ls <path>",
            cmd_cb_ls);

    cmd.add("cat",
            "Read the contents of a file. Usage: cat <path>",
            cmd_cb_cat);

    cmd.add("rm",
            "Remove a file. Usage: rm <path>",
            cmd_cb_rm);

    cmd.add("mkdir",
            "Make a directory. Usage: mkdir <path>",
            cmd_cb_mkdir);

    cmd.add("test",
            "Run the keystore tests. Usage: test",
            cmd_cb_test);

    cmd.add("kcmls",
            "Show KCM config parameters",
            cmd_cb_kcmls);

    //display the banner
    cmd.banner();

    //prime the serial
    cmd.init();
}

static void init_app_label(M2MClient *m2m)
{
    Keystore k;
    string label;

    display.register_sensor(APP_LABEL_SENSOR_NAME);

    k.open();
    if (k.exists(APP_LABEL_KEY)) {
        label = k.get(APP_LABEL_KEY);
    } else {
        label = MBED_CONF_APP_APP_LABEL;
    }
    k.close();

    set_app_label(m2m, label.c_str());
}

static void init_geo(M2MClient *m2m)
{
    Keystore k;

    k.open();

    if (k.exists(GEO_LAT_KEY)) {
        m2m->set_resource_value(M2MClient::M2MClientResourceGeoLat,
                                k.get(GEO_LAT_KEY));
#ifdef MBED_CONF_APP_GEO_LAT
    } else {
        m2m->set_resource_value(M2MClient::M2MClientResourceGeoLat,
                                MBED_CONF_APP_GEO_LAT);
#endif
    }

    if (k.exists(GEO_LONG_KEY)) {
        m2m->set_resource_value(M2MClient::M2MClientResourceGeoLong,
                                k.get(GEO_LONG_KEY));
#ifdef MBED_CONF_APP_GEO_LONG
    } else {
        m2m->set_resource_value(M2MClient::M2MClientResourceGeoLong,
                                MBED_CONF_APP_GEO_LONG);
#endif
    }

    if (k.exists(GEO_ACCURACY_KEY)) {
        m2m->set_resource_value(M2MClient::M2MClientResourceGeoAccuracy,
                                k.get(GEO_ACCURACY_KEY));
#ifdef MBED_CONF_APP_GEO_ACCURACY
    } else {
        m2m->set_resource_value(M2MClient::M2MClientResourceGeoAccuracy,
                                MBED_CONF_APP_GEO_ACCURACY);
#endif
    }

    k.close();
}

static void init_app(EventQueue *queue)
{
    int ret;

    /* create the network */
    cmd.printf("init network\n");
    net = network_create();
    if (NULL == net) {
        cmd.printf("ERROR: failed to create network stack\n");
        display.set_network_fail();
        return;
    }

    m2mclient = new M2MClient();
    m2mclient->init();

    init_app_label(m2mclient);
    init_geo(m2mclient);

    /* workaround: go ahead and connect the network.  it doesn't like being
     * polled for status before a connect() is attempted.
     * in addition, the fcc code requires a connected network when generating
     * creds the first time, so we need to spin here until we have an active
     * network. */
    sync_network_connect(net);
    cmd.printf("init network: OK\n");

    /* scan the network for nearby devices or APs. */
    cmd.printf("scanning network for nearby devices...\n");
    display.set_network_scanning();
    ret = network_scan(net, m2mclient);
    if (0 > ret) {
        cmd.printf("WARN: failed to scan network! %d\n", ret);
    }

    /* network_scan can take some time to perform when on WiFi, and since we
     * don't have a separate indicator to show progress we delay the setting
     * of success until we have finished.
     */
    display.set_network_success();

    /* initialize the factory configuration client
     * WARNING: the network must be connected first, otherwise this
     * will not return if creds haven't been provisioned for the first time.
     * */
    cmd.printf("init factory configuration client\n");
    ret = init_fcc();
    if (0 != ret) {
        cmd.printf("ERROR: failed to init factory configuration client: %d\n",
                   ret);
        return;
    }
    cmd.printf("init factory configuration client: OK\n");

    cmd.printf("init sensors\n");
    sensors_init(&sensors, m2mclient);
    sensors_start(&sensors, &evq);

    /* connect to mbed cloud */
    cmd.printf("init mbed client\n");
    /* WARNING: the sensor resources must be added to the mbed client
     * before the mbed client connects to the cloud, otherwise the
     * sensor resources will not exist in the portal. */
    register_mbed_client(net, m2mclient);
}

// ****************************************************************************
// Main
// main() runs in its own thread in the OS
//
// Be aware of 3 threads of execution.
// 1. The init thread is kicked off when the app first starts and is
// responsible for bringing up the mbed client, the network, the sensors,
// etc., and exits as soon as initialization is complete.
// 2. The main thread dispatches the event queue and is where all normal
// runtime operations are processed.
// 3. The firmware update thread runs in the context of the mbed client and
// executes callbacks in our app.  When a firmware update begins and a
// download started, the event queue in the main thread must be halted until
// the download completes.  Through a good deal of testing, it seems that
// any work performed outside of the mbed client's context while a download
// is in progress will cause the downloaded file to become corrupt and
// therefore cause the firmware update to fail.
// ****************************************************************************
int main()
{
    int ret;

    /* stack size 2048 is too small for fcc_developer_flow() */
    Thread thread(osPriorityNormal, 4224);

    /* init the console manager so we can printf */
    init_commander();

    /* the bootloader doesn't seem to print a final newline before passing
     * control to the app, which causes the version string to be mangled
     * when printed on the console. if we print a newline first before
     * printing anything else, we can work around the issue.
     */
    cmd.printf("\n");
    cmd.printf("Workplace Environmental Monitor version: %s\n", MBED_CONF_APP_VERSION);
    cmd.printf("                           code version: " xstr(DEVTAG) "\n");

    /* minimal init sequence */
    cmd.printf("init platform\n");
    ret = platform_init();
    if (0 != ret) {
        cmd.printf("init platform: FAIL\n");
    } else {
        cmd.printf("init platform: OK\n");
    }

    /* set the refresh rate of the display. */
    display_evq_id = evq.call_every(DISPLAY_UPDATE_PERIOD_MS, display_refresh, &display);

    /* use a separate thread to init the remaining components so that we
     * can continue to refresh the display */
    thread.start(callback(init_app, &evq));

    cmd.printf("entering run loop\n");
    evq.dispatch();
    cmd.printf("exited run loop\n");

    platform_shutdown();
    cmd.printf("exiting main\n");
    return 0;
}
