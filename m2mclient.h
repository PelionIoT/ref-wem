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

//----------------------------------------------------------------------------
// The confidential and proprietary information contained in this file may
// only be used by a person authorised under and to the extent permitted
// by a subsisting licensing agreement from ARM Limited or its affiliates.
//
// (C) COPYRIGHT 2016 ARM Limited or its affiliates.
// ALL RIGHTS RESERVED
//
// This entire notice must be reproduced on all copies of this file
// and copies of this file may only be made by a person if such person is
// permitted to do so under the terms of a subsisting license agreement
// from ARM Limited or its affiliates.
//----------------------------------------------------------------------------
#ifndef M2MCLIENT_H
#define M2MCLIENT_H

#include "m2mresources.h"

#include <MbedCloudClient.h>
#include <NetworkInterface.h>
#include <m2mdevice.h>

#include <map>
#include <stdio.h>

#define M2MCLIENT_F_REGISTER_CALLED           1 << 0
#define M2MCLIENT_F_REGISTERED                1 << 1
#define M2MCLIENT_F_FOTA_DL_REQD              1 << 2
#define M2MCLIENT_F_FOTA_DL_GRANTED           1 << 3
#define M2MCLIENT_F_FOTA_INSTALL_REQD         1 << 4
#define M2MCLIENT_F_FOTA_INSTALL_GRANTED      1 << 5

class M2MClient : public MbedCloudClientCallback {

public:
    enum M2MClientResource {
        /* Application Info */
        M2MClientResourceAppLabel,
        M2MClientResourceAppVersion,

        /* Temperature Sensor */
        M2MClientResourceTempValue,

        /* Humidity Sensor */
        M2MClientResourceHumidityValue,

        /* Light Sensor */
        M2MClientResourceLightValue,

        /* Network Data */
        M2MClientResourceNetwork,

        /* Geo Location specified by the user */
        M2MClientResourceGeoLat,
        M2MClientResourceGeoLong,
        M2MClientResourceGeoAccuracy,
        M2MClientResourceGeoType,

        /* Geo Location determined by automatic means */
        M2MClientResourceAutoGeoLat,
        M2MClientResourceAutoGeoLong,
        M2MClientResourceAutoGeoAccuracy,
        M2MClientResourceAutoGeoType,

        /* must be last */
        M2MClientResourceCount
    };

    M2MClient() : _flags(0) {
    }

    int init();

    /* handles PUT callbacks from MbedCloudClient
     *
     * this callback was registered by calling set_update_callback(this)
     * in the M2MClient constructor.  the MbedCloucClient.set_update_callback()
     * method expects *this to be a subclass of MbedCloudClientCallback,
     * which is true for M2MClient.
     */
    void value_updated(M2MBase *base, M2MBase::BaseType type);

    bool call_register(NetworkInterface *iface)
    {
        register_objects();
        bool setup = _cloud_client.setup(iface);
        _cloud_client.set_update_callback(this);
        _cloud_client.on_registered(this, &M2MClient::client_registered);
        _cloud_client.on_unregistered(this, &M2MClient::client_unregistered);
        _cloud_client.on_error(this, &M2MClient::error);
        set_flag(M2MCLIENT_F_REGISTER_CALLED);
        if (!setup) {
            printf("ERROR: m2m client setup failed\n");
            return false;
        }

        /* Set callback functions for authorizing updates and monitoring
         * progress.  Both callbacks are completely optional. If no
         * authorization callback is set, the update process will proceed
         * immediately in each step.
         * */
        _cloud_client.set_update_authorize_handler(_update_authorize_cb);
        _cloud_client.set_update_progress_handler(_update_progress_cb);
        return true;
    }

    void close() { _cloud_client.close(); }

    void keep_alive() { _cloud_client.register_update(); }

