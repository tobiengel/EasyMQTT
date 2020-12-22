/*
 * CommandHandler.c
 *
 *  Created on: 22 Dec 2020
 *      Author: worluk
 */


#include "CommandHandler.h"
#include <contiki.h>

extern struct etimer resetLED_timer;
static uint8_t maxCommand = 0;

standard_retval addCommand(char* c, CmdHandler handler){
    if((maxCommand + 1) == MAX_COMMANDS)
        return CALL_FAILED;

    if(handler == NULL)
        return CALL_FAILED;

    maxCommand++;

    commands[maxCommand].command = c;
    commands[maxCommand].handler = handler;
    return CALL_OK;
}


standard_retval handleCommand(char* topic, char* data){

    char* cmd;
    standard_retval r = CALL_FAILED;
    for(uint8_t i = 0; i < MAX_COMMANDS; i++){
        cmd = commands[i].command;
        if(strncmp(data, cmd, sizeof(cmd)) == 0){
            r = commands[i].handler(topic, data);
            if(r == CALL_OK){
            etimer_set(&resetLED_timer, (CLOCK_SECOND * 1));
            leds_on(LEDS_BLUE);
            }
        }
    }
    return CALL_OK;
}