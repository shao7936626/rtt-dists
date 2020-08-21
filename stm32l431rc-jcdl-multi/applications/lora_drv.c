#if 1

// 0x00：网关
// 0x01：多合一传感器
// 0x02：水浸传感器
// 0x03：井盖位移传感器
// 0x04：烟感传感器
// 0x05：温湿度传感器
// 0x06：无线测温传感器
// 0x07：有毒有害气体传感器
// 0x08  sf6传感器
// 0x09  o2传感器
// 0x10  o3 传感器
// 0x11  噪声传感器

#include "lora_drv.h"
#include "string.h"
#include "crc.h"
#include <stdlib.h>
#include "sht20.h"
#include "board.h"

#define DBG_TAG "lora"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define LORA_UART_NAME "uart1" /* 串口设备名称 */

static int lora_sending_data = 0;

enum jcdl_sensor
{
    GATEWAY = 0,
    MULTISENSOR,
    WATERSOAK,
    SHIFT,
    SMOKE,
    TEMPANDHUM,
    WIRELESSTEMP,
    TOCIXGAS,
    SF6,
    O2,
    O3,
    VOICE
};

static enum jcdl_sensor THIS_TYPE = VOICE;

struct serial_configure sl_uart2_config =
    {
        BAUD_RATE_9600,     /* 9600 bits/s */
        DATA_BITS_8,        /* 8 databits */
        STOP_BITS_1,        /* 1 stopbit */
        PARITY_NONE,        /* No parity  */
        BIT_ORDER_LSB,      /* LSB first sent */
        NRZ_NORMAL,         /* Normal mode */
        RT_SERIAL_RB_BUFSZ, /* Buffer size */
        0};

/* 串口接收消息结构*/
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};
/* 串口设备句柄 */
static rt_device_t serial;
/* 消息队列控制块 */
static struct rt_messagequeue rx_mq_uart;

void run_peg_loop_immediately(void);

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&rx_mq_uart, &msg, sizeof(msg));
    if (result == -RT_EFULL)
    {
        /* 消息队列满 */
        rt_kprintf("message queue full！\n");
    }
    return result;
}

void serial_thread_entry(void *parameter)
{

    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    static char rx_buffer[RT_SERIAL_RB_BUFSZ + 1];
    int i = 0;

    while (1)
    {
        rt_memset(&msg, 0, sizeof(msg));
        /* 从消息队列中读取消息*/
        result = rt_mq_recv(&rx_mq_uart, &msg, sizeof(msg), RT_WAITING_FOREVER);

        if (result == RT_EOK)
        {
            /* 从串口读取数据*/
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            rx_buffer[rx_length] = '\0';

            for (i = 0; i < rx_length; i++)
            {
                rt_kprintf("rx_buffer[%d] = %02x\n", i, rx_buffer[i]);
            }
            rt_kprintf("\n------------------------------\n \n result is: \n%s\n ", rx_buffer);

            //0xAA 0x01 0x02 0x01 0xFF 0x41 0x03
            if ((int)rx_buffer[0] == 0xaa && (int)rx_buffer[1] == 0x01 && (int)rx_buffer[2] == 0x02 && (int)rx_buffer[3] == 0x01 && (int)rx_buffer[4] == 0xff)
            {
                rt_kprintf("\n------------------------------\n \n GET msg from server and SEND data back\n ");
                run_peg_loop_immediately();
            }
        }
    }
}
#if 0
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
#endif
static void send_jcdl_login_data()
{
    rt_uint8_t login_data[9] = {0};
    login_data[0] = 0xaa;
    login_data[1] = THIS_TYPE; //设备类型
    login_data[2] = 0xff;
    login_data[3] = 0x00;
    login_data[4] = 0x02;
    login_data[5] = 0xff;
    login_data[6] = 0x00;
    uint16_t crc_data_all = crc16(login_data, 7);
    login_data[7] = crc_data_all & 0xff;
    login_data[8] = (crc_data_all >> 8) & 0xff;
    rt_device_write(serial, 0, login_data, sizeof(login_data));
}

