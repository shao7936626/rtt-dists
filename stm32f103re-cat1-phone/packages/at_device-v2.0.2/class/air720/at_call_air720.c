/*
 * File      : at_socket_air720.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-06-12     malongwei    first version
 * 2019-05-13     chenyong     multi AT socket client support
 */
//air720 打电话
// 1.测试前使用AT+SETVOLTE=1打开VOLTE功能
// 2.使用AT+CMUT=0 关闭静音。1：打开静音；0：关闭静音
// 3.使用AT+AUDCH=1设置音频Output通道。0：听筒；1：耳机；2：Speaker。
// 4、使用AT+CLVL=60 调整音量大小。最大100

#include <stdio.h>
#include <string.h>
#include <at_device_air720.h>

#if !defined(AT_SW_VERSION_NUM) || AT_SW_VERSION_NUM < 0x10300
#error "This AT Client version is older, please check and update latest AT Client!"
#endif

#define LOG_TAG "at.skt"
#include <at_log.h>

#if defined(AT_DEVICE_USING_AIR720) && defined(AT_USING_SOCKET)

#define AIR720_MODULE_SEND_MAX_SIZE 1000
#define CALL_EVENT_MSG_NUM 10
/* set real event by current socket and current state */
#define SET_EVENT(socket, event) (((socket + 1) << 16) | (event))

typedef enum
{
    CALL_EVENT_SRC_NONE,
    CALL_EVENT_SRC_CALLING,
    CALL_EVENT_SRC_BUSY,
    CALL_EVENT_SRC_NOANSWER,
    CALL_EVENT_SRC_HANGUP
} call_event_src_t;

typedef struct _call_msg
{
    char *data;
} call_msg_type_t;

typedef struct _event
{
    call_event_src_t event_src;
    call_msg_type_t type;
} call_event_t;

typedef struct _event_msg
{
    call_event_t event;

} call_event_msg_t;

static rt_err_t call_event_put(call_event_msg_t *msg);

rt_event_t call_event;
call_event_msg_t event_msg[CALL_EVENT_MSG_NUM];
struct rt_messagequeue call_event_mq;

static int air720_call_event_send(struct at_device *device, uint32_t event)
{
    return (int)rt_event_send(call_event, event);
}

static int air720_call_event_recv(struct at_device *device, uint32_t event, uint32_t timeout, rt_uint8_t option)
{
    int result = RT_EOK;
    rt_uint32_t recved;

    result = rt_event_recv(call_event, event, option | RT_EVENT_FLAG_CLEAR, timeout, &recved);
    if (result != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    return recved;
}

static int check_me_status(char *client_name)
{
    at_response_t resp = RT_NULL;
    struct at_device *device = RT_NULL;
    char parsed_data[5] = {0};
    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_CLIENT, client_name);
    if (device == RT_NULL)
    {
        LOG_E("get air720 device by client name(%s) failed.", client_name);
        return -RT_ERROR;
    }

    resp = at_create_resp(64, 0, 300);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for air720 device(%s) response structure.", device->name);
        return -RT_ENOMEM;
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+CPAS") < 0)
    {
        return -RT_ERROR;
    }

    if (at_resp_parse_line_args_by_kw(resp, "+CPAS:", "+CPAS:%s", &parsed_data) < 0)
    {
        LOG_E("can't get cpas");
    }
    else
    {
        LOG_I("the cpas is %s", parsed_data);
    }
}

static void urc_sound_func(struct at_client *client, const char *data, rt_size_t size)
{
    LOG_I("phone sound -----------------------"); //正在拨打电话
    char *client_name = client->device->parent.name;
    call_event_msg_t msg;
    msg.event.event_src = CALL_EVENT_SRC_CALLING;
    msg.event.type.data = client_name;
    call_event_put(&msg);
}

static void urc_ring_func(struct at_client *client, const char *data, rt_size_t size)
{
    LOG_I("coming a phone -----------------------"); //电话打入
}

static void urc_connect_func(struct at_client *client, const char *data, rt_size_t size)
{
    LOG_I("phone connected-----------------------"); //对方接听
}