    void client_registered()
    {
        set_flag(M2MCLIENT_F_REGISTERED);
        printf("Client registered\n");
        static const ConnectorClientEndpointInfo *endpoint = NULL;
        if (endpoint == NULL) {
            endpoint = _cloud_client.endpoint_info();
            if (endpoint) {
                printf("Cloud Client: Ready\n");
                printf("Internal Endpoint Name: %s\n",
                       endpoint->internal_endpoint_name.c_str());
                printf("Endpoint Name: %s\n", endpoint->endpoint_name.c_str());
                printf("Device Id: %s\n",
                       endpoint->internal_endpoint_name.c_str());
                printf("Account Id: %s\n", endpoint->account_id.c_str());
                printf("Security Mode (-1=not set, 0=psk, 1=<undef>, 2=cert, "
                       "3=none): %d\n",
                       endpoint->mode);
            }
        }
        _on_registered_cb(_on_registered_context);
    }

    void client_unregistered()
    {
        clear_flag(M2MCLIENT_F_REGISTERED);
        clear_flag(M2MCLIENT_F_REGISTER_CALLED);
        printf("\nClient unregistered\n\n");
        _on_unregistered_cb(_on_unregistered_context);
    }

    void error(int error_code)
    {
        const char *error;
        switch (error_code) {
            case MbedCloudClient::ConnectErrorNone:
                error = "MbedCloudClient::ConnectErrorNone";
                break;
            case MbedCloudClient::ConnectAlreadyExists:
                error = "MbedCloudClient::ConnectAlreadyExists";
                break;
            case MbedCloudClient::ConnectBootstrapFailed:
                error = "MbedCloudClient::ConnectBootstrapFailed";
                break;
            case MbedCloudClient::ConnectInvalidParameters:
                error = "MbedCloudClient::ConnectInvalidParameters";
                break;
            case MbedCloudClient::ConnectNotRegistered:
                error = "MbedCloudClient::ConnectNotRegistered";
                break;
            case MbedCloudClient::ConnectTimeout:
                error = "MbedCloudClient::ConnectTimeout";
                break;
            case MbedCloudClient::ConnectNetworkError:
                error = "MbedCloudClient::ConnectNetworkError";
                break;
            case MbedCloudClient::ConnectResponseParseFailed:
                error = "MbedCloudClient::ConnectResponseParseFailed";
                break;
            case MbedCloudClient::ConnectUnknownError:
                error = "MbedCloudClient::ConnectUnknownError";
                break;
            case MbedCloudClient::ConnectMemoryConnectFail:
                error = "MbedCloudClient::ConnectMemoryConnectFail";
                break;
            case MbedCloudClient::ConnectNotAllowed:
                error = "MbedCloudClient::ConnectNotAllowed";
                break;
            case MbedCloudClient::ConnectSecureConnectionFailed:
                error = "MbedCloudClient::ConnectSecureConnectionFailed";
                break;
            case MbedCloudClient::ConnectDnsResolvingFailed:
                error = "MbedCloudClient::ConnectDnsResolvingFailed";
                break;
            case MbedCloudClient::UpdateWarningCertificateNotFound:
                error = "MbedCloudClient::UpdateWarningCertificateNotFound";
                break;
            case MbedCloudClient::UpdateWarningIdentityNotFound:
                error = "MbedCloudClient::UpdateWarningIdentityNotFound";
                break;
            case MbedCloudClient::UpdateWarningCertificateInvalid:
                error = "MbedCloudClient::UpdateWarningCertificateInvalid";
                break;
            case MbedCloudClient::UpdateWarningSignatureInvalid:
                error = "MbedCloudClient::UpdateWarningSignatureInvalid";
                break;
            case MbedCloudClient::UpdateWarningVendorMismatch:
                error = "MbedCloudClient::UpdateWarningVendorMismatch";
                break;
            case MbedCloudClient::UpdateWarningClassMismatch:
                error = "MbedCloudClient::UpdateWarningClassMismatch";
                break;
            case MbedCloudClient::UpdateWarningDeviceMismatch:
                error = "MbedCloudClient::UpdateWarningDeviceMismatch";
                break;
            case MbedCloudClient::UpdateWarningURINotFound:
                error = "MbedCloudClient::UpdateWarningURINotFound";
                break;
            case MbedCloudClient::UpdateWarningRollbackProtection:
                error = "MbedCloudClient::UpdateWarningRollbackProtection";
                break;
            case MbedCloudClient::UpdateWarningUnknown:
                error = "MbedCloudClient::UpdateWarningUnknown";
                break;
            case MbedCloudClient::UpdateErrorWriteToStorage:
                error = "MbedCloudClient::UpdateErrorWriteToStorage";
                break;
            default:
                error = "UNKNOWN";
        }
        _on_error_cb(_on_error_context, error_code, error,
                     _cloud_client.error_description());
    }

