#include <contiki.h>        //dont forget to include here,

#include <board.h>          //Board defines (IOs etc)
#include <dev/button-hal.h> //Allows to use the button events

#include <os/lib/sensors.h> //include the sensor interface
#include <batmon-sensor.h>  //include the batmon specific sensor implementation

#include <mqttWrapper.h>
#include <storage.h>
#include <EasyMQTT.h>

#include <posix/unistd.h>   //usleep

#define BUFFER_SIZE 64

#include "configurationData.h"

/*---------------------------------------------------------------------------*/
/* Logging facilities */
#include <os/sys/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
#define LOG_MODULE "EasyMQTT"

/*---------------------------------------------------------------------------*/

AUTOSTART_PROCESSES(&easymqtt_process);
PROCESS(easymqtt_process, "EasyMQTT Process");
/*---------------------------------------------------------------------------*/



static struct etimer batmon_timer;
       struct etimer resetLED_timer;
static struct etimer data_send_timer;

static char send_buffer[BUFFER_SIZE];


void ReceiveData(char* topic, char* data){


}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(easymqtt_process, ev, data) {

    standard_retval r = CALL_OK;

    PROCESS_BEGIN();

    //sleep(2);       //wait for flash to become ready
    initStorage();
    initMQTT(&easymqtt_process, &ReceiveData); //initialize MQTT and pass callback to handle received data

    etimer_set(&data_send_timer,  (CLOCK_SECOND * NonVolatileConfig.datasendInterval));     //set timer for cyclic data sending
    etimer_set(&batmon_timer,  (CLOCK_SECOND * NonVolatileConfig.batterySendInterval));     //set timer for battery value sending
    etimer_set(&resetLED_timer,  (CLOCK_SECOND * 5));                                       //set timer to shut down LEDs

    SENSORS_ACTIVATE(batmon_sensor);                                                        //activate battery monitor

    /* Main loop */
    while(1) {

      PROCESS_YIELD();

      if((ev == button_hal_press_event  && ((button_hal_button_t *)data)->unique_id == BUTTON_HAL_ID_KEY_LEFT)) {
          publishStr("test", "Hello");
      }

      if((ev == button_hal_press_event  && ((button_hal_button_t *)data)->unique_id == BUTTON_HAL_ID_KEY_RIGHT)) {

      }

      if(ev == PROCESS_EVENT_TIMER && data == &data_send_timer ) {
          if(isConnectionReady()) {

          }
          /* restart the data send timer with configured interval */
          etimer_set(&data_send_timer, (CLOCK_SECOND * NonVolatileConfig.datasendInterval));
      }

      if(ev == PROCESS_EVENT_TIMER && data == &batmon_timer) {
          if(isConnectionReady()) {
              int value = batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT);
              value = (value * 125) >> 5;
              LOG_DBG("Bat: Volt=%d mV\n", value);

              sprintf(send_buffer, "%d", value);
              publishStr("battery", send_buffer);

              if(value < 1900)
                  publishStr("bat", "low");
         }
         etimer_set(&batmon_timer,  (CLOCK_SECOND * NonVolatileConfig.batterySendInterval));
      }

      MQTTTasks(ev, data);

    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
