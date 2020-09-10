#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <rtthread.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "mqtt.radar"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "mqtt_client.h"
#include "base64.h"
#include <hmac.h>
#include "cJSON.h"

/**
 * MQTT URI farmat:
 * domain mode
 * tcp://iot.eclipse.org:1883
 *
 * ipv4 mode
 * tcp://192.168.10.1:1883
 * ssl://192.168.10.1:1884
 *
 * ipv6 mode
 * tcp://[fe80::20c:29ff:fe9a:a07e]:1883
 * ssl://[fe80::20c:29ff:fe9a:a07e]:1884
 */

#ifdef MQTT_USE_ALIYUN
#define MQTT_URI "tcp://test.mosquitto.org:1883" //"tcp://mq.tongxinmao.com:18831"
#define MQTT_SUBTOPIC "/mqtt/test"
#define MQTT_PUBTOPIC "/mqtt/test"
#define MQTT_WILLMSG "Goodbye!"
#else
#define MQTT_URI "tcp://post-cn-6ja1tkxv80e.mqtt.aliyuncs.com:1883"
#define SEC_ID "LTAI4G3LKnmTcjCbCGkMC7mY"
#define SEC_KEY "ox3gY9WKoc3ECbwzoOi5rha2cJSJHf"
#define ALI_USERNAME "Signature|LTAI4G3LKnmTcjCbCGkMC7mY|post-cn-6ja1tkxv80e"
#define GID_TITLE "GID_RADAR@@@"
#define MQTT_SUBTOPIC "DEVICE_CONTROL"
#define MQTT_PUBTOPIC_STATUS "DEVICE_STATUS"
#define MQTT_PUBTOPIC_DATA "DEVICE_DATA"
#endif
/* define MQTT client context */
static mqtt_client client;
int is_started = 0;

static void mqtt_sub_callback(mqtt_client *c, message_data *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub callback: %.*s %.*s",
          msg_data->topic_name->lenstring.len,
          msg_data->topic_name->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);

    cJSON *root = NULL;
    cJSON *item = NULL;
    root = cJSON_Parse((char *)msg_data->message->payload);
    if (!root)
    {
        rt_kprintf("Error before: [%s]\n", cJSON_GetErrorPtr());
    }
    else
    {
        rt_kprintf("%s\n", "有格式的方式打印Json:");
        rt_kprintf("%s\n\n", cJSON_Print(root));
        item = cJSON_GetObjectItem(root, "username");
        if (rt_strcmp("QCRL", item->valuestring) == 0)
        {
            item = cJSON_GetObjectItem(root, "password");
            if (rt_strcmp("123456", item->valuestring) == 0)
            {
                item = cJSON_GetObjectItem(root, "cmd");
                rt_kprintf("the cmd is %s", item->valuestring);
            }
        }
        else
        {
        }
        
        //cJSON_Delete(root);
        free(root);

    }

}

static void mqtt_sub_default_callback(mqtt_client *c, message_data *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub default callback: %.*s %.*s",
          msg_data->topic_name->lenstring.len,
          msg_data->topic_name->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);
}

static void mqtt_connect_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_connect_callback!");
}

static void mqtt_online_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_online_callback!");
}

static rt_uint8_t mqtt_error_times;
static time_t last_error;

static void mqtt_offline_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_offline_callback!");
    LOG_D("system will reboot after %d times", (9 - mqtt_error_times));
    time_t now_error = time(0);
    if (!last_error)
    {
        last_error = time(0);
    }
    if (difftime(now_error, last_error) < 30)
    {
        last_error = now_error;
        mqtt_error_times++;
    }
    else if (difftime(now_error, last_error) > 5 * 60)
    {
        mqtt_error_times = 0;
    }

    if (mqtt_error_times >= 10)
    {
        extern void rt_hw_cpu_reset(void);
        rt_hw_cpu_reset();
    }
}

extern char imei[];

static char *get_password()
{
    char sec_key[] = SEC_KEY;
    char data[50] = {0};

    sprintf(data, "%s%.*s", GID_TITLE, 15, imei);

    char out[256] = {0};
    int len = sizeof(out);

    hmac_sha1(sec_key, strlen(sec_key), data, strlen(data), out, &len);

    //rt_kprintf("out is %s \n len: %d\n", out, len);

    char *encode_out;
    encode_out = rt_malloc(BASE64_ENCODE_OUT_SIZE(len));
    base64_encode(out, len, encode_out);

    //rt_kprintf("imei is %s  encode is  %s \n ", imei, encode_out);

    return encode_out;
}
MSH_CMD_EXPORT(get_password, get_password);

