/*
 * Storage.c
 *
 *  Created on: 17 Dec 2020
 *      Author: worluk
 */


#include "storage.h"

#include <cfs/cfs.h>
#include <cfs/cfs-coffee.h>

#include <os/sys/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
#define LOG_MODULE "Storage"

configData NonVolatileConfig;

void listFiles(){
    struct cfs_dir dir;
    struct cfs_dirent dirent;

    if(cfs_opendir(&dir, "/") == 0) {
      while(cfs_readdir(&dir, &dirent) != -1) {
        LOG_DBG("File: %s (%ld bytes)\n", dirent.name, (long)dirent.size);
      }
      cfs_closedir(&dir);
    }
}

standard_retval createDefaultConfig(){

    sprintf(NonVolatileConfig.channelBase, "default/%%s");
    NonVolatileConfig.datasendInterval = 600;
    NonVolatileConfig.batterySendInterval = 3600;

    return CALL_OK;
}


static void EnsureConfig(){

    memset(&NonVolatileConfig, 0, sizeof(NonVolatileConfig));

    int hd = cfs_open(configDataFile, CFS_READ);
    if(hd < 0){ //couldnt open config file
        LOG_DBG("No config file. Creating\n");
        cfs_close(hd);
        LOG_DBG("Formatting cfs\n");
        if(cfs_coffee_format() == CALL_OK)
            LOG_DBG("Sucessful\n");
        else
            LOG_DBG("Failed\n");
        createDefaultConfig();
        syncConfig();

    } else {
        int read = cfs_read(hd, &NonVolatileConfig, sizeof(NonVolatileConfig));
        if(read != sizeof(NonVolatileConfig)){
            cfs_close(hd);
            printf("Formatting cfs\n");
            if(cfs_coffee_format() == CALL_OK)
                printf("Sucessful\n");
            else
                printf("Failed\n");
            printf("Config struct changed\n");
            createDefaultConfig();
            syncConfig();
        } else { //here we now have a NonVolatileConfig struct

        }
    }
    cfs_close(hd);

}

standard_retval initStorage(){

    #ifdef CFS_FORMAT
        if(cfs_coffee_format() == CALL_OK)
            LOG_DBG("Sucessful\n");
        else
            LOG_DBG("Failed\n");

    #endif

   listFiles();
   EnsureConfig();


    //ensure channel config is correct
    uint8_t len = strlen(NonVolatileConfig.channelBase);
    if(strcmp(&NonVolatileConfig.channelBase[len - 3], "/%s") != 0){
        sprintf(NonVolatileConfig.channelBase, "default/%%s");
    }



    return CALL_OK;
}

standard_retval syncConfig(){
    int len = sizeof(NonVolatileConfig);
    int hd = cfs_open(configDataFile, CFS_WRITE);
    int n = cfs_write(hd, &NonVolatileConfig, len);

    cfs_close(hd);
    if(n == len)
            return CALL_OK;

    return CALL_FAILED;
}