    bool is_client_registered() { return test_flag(M2MCLIENT_F_REGISTERED); }

    bool is_register_called() { return test_flag(M2MCLIENT_F_REGISTER_CALLED); }

    MbedCloudClient &get_cloud_client() { return _cloud_client; }

    void on_registered(void *context, void (*callback)(void *))
    {
        _on_registered_cb = callback;
        _on_registered_context = context;
    }

    void on_unregistered(void *context, void (*callback)(void *))
    {
        _on_unregistered_cb = callback;
        _on_unregistered_context = context;
    }

    void on_error(void *context,
                  void (*callback)(void *context, int err_code,
                                   const char *err_name, const char *err_desc))
    {
        _on_error_cb = callback;
        _on_error_context = context;
    }

    void on_update_authorize(void (*callback)(int32_t))
    {
        _update_authorize_cb = callback;
    }

    void on_update_progress(void (*callback)(uint32_t, uint32_t))
    {
        _update_progress_cb = callback;
    }

    void on_resource_updated(void *context,
                             void (*callback)(
                                    void *context,
                                    M2MClient::M2MClientResource resource))
    {
        _on_resource_updated_cb = callback;
        _on_resource_updated_context = context;
    }

    void update_authorize(int32_t request)
    {
        switch (request) {
            case MbedCloudClient::UpdateRequestDownload:
                set_flag(M2MCLIENT_F_FOTA_DL_GRANTED);
                break;
            case MbedCloudClient::UpdateRequestInstall:
                set_flag(M2MCLIENT_F_FOTA_INSTALL_GRANTED);
                break;
            default:
                break;
        }
        _cloud_client.update_authorize(request);
    }

    /* retrieves a resource object tracked by the M2MClient class */
    M2MResource *get_resource(const char *uri_path);
    M2MResource *get_resource(enum M2MClientResource resource);

    std::string get_resource_value_str(enum M2MClientResource resource);
    std::string get_resource_value_str(M2MResource *res);

    void set_resource_value(M2MResource *res, const char *val, size_t len);

    void set_resource_value(enum M2MClientResource resource,
                            const char *val, size_t len);

    void set_resource_value(enum M2MClientResource resource,
                            const std::string &val);

    void set_fota_download_requested();
    bool is_fota_download_requested();

    void set_fota_install_requested();
    bool is_fota_install_requested();

private:
    struct resource_entry {
        M2MResource *res;
        enum M2MClientResource type;
    };

    /* our objects */
    std::map<std::string, struct resource_entry> _res_map;

    MbedCloudClient _cloud_client;

    int _flags;

    void set_flag(int flag)
    {
        _flags |= flag;
    }

    void clear_flag(int flag)
    {
        _flags &= ~flag;
    }

    int test_flag(int flag)
    {
        return ((_flags & flag) == flag);
    }

    void (*_on_registered_cb)(void *context);
    void *_on_registered_context;

    void (*_on_unregistered_cb)(void *context);
    void *_on_unregistered_context;

    void (*_on_error_cb)(void *context, int, const char *, const char *);
    void *_on_error_context;

    void (*_update_authorize_cb)(int32_t);
    void (*_update_progress_cb)(uint32_t, uint32_t);

    void (*_on_resource_updated_cb)(void *context,
                                    M2MClient::M2MClientResource resource);
    void *_on_resource_updated_context;

    /* adds a resource to the internal object map */
    void add_resource(M2MResource *res, enum M2MClientResource type);

    /* adds the M2M app resources to the internal object map */
    int add_app_resources();

    /* adds the M2M Geo and GeoSensor resources to the internal object map */
    int add_geo_resources();

    /* resource adders for each supported sensor type */
    int add_light_sensor();
    int add_temp_sensor();
    int add_humidity_sensor();
    int add_network_sensor();

    /* registers all objects with the underlying MbedCloudClient */
    void register_objects();

    struct resource_entry *get_resource_entry(const char *uri_path);
    struct resource_entry *get_resource_entry(enum M2MClientResource type);
};

#endif /* M2MCLIENT_H */
