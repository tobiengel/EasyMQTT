/*
 * Storage.h
 *
 *  Created on: 17 Dec 2020
 *      Author: worluk
 */

#ifndef STORAGE_STORAGE_H_
#define STORAGE_STORAGE_H_

#include <returnTypes.h>
#include "configurationData.h"

extern configData NonVolatileConfig;

void listFiles();
standard_retval syncConfig();
standard_retval initStorage();
standard_retval formatStorage();
#endif /* STORAGE_STORAGE_H_ */
