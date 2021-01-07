/*
 * MQTTImplementation.c
 *
 *  Created on: 17 Dec 2020
 *      Author: worluk
 */


#include <contiki.h>
#include <string.h>                 //memcpy
#include <net/routing/routing.h>

#include <net/ipv6/uip.h>           //uip_ip6addr_cmp
#include <net/ipv6/uip-icmp6.h>     //ping
#include <net/ipv6/sicslowpan.h>    //rssi

#include <sys/etimer.h>

#include "mqttWrapper.h"
#include "storage.h"     //needs to be aware of the config structure
#include "mqttConfig.h"             //default defines
#include "CommandHandler.h"     //handles received commands

#include <os/sys/log.h>
#define LOG_LEVEL LOG_LEVEL_NONE
#define LOG_MODULE "mqttWrapper"

/* Parent RSSI functionality */
static int def_rt_rssi = 0;
static struct uip_icmp6_echo_reply_notification echo_reply_notification;

static struct etimer echo_request_timer;

mqtt_client_config_t conf;

process_event_t mqtt_command_event; //
static char* mqtt_recv_command[32]; //
static struct mqtt_message *msg_ptr = 0;


static struct timer connection_life;
static uint8_t connect_attempt;

static const char *broker_ip = MQTT_CLIENT_CONF_BROKER_IP_ADDR;

/* Various states */
static uint8_t state;
static struct mqtt_connection conn;

static char topic_buffer[32];

static RcvHandler rcvHandler;

struct process *main_process;

extern struct etimer resetLED_timer;
struct etimer publish_periodic_timer;

char client_id[BUFFER_SIZE];

standard_retval handleHelpCmd(char* t, char* d){
    char buf[MAX_COMMAND_SIZE * MAX_COMMANDS];

    for(int i = 0; i < MAX_COMMANDS - 1; i++){
        if(commands[i].command != NULL) {
            strncat(buf, commands[i].command, sizeof(commands[i].command));
            strncat(buf, " - ", 3);
        }
    }
    if(commands[MAX_COMMANDS - 1].command != NULL) {
        strncat(buf, commands[MAX_COMMANDS - 1].command, sizeof(commands[MAX_COMMANDS - 1].command));
    }


    publishStr("default/help", buf);
    return CALL_OK;
}

standard_retval handleLocateCmd(char* t, char* d){

    etimer_set(&resetLED_timer, (CLOCK_SECOND * 5));    //handle the LED ourselves
    leds_on(LEDS_RED);                                  //use the red LED for locate
    return CALL_FAILED;                                 //prevent blue confirmation LED
}

standard_retval handleIntervalCmd(char* t, char* d){

    char* num = (char*)(d+9);
    uint8_t len = strlen((char*)(d+9));

    if(len > 0 ){
        NonVolatileConfig.datasendInterval = atoi(num);
        uint8_t r = syncConfig();
        return r;
    }
    return CALL_FAILED;
}

standard_retval handleBatIntCmd(char* t, char* d){

    char* num = (char*)(d+7);
    uint8_t len = strlen((char*)(d+7));
    if(len > 0 ){
        NonVolatileConfig.batterySendInterval = atoi(num);
        uint8_t r = syncConfig();
        return r;
    }
    return CALL_FAILED;
}

standard_retval handleChannelCmd(char* t, char* d){

    char* chan = (char*)(d+8);
    uint8_t len = strlen((char*)(d+8));
    if(len > 0 && len < 32){
        unsubscribe();
        memset(&(NonVolatileConfig.channelBase[0]), 0, 32);
        strncpy(&(NonVolatileConfig.channelBase[0]), chan, len);
        strncat(&(NonVolatileConfig.channelBase[0]), "/%s", 3);
        uint8_t r = syncConfig();
        return r;
    }
    return CALL_FAILED;
}

standard_retval initConfigCommands(){
    int8_t ret = CALL_OK;
    ret += addCommand("locate",    &handleLocateCmd);
    ret += addCommand("interval",  &handleIntervalCmd);
    ret += addCommand("batint",    &handleBatIntCmd);
    ret += addCommand("channel",   &handleChannelCmd);
    ret += addCommand("help",    &handleHelpCmd);

    if(ret != CALL_OK)
        return CALL_FAILED;

    return CALL_OK;
}