static void _discrete_report(rt_uint8_t type, int data)
{
    rt_uint8_t zgys_data[8] = {0};

    zgys_data[0] = 0xaa;
    zgys_data[1] = type;
    zgys_data[2] = 0xff;
    zgys_data[3] = 0x01; //cmd:upload
    zgys_data[4] = data & 0xff;
    zgys_data[5] = (data >> 8) & 0xff;
    uint16_t crc_data_all = crc16(zgys_data, 6);
    zgys_data[6] = crc_data_all & 0xff;
    zgys_data[7] = (crc_data_all >> 8) & 0xff;

    rt_device_write(serial, 0, zgys_data, sizeof(zgys_data));
    return;
}

static void _report_tah(rt_uint8_t type, int data)
{
    rt_uint8_t zgys_data[11] = {0};
    zgys_data[0] = 0xaa;
    zgys_data[1] = type;

    zgys_data[2] = 0xff;
    zgys_data[3] = 0x01;
    zgys_data[4] = 0x04;

    zgys_data[5] = 0x04;
    zgys_data[6] = 0x01;
    zgys_data[7] = 0x8D;
    zgys_data[8] = 0x02;

    uint16_t crc_data_all = crc16(zgys_data, 9);
    zgys_data[9] = crc_data_all & 0xff;
    zgys_data[10] = (crc_data_all >> 8) & 0xff;

    rt_device_write(serial, 0, zgys_data, sizeof(zgys_data));
}

static void _report_normal(rt_uint8_t type, int data)
{
    rt_uint8_t zgys_data[9] = {0};
    zgys_data[0] = 0xaa;
    zgys_data[1] = type;

    zgys_data[2] = 0xff;
    zgys_data[3] = 0x01;
    zgys_data[4] = 0x04;
    zgys_data[5] = data & 0xff;
    zgys_data[6] = (data >> 8) & 0xff;

    uint16_t crc_data_all = crc16(zgys_data, 7);
    zgys_data[7] = crc_data_all & 0xff;
    zgys_data[8] = (crc_data_all >> 8) & 0xff;

    rt_device_write(serial, 0, zgys_data, sizeof(zgys_data));
}

static void _analog_report(rt_uint8_t type, int data)
{

    if (type == 0x05)
    {
        _report_tah(type, data);
    }
    else
    {
        _report_normal(type, data);
    }
}



static void _data_report(rt_uint8_t type, int data)
{
    switch (type)
    {
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x07:
        _discrete_report(type, data);
        break;
    case 0x05:
    case 0x06:
    case 0x08:
    case 0x09:
    case 0x10:
    case 0x11:
        _analog_report(type, data);
        break;
    case 0x01:
        break;
    default:
        break;
    }
}

void send_jcdl_sensor_data(int data)
{

    _data_report(THIS_TYPE, data);
}

void uart_dma_send_lora(int argc, char *argv[])
{
    char str[20] = {0};
    char tail[2] = "\r\n";
    if (argc == 2)
    {
        rt_strncpy(str, argv[1], strlen(argv[1]));
        if (strstr(str, "+++") == NULL)
            strcat(str, tail);
        rt_device_write(serial, 0, str, strlen(str));
    }
}
MSH_CMD_EXPORT(uart_dma_send_lora, uart_dma_send_lora);

#define INTERVAL_TIME 50

static void jcdl_config(void)
{

    char *cmd[] = {
        [0] = "+++",
        [1] = "AT+NET=00\r\n",
        [2] = "AT+TFREQ=1C083560\r\n",
        [3] = "AT+RFREQ=1DD1F8E0\r\n",
        [4] = "AT+RIQ=00\r\n",
        [5] = "AT+TSF=07\r\n",
        [6] = "AT+RSF=07\r\n",
        [7] = "AT+LCP=0005\r\n",
        [8] = "AT+RXW=00\r\n",
        [9] = "AT+SIP=01\r\n",
        [10] = "AT+SIP=01\r\n",
        [11] = "AT+RPREM=00C8\r\n",
        [12] = "ATT\r\n"};

    for (int i = 0; i < sizeof(cmd) / sizeof(char *); i++)
    {
        rt_device_write(serial, 0, cmd[i], strlen(cmd[i]));
        rt_thread_mdelay(INTERVAL_TIME);
    }
}

