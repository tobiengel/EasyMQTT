/*
 * CommandHandler.h
 *
 *  Created on: 22 Dec 2020
 *      Author: worluk
 */

#ifndef COMMANDHANDLER_H_
#define COMMANDHANDLER_H_

#include "returnTypes.h"

#define MAX_COMMAND_SIZE 12 //defines how many characters a command can have
#define MAX_COMMANDS 5 //defines how many commands are checked
typedef standard_retval (*CmdHandler)(char*, char*);

typedef struct ConfigCommandHandler {
    char command[MAX_COMMAND_SIZE];
    CmdHandler handler;
} ConfigCommand;

ConfigCommandHandler commands[MAX_COMMANDS];


standard_retval addCommand(char*, CmdHandler);
standard_retval handleCommand(char*, char*);

#endif /* COMMANDHANDLER_H_ */
