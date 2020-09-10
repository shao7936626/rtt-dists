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

#define S_VER "1.0.2"
#define H_VER "1.0.0"

int main(void)
{
	rt_kprintf("the software_version is %s\nthe hardware_version is %s\n",S_VER,H_VER);
    return RT_EOK;
}
