/*
 * CommandHandler.c
 *
 *  Created on: 22 Dec 2020
 *      Author: worluk
 */


#include "CommandHandler.h"
#include <contiki.h>
#include <board.h>
#include <string.h>
#include <os/dev/leds.h>

ConfigCommand commands[MAX_COMMANDS];

extern struct etimer resetLED_timer;
static uint8_t maxCommand = 0;

standard_retval addCommand(char* c, CmdHandler handler){
    if((maxCommand + 1) == MAX_COMMANDS)
        return CALL_FAILED;

    if(handler == NULL)
        return CALL_FAILED;

    maxCommand++;

    strncpy(commands[maxCommand].command,c, strlen(c));
    commands[maxCommand].handler = handler;
    return CALL_OK;
}


standard_retval handleCommand(char* topic, char* data){

    char* cmd;
    CmdHandler h = NULL;
    standard_retval r = CALL_FAILED;
    for(uint8_t i = 0; i < MAX_COMMANDS; i++){
        cmd = commands[i].command;

        if(strncmp(data, cmd, sizeof(cmd)) == 0){
            h = commands[i].handler;
            if(h == NULL)
                return CALL_FAILED;
            r = h(topic, data);
            if(r == CALL_OK){
                etimer_set(&resetLED_timer, (CLOCK_SECOND * 1));
                leds_on(LEDS_BLUE);
            }
            break;
        }
    }
    return CALL_OK;
}