/*---------------------------------------------------------------------------*/
static void echo_reply_handler(uip_ipaddr_t *source, uint8_t ttl, uint8_t *data, uint16_t datalen) {

  if(uip_ip6addr_cmp(source, uip_ds6_defrt_choose())) {
    def_rt_rssi = sicslowpan_get_last_rssi();
  }

}

/*---------------------------------------------------------------------------*/
static void connect_to_broker(void) {

    LOG_DBG("Connect to broker\n");
   // mqtt_status_t r = mqtt_connect(&conn, conf.broker_ip, conf.broker_port, (conf.pub_interval * 3) / CLOCK_SECOND, MQTT_CLEAN_SESSION_OFF);
    mqtt_status_t r = mqtt_connect(&conn, conf.broker_ip, conf.broker_port, 900 *  CLOCK_SECOND, MQTT_CLEAN_SESSION_OFF);
    if(r!=MQTT_STATUS_OK)
        LOG_DBG("Error connecting: %d \n", r);

    state = STATE_CONNECTING;
}

/*---------------------------------------------------------------------------*/
static void ping_parent(void) {

  if(uip_ds6_get_global(ADDR_PREFERRED) == NULL) return;

  uip_icmp6_send(uip_ds6_defrt_choose(), ICMP6_ECHO_REQUEST, 0, ECHO_REQ_PAYLOAD_LEN);
}

/*---------------------------------------------------------------------------*/
static void state_machine(void) {

  char def_rt_str[64];
  switch(state) {

//=========================
// STATE_INIT
//=========================
  case STATE_INIT:
    /* If we have just been configured register MQTT connection */
    mqtt_register(&conn, main_process, client_id, mqtt_event, MAX_TCP_SEGMENT_SIZE);

    /* _register() will set auto_reconnect. We don't want that. */
    conn.auto_reconnect = 1;
    connect_attempt = 1;

    state = STATE_REGISTERED;
    LOG_DBG("Init\n");
    /* Continue */

//=========================
// STATE_REGISTERED
//=========================
  case STATE_REGISTERED:

    memset(def_rt_str, 0, sizeof(def_rt_str));

    if(uip_ds6_get_global(ADDR_PREFERRED) != NULL) {
      /* Registered and with a public IP. Connect */
      LOG_DBG("Registered. Connect attempt %u\n", connect_attempt);
      ping_parent();
      connect_to_broker();
    }

    etimer_set(&publish_periodic_timer, NET_CONNECT_PERIODIC);
    return;
    break;

//=========================
// STATE_CONNECTING
//=========================
  case STATE_CONNECTING:

    /* Not connected yet. Wait */
    LOG_DBG("Connecting (%u)\n", connect_attempt);
    break;

//====================================
// STATE_CONNECTED && STATE_PUBLISHING
//====================================
  case STATE_CONNECTED:
  case STATE_PUBLISHING:
    /* If the timer expired, the connection is stable. */
    if(timer_expired(&connection_life)) {
      /*
       * Intentionally using 0 here instead of 1: We want RECONNECT_ATTEMPTS
       * attempts if we disconnect after a successful connect
       */
      connect_attempt = 0;
    }

    if(mqtt_ready(&conn) && conn.out_buffer_sent) {
      /* Connected. Publish */
      if(state == STATE_CONNECTED) {
        //subscribe();
        state = STATE_PUBLISHING;
      } else {
        LOG_DBG("Ready to next\n");
      }
      etimer_set(&publish_periodic_timer, conf.pub_interval);
      /* Return here so we don't end up rescheduling the timer */
      return;
    } else {
      /*
       * Our publish timer fired, but some MQTT packet is already in flight
       * (either not sent at all, or sent but not fully ACKd).
       *
       * This can mean that we have lost connectivity to our broker or that
       * simply there is some network delay. In both cases, we refuse to
       * trigger a new message and we wait for TCP to either ACK the entire
       * packet after retries, or to timeout and notify us.
       */
      LOG_DBG("Waiting for TCP ack/Send confirmation... (MQTT state=%d, q=%u)\n", conn.state, conn.out_queue_full);
    }
    break;

//=========================
// STATE_DISCONNECTED
//=========================
  case STATE_DISCONNECTED:
    LOG_DBG("Disconnected\n");
    if(connect_attempt < RECONNECT_ATTEMPTS || RECONNECT_ATTEMPTS == RETRY_FOREVER) {
      /* Disconnect and backoff */
      clock_time_t interval;
      mqtt_disconnect(&conn);
      connect_attempt++;

      interval = connect_attempt < 3 ? RECONNECT_INTERVAL << connect_attempt : RECONNECT_INTERVAL << 3;

      LOG_DBG("Disconnected. Attempt %u in %lu ticks\n", connect_attempt, interval);

      etimer_set(&publish_periodic_timer, interval);

      state = STATE_REGISTERED;
      return;
    } else {
      /* Max reconnect attempts reached. Enter error state */
      state = STATE_ERROR;
      LOG_DBG("Aborting connection after %u attempts\n", connect_attempt - 1);
    }
    break;

//=========================
// STATE_CONFIG_ERROR
//=========================
  case STATE_CONFIG_ERROR:
    /* Idle away. The only way out is a new config */
    LOG_ERR("Bad configuration.\n");
    return;

//=========================
// STATE_ERROR
//=========================
  case STATE_ERROR:
  default:

    /*
     * 'default' should never happen.
     *
     * If we enter here it's because of some error. Stop timers. The only thing
     * that can bring us out is a new config event
     */
    LOG_ERR("Default case: State=0x%02x\n", state);
    return;
  }

  /* If we didn't return so far, reschedule ourselves */
  etimer_set(&publish_periodic_timer, STATE_MACHINE_PERIODIC);
}

