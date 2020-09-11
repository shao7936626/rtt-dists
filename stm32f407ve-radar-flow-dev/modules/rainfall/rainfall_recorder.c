#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#define PIN_KEY0 GET_PIN(A, 6)

#define STACK_SIZE 512
#define THREAD_PRIORITY 30
#define TIMESLICE 5

rt_uint16_t rain_counts;

static void _callback(void *args)
{
    rt_pin_irq_enable(PIN_KEY0, PIN_IRQ_DISABLE);

    if (rt_pin_read(PIN_KEY0) == 1)
    {

    }
    else
    {
				rain_counts++;
        rt_kprintf("KEY0 UP\n");
    }
    rt_pin_irq_enable(PIN_KEY0, PIN_IRQ_ENABLE);
}

static void irq_thread_entry(void *parameter)
{
    rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT);
    rt_pin_attach_irq(PIN_KEY0, PIN_IRQ_MODE_FALLING , _callback, RT_NULL);
    rt_pin_irq_enable(PIN_KEY0, PIN_IRQ_ENABLE);
}

void init_rainfall()
{
    /* 定义线程句柄 */
    rt_thread_t tid;

    /* 创建动态pin线程 ：优先级 25 ，时间片 5个系统滴答，线程栈512字节 */
    tid = rt_thread_create("irq_thread",
                           irq_thread_entry,
                           RT_NULL,
                           STACK_SIZE,
                           THREAD_PRIORITY,
                           TIMESLICE);

    /* 创建成功则启动动态线程 */
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }

    //while (1)
    //{
    //    rt_kprintf("the pin is %d\n", rt_pin_read(PIN_KEY0));
    //    rt_thread_mdelay(20);
    // }
}
INIT_APP_EXPORT(init_rainfall);