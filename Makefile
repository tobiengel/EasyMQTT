CONTIKI_PROJECT = EasyMQTT

all: $(CONTIKI_PROJECT)

TARGET = simplelink
BOARD = sensortag/cc1352r
CFLAGS += -g -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-function

CONTIKI = ${CONTIKI_ROOT}

include $(CONTIKI)/Makefile.dir-variables

MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/mqtt
MODULES += $(CONTIKI_NG_STORAGE_DIR)/cfs
MODULES_REL += arch/platform/$(TARGET)

PROJECT_SOURCEFILES += mqttWrapper.c storage.c CommandHandler.c utilities.c

-include $(CONTIKI)/Makefile.identify-target

PLATFORMS_ONLY = simplelink

include $(CONTIKI)/Makefile.include
EasyMQTT.$(TARGET):  $(OBJECTDIR)/mqttWrapper.o $(OBJECTDIR)/storage.o $(OBJECTDIR)/CommandHandler.o