/*---------------------------------------------------------------------------*/
void subscribe(void) {
  mqtt_status_t status;

  char subtopic[32];
  sprintf(subtopic, NonVolatileConfig.channelBase, "config");
  status = mqtt_subscribe(&conn, NULL, subtopic, MQTT_QOS_LEVEL_0);

  LOG_DBG("Subscribed to %s \n", subtopic);

  if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
    LOG_ERR("Tried to subscribe but command queue was full!\n");
  }
}

/*---------------------------------------------------------------------------*/
void unsubscribe(void){
    mqtt_status_t status;

    char subtopic[32];
    sprintf(subtopic, NonVolatileConfig.channelBase, "config");
    status = mqtt_unsubscribe(&conn, NULL, subtopic);

    LOG_DBG("Unsubscribing!\n");

    if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
      LOG_ERR("Tried to unsubscribe but command queue was full!\n");
    }
}


standard_retval publishStr(char* topic, char* msg) {

    sprintf(topic_buffer, NonVolatileConfig.channelBase, topic);

    if(!mqtt_ready(&conn) || !conn.out_buffer_sent){
        printf("Skipping, not ready\n");
        return CALL_FAILED;
    }

    mqtt_status_t r = mqtt_publish(&conn, NULL, topic_buffer, (uint8_t *)msg, strlen(msg), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);

    if(r!=MQTT_STATUS_OK)
        LOG_DBG("Error publishing: %d \n", r);

    if(r == MQTT_STATUS_OUT_QUEUE_FULL){
        LOG_DBG("Queue full, reconnecting\n");
        mqtt_clearQueue();
        conn.auto_reconnect = 1;
        connect_attempt = 1;

        state = STATE_REGISTERED;
    }
    return CALL_OK;
}

