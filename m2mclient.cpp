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

#include "m2mclient.h"

int M2MClient::init()
{
    int ret;

    ret = add_app_resources();
    if (0 != ret) {
        return ret;
    }

    ret = add_light_sensor();
    if (0 != ret) {
        return ret;
    }

    ret = add_temp_sensor();
    if (0 != ret) {
        return ret;
    }

    ret = add_humidity_sensor();
    if (0 != ret) {
        return ret;
    }

    ret = add_network_sensor();
    if (0 != ret) {
        return ret;
    }

    ret = add_geo_resources();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

std::string M2MClient::get_resource_value_str(M2MResource *res)
{
    return res->get_value_string().c_str();
}

std::string M2MClient::get_resource_value_str(enum M2MClientResource resource)
{
    M2MResource *res;

    res = get_resource(resource);
    if (NULL == res) {
        return "";
    }

    return get_resource_value_str(res);
}

void M2MClient::set_resource_value(M2MResource *res,
                                   const char *val,
                                   size_t len)
{
    res->set_value((const uint8_t *)val, len);
}

void M2MClient::set_resource_value(enum M2MClientResource resource,
                                   const char *val,
                                   size_t len)
{
    M2MResource *res;

    res = get_resource(resource);
    if (NULL == res) {
        return;
    }

    set_resource_value(res, val, len);
}

void M2MClient::set_resource_value(enum M2MClientResource resource,
                                   const std::string &val)
{
    M2MResource *res;

    res = get_resource(resource);
    if (NULL == res) {
        return;
    }

    set_resource_value(res, val.c_str(), val.length());
}

void M2MClient::register_objects()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectList obj_list;
    std::map<std::string, struct resource_entry>::iterator it;

    for (it = _res_map.begin(); it != _res_map.end(); ++it) {
        res = it->second.res;
        obj = &res->get_parent_object_instance().get_parent_object();
        obj_list.push_back(obj);
    }

    _cloud_client.add_objects(obj_list);
}

void M2MClient::add_resource(M2MResource *res, enum M2MClientResource type)
{
    struct resource_entry entry = {res, type};
    _res_map[res->uri_path()] = entry;
}

#ifdef DEBUG
void print_object_list(M2MObjectList &obj_list)
{
    M2MObjectList::const_iterator obj_it;
    M2MObjectInstanceList::const_iterator obj_inst_it;
    M2MResourceList::const_iterator res_it;
    M2MResourceInstanceList::const_iterator res_inst_it;

    for (obj_it = obj_list.begin(); obj_it != obj_list.end(); obj_it++) {

        M2MObject *obj = (*obj_it);
        printf("object path: %s\n", obj->uri_path());
        for (obj_inst_it = obj->instances().begin();
             obj_inst_it != obj->instances().end();
             obj_inst_it++) {

            M2MObjectInstance *obj_inst = (*obj_inst_it);
            printf("object instance path: %s\n", obj_inst->uri_path());
            for (res_it = obj_inst->resources().begin();
                 res_it != obj_inst->resources().end();
                 res_it++) {

                 M2MResource *res = (*res_it);
                 printf("resource path: %s\n", res->uri_path());
                 for (res_inst_it = res->resource_instances().begin();
                      res_inst_it != res->resource_instances().end();
                      res_inst_it++) {

                      M2MResourceInstance *res_inst = (*res_inst_it);
                      printf("resource instance path: %s\n",
                             res_inst->uri_path());
                  }
             }
         }
     }
}
#endif

struct M2MClient::resource_entry *
M2MClient::get_resource_entry(const char *uri_path)
{
    std::map<std::string, struct resource_entry>::iterator it;

    it = _res_map.find(uri_path);
    if (it == _res_map.end()) {
        return NULL;
    }

    return &it->second;
}

M2MResource *M2MClient::get_resource(const char *uri_path)
{
    struct resource_entry *entry;

    entry = get_resource_entry(uri_path);
    if (NULL == entry) {
        return NULL;
    }
    return entry->res;
}

struct M2MClient::resource_entry *
M2MClient::get_resource_entry(enum M2MClientResource type)
{
    struct resource_entry *entry;
    std::map<std::string, struct resource_entry>::iterator it;

    if (type >= M2MClientResourceCount) {
        return NULL;
    }

    entry = NULL;
    for (it = _res_map.begin(); it != _res_map.end(); ++it) {
        entry = &it->second;
        if (type == entry->type) {
            break;
        }
        entry = NULL;
    }
    return entry;
}

M2MResource *M2MClient::get_resource(enum M2MClientResource resource)
{
    struct resource_entry *entry;

    entry = get_resource_entry(resource);
    if (NULL == entry) {
        return NULL;
    }
    return entry->res;
}

int M2MClient::add_light_sensor()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    obj = M2MInterfaceFactory::create_object("3301");
    inst = obj->create_object_instance();
    res = inst->create_dynamic_resource("5700", "light_value",
                                        M2MResourceInstance::FLOAT,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    add_resource(res, M2MClientResourceLightValue);

    return 0;
}

