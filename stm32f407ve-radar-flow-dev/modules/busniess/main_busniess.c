#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <rtthread.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "main.radar"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "sht20.h"
#include "cJSON.h"

#define BUSNIESS_STACK_SIZE 1024
#define BUSNIESS_THREAD_PRIORITY 8
#define BUSNIESS_TIMESLICE 20

typedef struct _radar_data
{
    float temp;
    float humi;
    float height_gauge;
    float rain_gauge;
    float water_level;
    float Preset_Value_Level;
    float Correction_Value_Level;
} radar_data;

static radar_data upload_radar_data;
static struct rt_event radar_event;

extern int is_started;
extern rt_uint8_t rssi;

extern sht20_device_t global_sht20;
extern int radar_mqtt_pub(char *msg, int topic_index, int len);

int radar_event_send(uint32_t event)
{
    return (int)rt_event_send(&radar_event, event);
}

int radar_event_recv(uint32_t event, uint32_t timeout, rt_uint8_t option)
{
    int result = RT_EOK;
    rt_uint32_t recved;

    result = rt_event_recv(&radar_event, event, option | RT_EVENT_FLAG_CLEAR, timeout, &recved);
    if (result != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    return recved;
}

static int get_tem(void)
{
    upload_radar_data.temp = sht20_read_temperature(global_sht20);
    return 0;
}

static int get_hum(void)
{
    upload_radar_data.humi = sht20_read_humidity(global_sht20);
    return 0;
}
extern char imei[];
static int get_data(int heart)
{
    if (heart)
    {
        char *pub_data;
        get_tem();
        get_hum();

        cJSON *root = cJSON_CreateObject();
        cJSON *body = cJSON_CreateObject();

        cJSON_AddItemToObject(root, "body", body);

        cJSON_AddItemToObject(body, "VER", cJSON_CreateString("0.1"));
        cJSON_AddItemToObject(body, "imei", cJSON_CreateString(imei));
        // cJSON_AddItemToObject(body, "0029", cJSON_CreateNumber((double)upload_radar_data.temp));
        char temp[6] = {0};
        sprintf(temp, "%2.1f", upload_radar_data.temp);
        cJSON_AddItemToObject(body, "temperature", cJSON_CreateString(temp));
        // cJSON_AddItemToObject(body, "0041", cJSON_CreateNumber((double)upload_radar_data.humi));
        char humi[6] = {0};
        sprintf(humi, "%2.1f", upload_radar_data.humi);
        cJSON_AddItemToObject(body, "humidity", cJSON_CreateString(humi));
        //cJSON_AddItemToObject(body, "0043", cJSON_CreateString("0"));
        char upload_rssi[6] = {0};
        sprintf(upload_rssi, "%d", rssi);
        cJSON_AddItemToObject(body, "RSSI", cJSON_CreateString(upload_rssi));

        //height gauge
        extern float average_height_data;
        char upload_height[6] = {0};
        sprintf(upload_height, "%.3f", average_height_data);
        cJSON_AddItemToObject(body, "height_gauge", cJSON_CreateString(upload_height));
        //

        //rain gauge
        extern rt_uint16_t rain_counts;
        cJSON_AddItemToObject(body, "rain_gauge", cJSON_CreateNumber(rain_counts / 2));
        rain_counts = 0;
        //ADC
        cJSON_AddItemToObject(body, "ADC", cJSON_CreateString("23.97"));
        cJSON_AddItemToObject(body, "water_level", cJSON_CreateString("0.0"));
        cJSON_AddItemToObject(body, "Preset_Value_Level", cJSON_CreateString("10.0"));
        cJSON_AddItemToObject(body, "Correction_Value_Level", cJSON_CreateString("0.0"));

        pub_data = cJSON_Print(root);

        //sprintf(pub_data, "{\"body\":{\"0057\":36,\"0029\":%2.1f,\"0041\":%2.1f,\"0043\":%d,\"0044\":%d}}", upload_radar_data.temp, upload_radar_data.humi, 0, rssi);
        radar_mqtt_pub(pub_data, 1, rt_strlen(pub_data));

        free(pub_data);

        cJSON_Delete(root);
    }
}

static void _busniess_thead_entry(void *param)
{
    while (!is_started)
    {
        rt_thread_mdelay(1000);
    }
    int count = 0;
    while (1)
    {

        rt_thread_mdelay(1000);
        count++;

        if (count % 5 == 0)
        {
        }

        if (count == 60)
        {
            get_data(1);
            count = 0;
        }
    }
}

int main_busniess_start()
{
    /* 定义线程句柄 */
    rt_thread_t tid;
    rt_err_t result;

    result = rt_event_init(&radar_event, "event", RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        rt_kprintf("init event failed.\n");
        return -1;
    }

    tid = rt_thread_create("busniess_thread",
                           _busniess_thead_entry,
                           RT_NULL,
                           BUSNIESS_STACK_SIZE,
                           BUSNIESS_THREAD_PRIORITY,
                           BUSNIESS_TIMESLICE);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    return 0;
}

INIT_APP_EXPORT(main_busniess_start);