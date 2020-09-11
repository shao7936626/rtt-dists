#include <rtthread.h>
#include <stdlib.h>
#define _UART_NAME "uart6" /* 串口设备名称 */

/* 串口接收消息结构*/
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};
/* 串口设备句柄 */
static rt_device_t serial;
/* 消息队列控制块 */
static struct rt_messagequeue rx_mq;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&rx_mq, &msg, sizeof(msg));
    if (result == -RT_EFULL)
    {
        /* 消息队列满 */
        rt_kprintf("message queue full！\n");
    }
    return result;
}

#define HEIGHT_RADAR_DATA_LEN 21

float average_height_data;
float total_height_data;

static void serial_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    static char rx_buffer[RT_SERIAL_RB_BUFSZ + 1];
    rt_uint16_t get_times = 0;
    while (1)
    {
        rt_memset(&msg, 0, sizeof(msg));
        /* 从消息队列中读取消息*/
        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result == RT_EOK)
        {
            /* 从串口读取数据*/
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            rx_buffer[rx_length] = '\0';

            /* 打印数据 */
            //rt_kprintf("the data is %s\n", rx_buffer);

            //rt_kprintf("the rx_buffer len is %d\n",rt_strlen(rx_buffer));

            if (rt_strlen(rx_buffer) == HEIGHT_RADAR_DATA_LEN)
            {
                if (rx_buffer[0] - 'R' == 0)
                {
                    if (rx_buffer[10] - 'R' == 0)
                    {
                        //rt_kprintf("the data is %s\n", rx_buffer);
                        float height_data = 0.0;
                        char f_data[8] = {0};
                        sprintf(f_data, "%.*s", 7, &rx_buffer[12]);
                        // rt_kprintf("the f_data is %s\n", f_data);
                        height_data = atof(f_data);
                        get_times++;
                        total_height_data += height_data;

                        if (get_times == 500)
                        {

                            get_times = 0;
                            average_height_data = total_height_data / 500.0;

                            int a = (int)average_height_data;
                            int b = (average_height_data - a) * 1000;

                            rt_kprintf(" the average_height is %d.%d\n", a, b);
                            total_height_data = 0;
                        }
                        //int a = (int)height_data;
                        //int b = (height_data - a) * 1000;
                        //rt_kprintf("the height_data is %d.%d \n", a, b);
                    }
                }
            }
        }
    }
}

static int uart6_dma(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    static char msg_pool[256];

    if (argc == 2)
    {
        rt_strncpy(uart_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(uart_name, _UART_NAME, RT_NAME_MAX);
    }

    /* 查找串口设备 */
    serial = rt_device_find(uart_name);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }

    /* 初始化消息队列 */
    rt_mq_init(&rx_mq, "rx_mq",
               msg_pool,              /* 存放消息的缓冲区 */
               sizeof(struct rx_msg), /* 一条消息的最大长度 */
               sizeof(msg_pool),      /* 存放消息的缓冲区大小 */
               RT_IPC_FLAG_FIFO);     /* 如果有多个线程等待，按照先来先得到的方法分配消息 */

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 25, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    return ret;
}
/* 导出到 msh 命令列表中 */
//MSH_CMD_EXPORT(uart6_dma, uart6 device dma);
INIT_APP_EXPORT(uart6_dma);