/*---------------------------------------------------------------------------*/
static void pub_handler(const char *topic, uint16_t topic_len, uint8_t *chunk, uint16_t chunk_len) {

    LOG_DBG("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len, chunk_len);
    int len = strlen(NonVolatileConfig.channelBase);
    if(topic_len != (len - 3 + 7)) return;

    int pos = 0;
    char c;
    for(int i=topic_len; i > 0;i--){
        c = topic[i];
        if(c == '/'){
            pos = i;
            break;
        }
    }

    if(pos != 0 && strncmp(&topic[pos+1], "config", topic_len - pos) == 0) {
      memcpy(mqtt_recv_command, (char*)chunk, chunk_len);
      process_post(main_process, mqtt_command_event, mqtt_recv_command);
      return;
    }

    return;

}

/*---------------------------------------------------------------------------*/
standard_retval initMQTT(struct process *process, RcvHandler handler){

    rcvHandler = handler;
    main_process = process;

    initConfigCommands();

    /* Populate configuration with default values */
    memset(&conf, 0, sizeof(mqtt_client_config_t));

    memcpy(conf.org_id, MQTT_CLIENT_ORG_ID, strlen(MQTT_CLIENT_ORG_ID));
    memcpy(conf.type_id, DEFAULT_TYPE_ID, strlen(DEFAULT_TYPE_ID));
    memcpy(conf.auth_token, MQTT_CLIENT_AUTH_TOKEN, strlen(MQTT_CLIENT_AUTH_TOKEN));
    memcpy(conf.event_type_id, DEFAULT_EVENT_TYPE_ID, strlen(DEFAULT_EVENT_TYPE_ID));
    memcpy(conf.broker_ip, broker_ip, strlen(broker_ip));
    memcpy(conf.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);

    conf.broker_port = DEFAULT_BROKER_PORT;
    conf.pub_interval = DEFAULT_PUBLISH_INTERVAL;
    conf.def_rt_ping_interval = DEFAULT_RSSI_MEAS_INTERVAL;

    etimer_set(&echo_request_timer, conf.def_rt_ping_interval);

    if(construct_client_id() == CALL_FAILED) {
      /* Fatal error. Client ID larger than the buffer */
      state = STATE_CONFIG_ERROR;
      return CALL_FAILED;
    }

    state = STATE_INIT;

    /*
     * Schedule next timer event ASAP
     *
     * If we entered an error state then we won't do anything when it fires.
     *
     * Since the error at this stage is a config error, we will only exit this
     * error state if we get a new config.
     */
    etimer_set(&publish_periodic_timer, 0);

    def_rt_rssi = 0x8000000;
    uip_icmp6_echo_reply_callback_add(&echo_reply_notification,  echo_reply_handler);

    mqtt_command_event = process_alloc_event();

    return CALL_OK;
}

/*---------------------------------------------------------------------------*/
standard_retval MQTTTasks(process_event_t ev,  process_data_t data){

    if((ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer) || ev == PROCESS_EVENT_POLL ) {
        LOG_DBG("Handle state_machine\n");
        state_machine();
    }

    if(ev == PROCESS_EVENT_TIMER && data == &echo_request_timer) {
        LOG_DBG("Ping parent\n");
        ping_parent();
        etimer_set(&echo_request_timer, conf.def_rt_ping_interval);
    }

    if( ev == mqtt_command_event ) {

          LOG_DBG("Received MQTT command\n");
          handleCommand("config",(char*)data);
      }
    if(ev == PROCESS_EVENT_TIMER && data == &resetLED_timer) {
        LOG_DBG("LED off\n");
        leds_off(LEDS_ALL);
    }
    return CALL_OK;
}

/*---------------------------------------------------------------------------*/
standard_retval construct_client_id(void) {

  int len = snprintf(client_id, BUFFER_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x",
                     conf.org_id, conf.type_id,
                     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                     linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
                     linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);


  if(len < 0 || len >= BUFFER_SIZE) {
    //LOG_ERR("Client ID: %d, Buffer %d\n", len, BUFFER_SIZE);
    return CALL_FAILED;
  }

  return CALL_OK;
}

/*---------------------------------------------------------------------------*/
ConnectionStatus isConnectionReady(){
    if(((state == STATE_CONNECTED || state == STATE_PUBLISHING) && (mqtt_ready(&conn) && conn.out_buffer_sent)))
        return ConnectionReady;

    return ConnectionPending;
}

/*---------------------------------------------------------------------------*/
void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data) {

  switch(event) {

    case MQTT_EVENT_CONNECTED: {
        LOG_DBG("Application has a MQTT connection\n");
        timer_set(&connection_life, CONNECTION_STABLE_TIME);
        subscribe();
        state = STATE_CONNECTED;
        break;
        }

    case MQTT_EVENT_DISCONNECTED: {
        LOG_DBG("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *)data));

        state = STATE_DISCONNECTED;
        process_poll(main_process);
        break;
        }

    case MQTT_EVENT_PUBLISH: {
        msg_ptr = data;

        /* Implement first_flag in publish message? */
        if(msg_ptr->first_chunk) {
          msg_ptr->first_chunk = 0;
          LOG_DBG("Application received publish for topic '%s'. Payload size is %i bytes.\n", msg_ptr->topic, msg_ptr->payload_length);
        }

        pub_handler(msg_ptr->topic, strlen(msg_ptr->topic), msg_ptr->payload_chunk, msg_ptr->payload_length);
        break;
        }

    case MQTT_EVENT_SUBACK: {
        LOG_DBG("Subscribed!\n\n");
        break;
        }
    default:
        LOG_DBG("Application got a unhandled MQTT event: %i\n", event);
        break;
  }
}