int M2MClient::add_temp_sensor()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    obj = M2MInterfaceFactory::create_object("3303");
    inst = obj->create_object_instance();
    res = inst->create_dynamic_resource("5700", "temperature_value",
                                        M2MResourceInstance::FLOAT,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    add_resource(res, M2MClientResourceTempValue);

    return 0;
}

int M2MClient::add_humidity_sensor()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    obj = M2MInterfaceFactory::create_object("3304");
    inst = obj->create_object_instance();
    res = inst->create_dynamic_resource("5700", "humidity_value",
                                        M2MResourceInstance::FLOAT,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    add_resource(res, M2MClientResourceHumidityValue);

    return 0;
}

int M2MClient::add_network_sensor()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    /*
     * create an instance of the top level object for wem custom
     * app resources
     * */
    obj = M2MInterfaceFactory::create_object("26242");
    inst = obj->create_object_instance();

    /*
     * attach resources to the App object instance
     */
    res = inst->create_dynamic_resource("1", "network_resource",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    add_resource(res, M2MClientResourceNetwork);

    return 0;
}

void M2MClient::value_updated(M2MBase *base, M2MBase::BaseType type)
{
    struct resource_entry *entry;

    if (NULL == base || NULL == base->uri_path()) {
        return;
    }

    entry = get_resource_entry(base->uri_path());
    if (NULL == entry) {
        printf("WARN: PUT called on unknown uri_path=%s\n", base->uri_path());
        return;
    }

    _on_resource_updated_cb(_on_resource_updated_context, entry->type);
}

int M2MClient::add_geo_resources()
{
    const char *type;
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    /* one object will hold both geo resource types */
    obj = M2MInterfaceFactory::create_object("3336");

    /*
     * add the user-specified geo resource
     * */
    inst = obj->create_object_instance((uint16_t)0);
    res = inst->create_dynamic_resource("5514", "Latitude",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_PUT_ALLOWED);
    add_resource(res, M2MClientResourceGeoLat);
    res = NULL;

    res = inst->create_dynamic_resource("5515", "Longitude",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_PUT_ALLOWED);
    add_resource(res, M2MClientResourceGeoLong);
    res = NULL;

    res = inst->create_dynamic_resource("5516", "Uncertainty",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_PUT_ALLOWED);
    add_resource(res, M2MClientResourceGeoAccuracy);
    res = NULL;

    res = inst->create_dynamic_resource("5750", "Application_Type",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    type = "user";
    set_resource_value(res, type, strlen(type));
    add_resource(res, M2MClientResourceGeoType);
    res = NULL;

    /*
     * add the dynamic geo resource
     * */
    inst = obj->create_object_instance((uint16_t)1);
    res = inst->create_dynamic_resource("5514", "Latitude",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_PUT_ALLOWED);
    add_resource(res, M2MClientResourceAutoGeoLat);
    res = NULL;

    res = inst->create_dynamic_resource("5515", "Longitude",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_PUT_ALLOWED);
    add_resource(res, M2MClientResourceAutoGeoLong);
    res = NULL;

    res = inst->create_dynamic_resource("5516", "Uncertainty",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_PUT_ALLOWED);
    add_resource(res, M2MClientResourceAutoGeoAccuracy);
    res = NULL;

    res = inst->create_dynamic_resource("5750", "Application_Type",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    type = "auto";
    set_resource_value(res, type, strlen(type));
    add_resource(res, M2MClientResourceAutoGeoType);
    res = NULL;

    return 0;
}

int M2MClient::add_app_resources()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    /*
     * create an instance of the top level object for wem custom
     * app resources
     * */
    obj = M2MInterfaceFactory::create_object("26241");
    inst = obj->create_object_instance();

    /*
     * attach resources to the App object instance
     * */

    /* add the user-friendly app label */
    res = inst->create_dynamic_resource("1",
                                        "Label",
                                        M2MResourceInstance::STRING,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_PUT_ALLOWED);
    add_resource(res, M2MClientResourceAppLabel);
    res = NULL;

    /* add the app version */
    res = inst->create_dynamic_resource("2",
                                        "Version",
                                        M2MResourceInstance::STRING,
                                        false /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    res->set_value((const uint8_t *)MBED_CONF_APP_VERSION,
                   strlen(MBED_CONF_APP_VERSION));
    add_resource(res, M2MClientResourceAppVersion);
    res = NULL;

    return 0;
}

void M2MClient::set_fota_download_requested()
{
    set_flag(M2MCLIENT_F_FOTA_DL_REQD);
}

bool M2MClient::is_fota_download_requested()
{
    return test_flag(M2MCLIENT_F_FOTA_DL_REQD);
}

void M2MClient::set_fota_install_requested()
{
    set_flag(M2MCLIENT_F_FOTA_INSTALL_REQD);
}

bool M2MClient::is_fota_install_requested()
{
    return test_flag(M2MCLIENT_F_FOTA_INSTALL_REQD);
}
