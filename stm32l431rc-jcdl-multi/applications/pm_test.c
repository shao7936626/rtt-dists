/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     liang.shao   first version
 */
#if 1
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

/* defined the LED0 pin: PB13 */
//#define LED0_PIN    GET_PIN(B, 13)

#define PIN_KEY0 GET_PIN(C, 8) //1

static RTC_HandleTypeDef hrtc;
static RTC_TimeTypeDef curtime = {0};
static RTC_TimeTypeDef alarmtime = {0};
static RTC_AlarmTypeDef alarm = {0};

static rt_uint8_t sleep_mode = PM_SLEEP_MODE_DEEP; /* STOP 模式 */
//static rt_uint8_t sleep_mode = PM_SLEEP_MODE_STANDBY; /* 休眠模式 */

static rt_uint8_t run_mode = PM_RUN_MODE_NORMAL_SPEED;

static rt_uint32_t get_interval(void);
static void get_rtc_time(RTC_TimeTypeDef *time);
static rt_uint8_t mode_loop(void);

int pm_test(void)
{
    hrtc.Instance = RTC;

    /* set LED0 pin mode to output */
    // rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);

    // rt_pin_write(LED0_PIN, PIN_HIGH);
    rt_thread_mdelay(5000);

#ifdef RT_USING_PM
    /* 申请低功耗模式 */
    rt_pm_request(sleep_mode);
#endif

    get_rtc_time(&curtime);

    if (sleep_mode == PM_SLEEP_MODE_STANDBY)
    {
        /* 设置休眠，闹钟 20 秒后唤醒，简化版闹钟，只支持 1分钟内有效 */
        alarmtime.Hours = curtime.Hours;
        alarmtime.Minutes = curtime.Minutes;
        alarmtime.SubSeconds = curtime.SubSeconds;
        alarmtime.Seconds = curtime.Seconds + 59;
        if (alarmtime.Seconds >= 60)
        {
            alarmtime.Seconds -= 60;
            alarmtime.Minutes++;
            if (alarmtime.Minutes >= 60)
                alarmtime.Minutes -= 60;
        }

        alarm.Alarm = RTC_ALARM_A;
        alarm.AlarmTime = alarmtime;
        alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
        alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS;
        alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
        alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
        alarm.AlarmDateWeekDay = 0x1;

        /* 开启闹钟 */
        HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN);
    }

    while (1)
    {
        // rt_pin_write(LED0_PIN, PIN_LOW);

        /* 开始进入低功耗模式 */
        rt_thread_mdelay(5000);

        /* 退出低功耗模式 */
        rt_kprintf("Sleep %d ms\n", get_interval());

#ifdef RT_USING_PM
        rt_pm_request(PM_SLEEP_MODE_DEEP);
#endif

        // rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(5000);

        rt_kprintf("Wakeup %d ms\n", get_interval());

        /* 运行模式切换 */
       // rt_pm_run_enter(mode_loop());

#ifdef RT_USING_PM
        rt_pm_release(PM_SLEEP_MODE_DEEP);
#endif
    }

    return RT_EOK;
}

MSH_CMD_EXPORT(pm_test, pm test);

rt_uint32_t get_interval(void)
{
    rt_uint32_t seconds;

    rt_uint32_t last_seconds = curtime.Seconds;
    rt_uint32_t last_subseconds = curtime.SubSeconds;

    get_rtc_time(&curtime);

    if (curtime.Seconds < last_seconds)
        seconds = 60 + curtime.Seconds - last_seconds;
    else
        seconds = curtime.Seconds - last_seconds;

    return (rt_uint32_t)(seconds * 1000 + ((int32_t)last_subseconds - (int32_t)curtime.SubSeconds) * 1000 / (int32_t)(((RTC->PRER & RTC_PRER_PREDIV_S) >> RTC_PRER_PREDIV_S_Pos) + 1U));
}

void get_rtc_time(RTC_TimeTypeDef *time)
{
    rt_uint32_t st, datetmpreg;

    HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
    datetmpreg = RTC->DR;
    if (HAL_RCC_GetPCLK1Freq() < 32000U * 7U)
    {
        st = time->SubSeconds;
        HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
        datetmpreg = RTC->DR;

        if (st != time->SubSeconds)
        {
            HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
            datetmpreg = RTC->DR;
        }
    }
    (void)datetmpreg;
}

rt_uint8_t mode_loop(void)
{
    rt_uint8_t mode = 1;

    run_mode++;
    switch (run_mode)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        mode = run_mode;
        break;
    case 4:
        mode = 2;
        break;
    case 5:
        mode = 1;
        break;
    case 6:
        mode = run_mode = 0;
        break;
    }

    return mode;
}
#endif