void set_lora_config(int argc, char *argv[])
{
    int config_index = 0;
    config_index = atoi(argv[1]);

    if (argc == 2)
    {
        switch (config_index)
        {
        case 1:
        case 2:
        case 3:
            jcdl_config();
        default:
            break;
        }
    }
}
MSH_CMD_EXPORT(set_lora_config, set_lora_config);

struct rt_messagequeue mq;
static rt_uint8_t msg_pool[128];

static void timeout1(void *parameter)
{
    int msg = 0x01;
    rt_mq_send(&mq, &msg, 1);
}

time_t last;
void run_peg_loop(void)
{

    time_t now;
    now = time(0);
    if (lora_sending_data == 0)
    {
        lora_sending_data = 1;

        if (difftime(now, last) > 3600)
        {

            //send_peg_login_data();
            rt_thread_mdelay(5000);

            //send_peg_sensor_data();
            rt_thread_mdelay(5000);

            last = now;
        }
        lora_sending_data = 0;
    }
}

void run_peg_loop_immediately(void)
{

    if (lora_sending_data == 0)
    {
        lora_sending_data = 1;

        //send_peg_login_data();
        rt_thread_mdelay(5000);

        //send_peg_sensor_data();
        rt_thread_mdelay(5000);
        lora_sending_data = 0;
    }
}

static void lora_loop_entry(void *parameter)
{
    char buf = 0;
    while (1)
    {
        if (rt_mq_recv(&mq, &buf, sizeof(buf), RT_WAITING_FOREVER) == RT_EOK)
        {
            if ((buf - 0x01) == 0)
            {
                run_peg_loop();
            }
        }
    }
}

void lora_loop(void)
{
    rt_err_t result;
    rt_timer_t timer1;
    // timer1 = rt_timer_create("timer1", timeout1,
    //                          RT_NULL, 1000 * 3600 * 2,
    //                          RT_TIMER_CTRL_SET_PERIODIC);

    timer1 = rt_timer_create("timer1", timeout1,
                             RT_NULL, 1000 * 30,
                             RT_TIMER_CTRL_SET_PERIODIC);
    if (timer1 != RT_NULL)
        rt_timer_start(timer1);

    result = rt_mq_init(&mq,
                        "mqt",
                        &msg_pool[0],      /* 内存池指向 msg_pool */
                        1,                 /* 每个消息的大小是 1 字节 */
                        sizeof(msg_pool),  /* 内存池的大小是 msg_pool 的大小 */
                        RT_IPC_FLAG_FIFO); /* 如果有多个线程等待，按照先来先得到的方法分配消息 */

    if (result != RT_EOK)
    {
        rt_kprintf("init message queue failed.\n");
        return;
    }

    rt_thread_t lora_loop;
    lora_loop = rt_thread_create("lora_loop",
                                 lora_loop_entry,
                                 RT_NULL,
                                 1024, RT_THREAD_PRIORITY_MAX / 2 - 1, 20);
    if (lora_loop != RT_NULL)
        rt_thread_startup(lora_loop);
}

INIT_APP_EXPORT(lora_loop);

int uart_dma()
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    static char msg_pool[1024];
    rt_strncpy(uart_name, LORA_UART_NAME, RT_NAME_MAX);

    /* 查找串口设备 */
    serial = rt_device_find(uart_name);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }

    /* 初始化消息队列 */
    rt_mq_init(&rx_mq_uart, "rx_mq",
               msg_pool,              /* 存放消息的缓冲区 */
               sizeof(struct rx_msg), /* 一条消息的最大长度 */
               sizeof(msg_pool),      /* 存放消息的缓冲区大小 */
               RT_IPC_FLAG_FIFO);     /* 如果有多个线程等待，按照先来先得到的方法分配消息 */

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);

    if (RT_EOK != rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, (void *)&sl_uart2_config))
    {
        rt_kprintf("uart config baud rate failed.\n");
    }

    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 2048, 12, 20);
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
INIT_APP_EXPORT(uart_dma);
#endif