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

#include <string>
#include <FATFileSystem.h>

#define FS_NAME "sd"
#define FS_MOUNT_POINT "/" FS_NAME

int fs_init();
int fs_test();
void fs_shutdown();

int fs_ls(std::string &path);
int fs_cat(std::string &path);
int fs_mkdir(std::string &path);
int fs_remove(std::string &path);
int fs_format();
int fs_mount();
int fs_unmount();
