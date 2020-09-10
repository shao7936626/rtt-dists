/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-27     SummerGift   add spi flash port file
 */

#include <rtthread.h>
#include "spi_flash.h"
#include "spi_flash_sfud.h"
#include "drv_spi.h"
#include "fal.h"

#define FS_PARTITION_NAME "filesystem"

extern int dfs_mount(const char *device_name,
                     const char *path,
                     const char *filesystemtype,
                     unsigned long rwflag,
                     const void *data);
extern int dfs_mkfs(const char *fs_name, const char *device_name);

int lfs_load(void)
{

    struct rt_device *mtd_dev = RT_NULL;
    mtd_dev = fal_mtd_nor_device_create(FS_PARTITION_NAME);

    if (!mtd_dev)
    {
        rt_kprintf("Can't create a mtd device on '%s' partition.", FS_PARTITION_NAME);
    }
    else
    {
        /* 挂载 littlefs */
        if (dfs_mount(FS_PARTITION_NAME, "/", "lfs", 0, 0) == 0)
        {
            rt_kprintf("Filesystem initialized!\n");
        }
        else
        {
            /* 格式化文件系统 */
            dfs_mkfs("lfs", FS_PARTITION_NAME);
            /* 挂载 littlefs */
            if (dfs_mount("filesystem", "/", "lfs", 0, 0) == 0)
            {
                rt_kprintf("Filesystem initialized!\n");
            }
            else
            {
                rt_kprintf("Failed to initialize filesystem!\n");
            }
        }
    }
    return 0;
}
INIT_ENV_EXPORT(lfs_load);

#if defined(BSP_USING_SPI_FLASH)
static int rt_hw_spi_flash_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    rt_hw_spi_device_attach("spi3", "spi30", GPIOA, GPIO_PIN_15);

    if (RT_NULL == rt_sfud_flash_probe(FAL_USING_NOR_FLASH_DEV_NAME, "spi30"))
    {
        return -RT_ERROR;
    }else{
		 fal_init();
	}

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);
#endif

