/*
 * configurationData.h
 *
 *  Created on: 24 Nov 2020
 *      Author: worluk
 */

#ifndef CONFIGURATIONDATA_H_
#define CONFIGURATIONDATA_H_


#define configDataFile "config"
#define configBatInterval 30
#define configDataSendInterval 30


typedef struct {
    char channelBase[32];
    int batterySendInterval;
    int datasendInterval;
} configData;


#endif /* CONFIGURATIONDATA_H_ */