static void urc_hangup_func(struct at_client *client, const char *data, rt_size_t size)
{
    LOG_I("phone hangup-----------------------"); //电话挂断
}

static void urc_noanswer_func(struct at_client *client, const char *data, rt_size_t size)
{
    LOG_I("phone hangup-----------------------"); //无人接听
}

static void urc_busy_func(struct at_client *client, const char *data, rt_size_t size)
{
    LOG_I("phone bush-----------------------"); //电话占线
}

/* air720 device URC table for the phone data */
static const struct at_urc call_urc_table[] =
    {
        {"+CIEV: \"SOUNDER\"", "\r\n", urc_sound_func},
        {"RING", "\r\n", urc_ring_func},
        {"CONNECT", "\r\n", urc_connect_func},
        {"NO CARRIER", "\r\n", urc_hangup_func},
        {"NO ANSWER", "\r\n", urc_noanswer_func},
        {"BUSY", "\r\n", urc_busy_func},
};

/**
 * @description: 
 * @param {type} 
 * @return: 
 */
static rt_err_t call_event_put(call_event_msg_t *msg)
{
    rt_err_t rst = RT_EOK;

    rst = rt_mq_send(&call_event_mq, msg, sizeof(call_event_msg_t));
    if (rst == -RT_EFULL)
    {
        LOG_I("onenet event queue is full");
        rt_thread_mdelay(100);
        rst = -RT_EFULL;
    }
    return rst;
}

/**
 * @description: 
 * @param {type} 
 * @return: 
 */
static rt_err_t call_event_get(call_event_msg_t *msg, int timeout)
{
    rt_err_t rst = RT_EOK;

    rst = rt_mq_recv(&call_event_mq, msg, sizeof(call_event_msg_t), timeout);
    if ((rst != RT_EOK) && (rst != -RT_ETIMEOUT))
    {
        LOG_E("onenet event get failed! Errno: %d", rst);
    }
    return rst;
}

static rt_err_t call_event_process(call_event_msg_t *msg)
{
    rt_err_t rst = RT_EOK;
    switch (msg->event.event_src)
    {
    case CALL_EVENT_SRC_CALLING:
        LOG_I("process call ");

        check_me_status(msg->event.type.data);
        break;
    case CALL_EVENT_SRC_BUSY:
        break;
    case CALL_EVENT_SRC_NOANSWER:
        break;
    case CALL_EVENT_SRC_HANGUP:
        break;
    default:
        break;
    }
}

static void air720_call_thread_entry(void *parameter)
{
    rt_err_t rst = RT_EOK;
    call_event_msg_t msg;

    while (1)
    {
        rst = call_event_get(&msg, RT_WAITING_FOREVER);
        if (rst == RT_EOK)
        {
            call_event_process(&msg);
        }
    }
}

int air720_call_init(struct at_device *device)
{
    RT_ASSERT(device);

    char name[RT_NAME_MAX] = {0};
    rt_snprintf(name, RT_NAME_MAX, "call_event");
    call_event = rt_event_create(name, RT_IPC_FLAG_FIFO);
    if (call_event == RT_NULL)
    {
        LOG_E("no memory for AT device(%s) call event create.", device->name);
        return -RT_ENOMEM;
    }

    rt_mq_init(&call_event_mq, "call_event_mq",
               &event_msg[0],
               sizeof(call_event_msg_t),
               sizeof(event_msg),
               RT_IPC_FLAG_FIFO);

    //create call listen thread
    rt_thread_t call_tid;
    call_tid = rt_thread_create("air720_call", air720_call_thread_entry, RT_NULL, 512, 18, 20);
    if (call_tid)
    {
        rt_thread_startup(call_tid);
    }
    /* register URC data execution function  */
    at_obj_set_urc_table(device->client, call_urc_table, sizeof(call_urc_table) / sizeof(call_urc_table[0]));

    return RT_EOK;
}

#endif /* AT_DEVICE_USING_air720 && AT_USING_SOCKET */
