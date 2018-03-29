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

#include "fs.h"
#include "compat.h"

#include <SPIFBlockDevice.h>

#include <errno.h>
#include <stdio.h>

SPIFBlockDevice bd(MBED_CONF_APP_SPI_MOSI,
                   MBED_CONF_APP_SPI_MISO,
                   MBED_CONF_APP_SPI_CLK,
                   MBED_CONF_APP_SPI_CS);
FATFileSystem fs(FS_NAME);

int fs_init()
{
    int ret;

    /* init the underlying block device */
    ret = bd.init();
    if (ret != BD_ERROR_OK) {
        printf("bd.init failed\n");
        return ret;
    }

    /* mount the filesystem */
    ret = fs_mount();
    if (0 != ret) {
        printf("fs.mount failed\n");
        return ret;
    }

    return 0;
}

void fs_shutdown()
{
    bd.deinit();
}

int fs_remove(std::string &path)
{
    std::string real;

    real = FS_MOUNT_POINT + path;

    return remove(real.c_str());
}

int fs_cat(std::string &path)
{
    FILE *fp;
    char buff[16];
    std::string real;

    real = FS_MOUNT_POINT + path;

    fp = fopen(real.c_str(), "r");
    if (NULL == fp) {
        printf("failed to open %s: %d\n", path.c_str(), errno);
        return -errno;
    }

    while (!feof(fp)){
        int size = fread(buff, 1, sizeof(buff) - 1, fp);
        fwrite(buff, 1, size, stdout);
    }

    fclose(fp);

    return 0;
}

int fs_ls(std::string &path)
{
    DIR *dir;
    struct dirent *dp;
    std::string real;

    real = FS_MOUNT_POINT + path;

    if ( (dir = opendir(real.c_str())) == NULL) {
        printf("ERROR: failed to open dir %s: %d\n", path.c_str(), errno);
        return -errno;
    }

    while ( (dp = readdir(dir)) != NULL) {
        printf("%s\n", dp->d_name);
    }

    closedir(dir);

    return 0;
}

int fs_mkdir(std::string &path)
{
    int ret;
    std::string real;

    real = FS_MOUNT_POINT + path;

    ret = ::mkdir(real.c_str(), 0777);
    if (0 != ret) {
        printf("failed mkdir %s: %d\n", path.c_str(), errno);
        return -errno;
    }

    return 0;
}

int fs_format()
{
    return fs.format(&bd);
}

int fs_mount()
{
    return fs.mount(&bd);
}

int fs_unmount()
{
    return fs.unmount();
}

int fs_test()
{
    int error;

    printf("Opening a new file, numbers.txt.");
    FILE* fd = fopen(FS_MOUNT_POINT "/numbers.txt", "w");
    if (NULL == fd) {
        printf(" Failure. %d\n", errno);
    } else {
        printf(" done.\n");
    }

    for (int i = 0; i < 20; i++){
        printf("Writing decimal numbers to a file (%d/20)\r", i);
        fprintf(fd, "%d\r\n", i);
    }
    printf("Writing decimal numbers to a file (20/20) done.\r\n");

    printf("Closing file.");
    fclose(fd);
    printf(" done.\r\n");

    printf("Re-opening file read-only.");
    fd = fopen(FS_MOUNT_POINT "/numbers.txt", "r");
    if (NULL == fd) {
        printf(" Failure. %d\n", errno);
    } else {
        printf(" done.\n");
    }

    printf("Dumping file to screen.\r\n");
    char buff[16] = {0};
    while (!feof(fd)){
        int size = fread(&buff[0], 1, 15, fd);
        fwrite(&buff[0], 1, size, stdout);
    }
    printf("EOF.\r\n");

    printf("Closing file.");
    fclose(fd);
    printf(" done.\r\n");

    printf("Opening root directory.");
    DIR* dir = opendir(FS_MOUNT_POINT);
    if (NULL == fd) {
        printf(" Failure. %d\n", errno);
    } else {
        printf(" done.\n");
    }

    struct dirent* de;
    printf("Printing all filenames:\r\n");
    while((de = readdir(dir)) != NULL){
        printf("  %s\r\n", &(de->d_name)[0]);
    }

    printf("Closeing root directory. ");
    error = closedir(dir);
    if (error) {
        printf("Failure. %d\n", error);
    } else {
        printf("done.\n");
    }
    printf("Filesystem Demo complete.\r\n");

    return 0;
}
