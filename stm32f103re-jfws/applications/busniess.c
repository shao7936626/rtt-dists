#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "sht20.h"

#define STACK_SIZE 1024
#define THREAD_PRIORITY 25
#define TIMESLICE 20

#define PIN_KEY0 GET_PIN(A, 12) //KEY0 220
#define PIN_KEY1 GET_PIN(B, 3)  //KEY1  ups

extern sht20_device_t global_sht20;
extern float sht20_read_humidity(sht20_device_t dev);
extern float sht20_read_temperature(sht20_device_t dev);

static int get_tem(void)
{
    float temperature;
    temperature = sht20_read_temperature(global_sht20);
    int tem_send = (int)(temperature * 10);
    return tem_send;
}

static int get_hum(void)
{
    float humidity;
    humidity = sht20_read_humidity(global_sht20);
    int hu_send = (int)(humidity * 10);
    return hu_send;
}

static void gpio_scanner(void)  //
{
    int key0 = 0;
    int key1 = 0;

    key0 = rt_pin_read(PIN_KEY0); //220
    key1 = rt_pin_read(PIN_KEY1); //ups
}

static void smoke_decetetor(void) //
{

    extern void get_smoke_data(void);
    get_smoke_data(); // somke
}



static void busniess_thread_entry(void *p)
{
        
    
}

static void busniess_thread(void)
{
    /* 定义线程句柄 */
    rt_thread_t tid;

    //定义

    /* 创建动态pin线程 ：优先级 25 ，时间片 5个系统滴答，线程栈512字节 */
    tid = rt_thread_create("busniess_thread",
                           busniess_thread_entry,
                           RT_NULL,
                           STACK_SIZE,
                           THREAD_PRIORITY,
                           TIMESLICE);

    /* 创建成功则启动动态线程 */
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
}