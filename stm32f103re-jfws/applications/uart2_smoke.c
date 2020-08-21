#include <rtthread.h>
#include "board.h"
#define SAMPLE_UART_NAME "uart1" /* 串口设备名称 */

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

struct serial_configure sl_uart1_config =
    {
        BAUD_RATE_9600,     /* 9600 bits/s */
        DATA_BITS_8,        /* 8 databits */
        STOP_BITS_1,        /* 1 stopbit */
        PARITY_NONE,        /* No parity  */
        BIT_ORDER_LSB,      /* LSB first sent */
        NRZ_NORMAL,         /* Normal mode */
        RT_SERIAL_RB_BUFSZ, /* Buffer size */
        0};

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

static void serial_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    rt_uint8_t rx_buffer[RT_SERIAL_RB_BUFSZ + 1];
    rt_uint16_t smoke_data = 0;
    int i = 0;
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
            /* 通过串口设备 serial 输出读取到的消息 */
            //rt_device_write(serial, 0, rx_buffer, rx_length);
            /* 打印数据 */
            for (i = 0; i < rx_length; i++)
            {
                rt_kprintf("rx_buffer[%d] = %02x\n", i, rx_buffer[i]);
            }
            rt_kprintf("%s\n", rx_buffer);

            if (rx_buffer[0] == 0x01 && rx_buffer[1] == 0x02 && rx_buffer[2] == 0x01 && rx_length == 6)
            {
                

                rt_kprintf("smoke_data = %02x\n", rx_buffer[3]);
            }
        }
    }
}

static int uart1_dma_smoke(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    static char msg_pool[256];
    char str[] = "hello RT-Thread!\r\n";

    if (argc == 2)
    {
        rt_strncpy(uart_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);
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

    if (RT_EOK != rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, (void *)&sl_uart1_config))
    {
        rt_kprintf("uart config baud rate failed.\n");
    }

    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);
    /* 发送字符串 */
    //rt_device_write(serial, 0, str, (sizeof(str) - 1));

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
//MSH_CMD_EXPORT(uart_dma_sample, uart device dma sample);

INIT_APP_EXPORT(uart1_dma_smoke);

#define PIN_485DE GET_PIN(A, 8)

void get_smoke_data(void)
{
    rt_pin_mode(PIN_485DE, PIN_MODE_OUTPUT);
    char buf[8] = {0x01, 0x02, 0x00, 0x00, 0x00, 0x01, 0xB9, 0xca};
    rt_pin_write(PIN_485DE, PIN_HIGH);
    rt_device_write(serial, 0, buf, 8);
    rt_pin_write(PIN_485DE, PIN_LOW);
}
MSH_CMD_EXPORT(get_smoke_data, get_smoke_data);
