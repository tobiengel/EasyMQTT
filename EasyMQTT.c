#include <contiki.h>        //dont forget to include here,

#include <board.h>          //Board defines (IOs etc)
#include <dev/button-hal.h> //Allows to use the button events

#include <os/lib/sensors.h> //include the sensor interface
#include <batmon-sensor.h>  //include the batmon specific sensor implementation
#include <ICM20948.h>
#include <bme-680-sensor.h>

#include <mqttWrapper.h>
#include <storage.h>
#include <EasyMQTT.h>
#include "utilities.h"

#include <posix/unistd.h>   //usleep

#define BUFFER_SIZE 64

#include "configurationData.h"

/*---------------------------------------------------------------------------*/
/* Logging facilities */
#include <os/sys/log.h>
#define LOG_LEVEL LOG_LEVEL_NONE
#define LOG_MODULE "EasyMQTT"

/*---------------------------------------------------------------------------*/

AUTOSTART_PROCESSES(&easymqtt_process);
PROCESS(easymqtt_process, "EasyMQTT Process");
/*---------------------------------------------------------------------------*/

static int batval = 0;

static struct etimer batmon_timer;
       struct etimer resetLED_timer;
static struct etimer data_send_timer;

static char send_buffer[BUFFER_SIZE];

float getSensorData(int t){
    return bme_680_sensor.value(t) / 1000;
}

void roundSensorDatatoString(int t, char* b){
    ftoa(getSensorData(t), b, 2);
}


void sendSensorData(){
    char buf[10];

    roundSensorDatatoString(BME_680_SENSOR_TYPE_TEMP, buf);
    publishStr("temperature",buf);

    roundSensorDatatoString(BME_680_SENSOR_TYPE_PRESS, buf);
    publishStr("pressure",buf);

    roundSensorDatatoString(BME_680_SENSOR_TYPE_HUM, buf);
    publishStr("humidity",buf);

    roundSensorDatatoString(BME_680_SENSOR_TYPE_IAQ, buf);
    publishStr("iaq",buf);

    sprintf(buf, "%d", bme_680_sensor.value(BME_680_SENSOR_TYPE_IAQ_ACC));
    publishStr("accuracy",buf);

    roundSensorDatatoString(BME_680_SENSOR_TYPE_CO2, buf);
    publishStr("co2",buf);


    sprintf(buf, "%d", batval);
    publishStr("battery",buf);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(easymqtt_process, ev, data) {

    standard_retval r = CALL_OK;

    PROCESS_BEGIN();

    initStorage();
    initMQTT(&easymqtt_process, NULL);

    etimer_set(&data_send_timer,  (CLOCK_SECOND * NonVolatileConfig.datasendInterval));     //set timer for cyclic data sending
   // etimer_set(&batmon_timer,  (CLOCK_SECOND * NonVolatileConfig.batterySendInterval));     //set timer for battery value sending
    etimer_set(&resetLED_timer,  (CLOCK_SECOND * 5));                                       //set timer to shut down LEDs, runs once in the beginning to ensure LEDs are off

    SENSORS_ACTIVATE(batmon_sensor);   //activate battery monitor
    SENSORS_ACTIVATE(bme_680_sensor);
    batval = (batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT) * 125) >> 5;

    /* Main loop */
    while(1) {

      PROCESS_YIELD();

      if((ev == button_hal_press_event  && ((button_hal_button_t *)data)->unique_id == BUTTON_HAL_ID_KEY_LEFT)) {
          if(isConnectionReady()) {
              sendSensorData();
          }
      }

      if((ev == button_hal_press_event  && ((button_hal_button_t *)data)->unique_id == BUTTON_HAL_ID_KEY_RIGHT)) {
          if(isConnectionReady()) {
            }
      }

      if(ev == PROCESS_EVENT_TIMER && data == &data_send_timer ) {
          LOG_DBG("Data send timer\n");
          if(isConnectionReady()) {

              sendSensorData();
              batval = (batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT) * 125) >> 5;
              if(batval < 1900)
                publishStr("bat", "low");
          }
          /* restart the data send timer with configured interval */
          etimer_set(&data_send_timer, (CLOCK_SECOND * NonVolatileConfig.datasendInterval));
      }

//      if(ev == PROCESS_EVENT_TIMER && data == &batmon_timer) {
//
//         batval = (batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT) * 125) >> 5;
//         etimer_set(&batmon_timer,  (CLOCK_SECOND * NonVolatileConfig.batterySendInterval));
//      }

      MQTTTasks(ev, data);

    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
