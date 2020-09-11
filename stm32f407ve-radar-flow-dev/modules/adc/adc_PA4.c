#include <rtthread.h>
#include <rtdevice.h>

#define POWC_STACK_SIZE 512
#define POWC_THREAD_PRIORITY 29
#define POWC_TIMESLICE 20

#define ADC_DEV_NAME "adc1"    /* ADC 设备名称 */
#define ADC_DEV_CHANNEL 4      /* ADC 通道 */
#define REFER_VOLTAGE 330      /* 参考电压 3.3V,数据精度乘以100保留2位小数*/
#define CONVERT_BITS (1 << 12) /* 转换位数为12位 */

static int adc_vol()
{
    rt_adc_device_t adc_dev;
    rt_uint32_t value, vol;
    rt_err_t ret = RT_EOK;

    /* 查找设备 */
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
        return RT_ERROR;
    }

    /* 使能设备 */
    ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);

    /* 读取采样值 */
    value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
    rt_kprintf("the value is :%d \n", value);

    /* 转换为对应电压值 */
    vol = value * REFER_VOLTAGE / CONVERT_BITS;
    rt_kprintf("the voltage is :%d.%02d \n", vol / 100, vol % 100);

    /* 关闭通道 */
    ret = rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);

    return ret;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(adc_vol, adc voltage convert);

rt_uint32_t powc_vol;
static void _powc_thead_entry(void *param)
{
    rt_adc_device_t adc_dev;
    rt_uint32_t value;
    rt_err_t ret = RT_EOK;

    /* 查找设备 */
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
        return ;
    }

    /* 使能设备 */
    ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);

    while (1)
    {
        powc_vol = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
        //powc_vol = value * REFER_VOLTAGE / CONVERT_BITS;
        //rt_kprintf("the voltage is :%d.%02d \n", vol / 100, vol % 100);
        rt_thread_mdelay(60000);
    }
    ret = rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);
}

static int pow_capture_thread()
{
    rt_thread_t tid;

    tid = rt_thread_create("powc_thread",
                           _powc_thead_entry,
                           RT_NULL,
                           POWC_STACK_SIZE,
                           POWC_THREAD_PRIORITY,
                           POWC_TIMESLICE);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    return 0;
}
INIT_APP_EXPORT(pow_capture_thread);