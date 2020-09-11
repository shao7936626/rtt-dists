#include <rtthread.h>
#include <rtdevice.h>

#define HWTIMER_DEV_NAME   "timer3"     /* 定时器名称 */
rt_device_t hw_dev;                     /* 定时器设备句柄 */
rt_hwtimer_mode_t mode;         /* 定时器模式 */
rt_uint32_t freq = 10000;       /* 计数频率 */

/* 定时器超时回调函数 */
static rt_err_t timeout_cb(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("this is hwtimer timeout callback fucntion!\n");
    rt_kprintf("tick is :%d !\n", rt_tick_get());

    return 0;
}

static int hwtimer_sample(int argc, char *argv[])
{
    /* 查找定时器设备 */
    hw_dev = rt_device_find(HWTIMER_DEV_NAME);
    /* 以读写方式打开设备 */
    rt_device_open(hw_dev, RT_DEVICE_OFLAG_RDWR);
    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(hw_dev, timeout_cb);

    /* 设置计数频率(默认1Mhz或支持的最小计数频率) */
    rt_device_control(hw_dev, HWTIMER_CTRL_FREQ_SET, &freq);
    /* 设置模式为周期性定时器 */
    mode = HWTIMER_MODE_PERIOD;
    rt_device_control(hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
}
/* 导出到 msh 命令列表中 */
//MSH_CMD_EXPORT(hwtimer_sample, hwtimer sample);
INIT_APP_EXPORT(hwtimer_sample);