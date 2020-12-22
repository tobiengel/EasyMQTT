/*
 * MQTTImplementation.h
 *
 *  Created on: 17 Dec 2020
 *      Author: worluk
 */

#ifndef MQTTIMPLEMENTATION_H_
#define MQTTIMPLEMENTATION_H_

#include <contiki.h>
#include <returnTypes.h>
#include <mqtt.h>

extern process_event_t mqtt_command_event;

#define BUFFER_SIZE 64


typedef enum ConnectionStatus_e {
    ConnectionPending,
    ConnectionReady
} ConnectionStatus;


typedef void (*RcvHandler)(char*, char*);

standard_retval initMQTT(struct process *process, RcvHandler handler);
standard_retval construct_client_id(void);
ConnectionStatus isConnectionReady();

standard_retval MQTTTasks(process_event_t ev,  process_data_t data);
void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data);

standard_retval publishStr(char* topic, char* msg);
void subscribe(void);
void unsubscribe(void);


/*---------------------------------------------------------------------------*/
#define CONFIG_ORG_ID_LEN        32
#define CONFIG_TYPE_ID_LEN       32
#define CONFIG_AUTH_TOKEN_LEN    32
#define CONFIG_EVENT_TYPE_ID_LEN 32
#define CONFIG_CMD_TYPE_LEN       8
#define CONFIG_IP_ADDR_STR_LEN   64

/*---------------------------------------------------------------------------*/

#define STATE_INIT            0
#define STATE_REGISTERED      1
#define STATE_CONNECTING      2
#define STATE_CONNECTED       3
#define STATE_PUBLISHING      4
#define STATE_DISCONNECTED    5
#define STATE_NEWCONFIG       6
#define STATE_CONFIG_ERROR 0xFE
#define STATE_ERROR        0xFF

/*---------------------------------------------------------------------------*/
/* Maximum TCP segment size for outgoing segments of our socket */
#define MAX_TCP_SEGMENT_SIZE    32
/**
 * \brief Data structure declaration for the MQTT client configuration
 */
typedef struct mqtt_client_config {
  char org_id[CONFIG_ORG_ID_LEN];
  char type_id[CONFIG_TYPE_ID_LEN];
  char auth_token[CONFIG_AUTH_TOKEN_LEN];
  char event_type_id[CONFIG_EVENT_TYPE_ID_LEN];
  char broker_ip[CONFIG_IP_ADDR_STR_LEN];
  char cmd_type[CONFIG_CMD_TYPE_LEN];
  clock_time_t pub_interval;
  int def_rt_ping_interval;
  uint16_t broker_port;
} mqtt_client_config_t;




/*---------------------------------------------------------------------------*/
/* A timeout used when waiting to connect to a network */
#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)

/*---------------------------------------------------------------------------*/
/* Default configuration values */
#define DEFAULT_RSSI_MEAS_INTERVAL  (CLOCK_SECOND * 300)
#define DEFAULT_PUBLISH_INTERVAL    (30 * CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
/* Payload length of ICMPv6 echo requests used to measure RSSI with def rt */
#define ECHO_REQ_PAYLOAD_LEN   20

/*---------------------------------------------------------------------------*/
/*
 * A timeout used when waiting for something to happen (e.g. to connect or to
 * disconnect)
 */
#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)

/*---------------------------------------------------------------------------*/
/* Connections and reconnections */
#define RETRY_FOREVER              0xFF
#define RECONNECT_INTERVAL         (CLOCK_SECOND * 2)

/*
 * Number of times to try reconnecting to the broker.
 * Can be a limited number (e.g. 3, 10 etc) or can be set to RETRY_FOREVER
 */
#define RECONNECT_ATTEMPTS         RETRY_FOREVER
#define CONNECTION_STABLE_TIME     (CLOCK_SECOND * 5)

#endif /* MQTTIMPLEMENTATION_H_ */
