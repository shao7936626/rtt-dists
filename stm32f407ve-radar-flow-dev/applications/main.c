/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-18     liang.shao    first version
 */
#include "main.h"

// #define OTA_TEST

#define S_VER "1.0.7"
#define H_VER "1.0.2"

#define PIN_RUN GET_PIN(A, 11)

rt_uint8_t internet_up = 0;

int main(void)
{

	rt_kprintf("the software_version is %s\nthe hardware_version is %s\n", S_VER, H_VER);
#ifdef OTA_TEST
	rt_thread_mdelay(60000);
	extern void http_ota(uint8_t argc, char **argv);
	http_ota(1, RT_NULL);
#endif
	rt_pin_mode(PIN_RUN, PIN_MODE_OUTPUT);
	
	while (1)
	{
		rt_pin_write(PIN_RUN, PIN_HIGH);
		rt_thread_mdelay(1000);
		rt_pin_write(PIN_RUN, PIN_LOW);
		rt_thread_mdelay(1000);
	}

	return RT_EOK;
}
