/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define VERSION "1.0.1"


int main(void)
{

	rt_kprintf("the iot-dev version is %s\n\n", VERSION);
	return RT_EOK;
}