static int radar_mqtt_start(int argc, char **argv)
{

    LOG_I("start init radar mqtt");
    /* init condata param by using MQTTPacket_connectData_initializer */
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    static char cid[30] = {0};

    if (argc != 1)
    {
        rt_kprintf("mqtt_start    --start a mqtt worker thread.\n");
        return -1;
    }

    if (is_started)
    {
        LOG_E("mqtt client is already connected.");
        return -1;
    }
    /* config MQTT context param */
    {
        client.isconnected = 0;
        client.uri = MQTT_URI;

        /* generate the aliyun mqtt  client ID */
        snprintf(cid, sizeof(cid), "%s%.*s", GID_TITLE, 15, imei);
        LOG_I("mqtt clientId is %s", cid);
        /* config connect param */
        memcpy(&client.condata, &condata, sizeof(condata));
        client.condata.clientID.cstring = cid;
        client.condata.username.cstring = ALI_USERNAME;
        LOG_I("mqtt username is %s", ALI_USERNAME);
        client.condata.password.cstring = get_password();
        LOG_I("mqtt password is %s", get_password());
        client.condata.keepAliveInterval = 30;
        client.condata.cleansession = 1;

        /* malloc buffer. */
        client.buf_size = client.readbuf_size = 1024;
        client.buf = rt_calloc(1, client.buf_size);
        client.readbuf = rt_calloc(1, client.readbuf_size);
        if (!(client.buf && client.readbuf))
        {
            LOG_E("no memory for MQTT client buffer!");
            return -1;
        }

        /* set event callback function */
        client.connect_callback = mqtt_connect_callback;
        client.online_callback = mqtt_online_callback;
        client.offline_callback = mqtt_offline_callback;

        /* set subscribe table and event callback */
        char sub_topic[40] = {0};
        sprintf(sub_topic, "%s/%s", MQTT_SUBTOPIC, imei);
        LOG_I("subscribe to %s", sub_topic);
        client.message_handlers[0].topicFilter = rt_strdup(sub_topic);
        client.message_handlers[0].callback = mqtt_sub_callback;
        client.message_handlers[0].qos = QOS1;

        /* set default subscribe event callback */
        client.default_message_handlers = mqtt_sub_default_callback;
    }

    {
        int value;
        uint16_t u16Value;
        value = 5;
        paho_mqtt_control(&client, MQTT_CTRL_SET_CONN_TIMEO, &value);
        value = 5;
        paho_mqtt_control(&client, MQTT_CTRL_SET_MSG_TIMEO, &value);
        value = 5;
        paho_mqtt_control(&client, MQTT_CTRL_SET_RECONN_INTERVAL, &value);
        value = 30;
        paho_mqtt_control(&client, MQTT_CTRL_SET_KEEPALIVE_INTERVAL, &value);
        u16Value = 3;
        paho_mqtt_control(&client, MQTT_CTRL_SET_KEEPALIVE_COUNT, &u16Value);
    }

    /* run mqtt client */
    paho_mqtt_start(&client, 8196, 20);
    is_started = 1;

    return 0;
}

static int radar_mqtt_stop(int argc, char **argv)
{
    if (argc != 1)
    {
        rt_kprintf("mqtt_stop    --stop mqtt worker thread and free mqtt client object.\n");
    }

    is_started = 0;

    return paho_mqtt_stop(&client);
}

static int radar_mqtt_publish(int argc, char **argv)
{
    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    if (argc == 2)
    {
        paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC_STATUS, argv[1], strlen(argv[1]));
    }
    else if (argc == 3)
    {
        paho_mqtt_publish(&client, QOS1, argv[1], argv[2], strlen(argv[2]));
    }
    else
    {
        rt_kprintf("mqtt_publish <topic> [message]  --mqtt publish message to specified topic.\n");
        return -1;
    }

    return 0;
}

static void mqtt_new_sub_callback(mqtt_client *client, message_data *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt new subscribe callback: %.*s %.*s",
          msg_data->topic_name->lenstring.len,
          msg_data->topic_name->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);
}

static int radar_mqtt_subscribe(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("mqtt_subscribe [topic]  --send an mqtt subscribe packet and wait for suback before returning.\n");
        return -1;
    }

    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    return paho_mqtt_subscribe(&client, QOS1, argv[1], mqtt_new_sub_callback);
}

static int radar_mqtt_unsubscribe(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("mqtt_unsubscribe [topic]  --send an mqtt unsubscribe packet and wait for suback before returning.\n");
        return -1;
    }

    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    return paho_mqtt_unsubscribe(&client, argv[1]);
}

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(radar_mqtt_start, startup mqtt client);
MSH_CMD_EXPORT(radar_mqtt_stop, stop mqtt client);
MSH_CMD_EXPORT(radar_mqtt_publish, mqtt publish message to specified topic);
MSH_CMD_EXPORT(radar_mqtt_subscribe, mqtt subscribe topic);
MSH_CMD_EXPORT(radar_mqtt_unsubscribe, mqtt unsubscribe topic);
#endif /* FINSH_USING_MSH */

int radar_mqtt_pub(char *msg, int topic_index, int len)
{
    char topic[100] = {0};
    switch (topic_index)
    {
    case 0:
        sprintf(topic, "%s/%s/status", MQTT_PUBTOPIC_STATUS, imei);
        break;
    case 1:
        sprintf(topic, "%s/%s/data", MQTT_PUBTOPIC_DATA, imei);
    default:
        break;
    }
    LOG_I("publish %s to %s", msg, topic);
    paho_mqtt_publish(&client, QOS1, topic, msg, len);
    rt_thread_mdelay(2000);
}

extern rt_uint8_t internet_up;

static void mqtt_busniess_entry(void *p)
{
    while (!internet_up)
    {
        rt_thread_mdelay(1000);
    }

    rt_thread_mdelay(2000);

    radar_mqtt_start(1, RT_NULL);
}

void mqtt_busniess_start(void)
{
    rt_thread_t tid;
    tid = rt_thread_create("mqtt_busniess_start", mqtt_busniess_entry, RT_NULL,
                           10240, 27, 20);
    if (tid)
    {
        rt_thread_startup(tid);
    }
}

INIT_APP_EXPORT(mqtt_busniess_start);
