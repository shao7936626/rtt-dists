/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-5      SummerGift   first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtthread.h>
#include <board.h>

#define RT_APP_PART_ADDR 0x8008000

#define STM32_FLASH_START_ADRESS ((uint32_t)0x08000000)
#define STM32_FLASH_END_ADDRESS ((uint32_t)(STM32_FLASH_START_ADRESS + STM32_FLASH_SIZE))

extern const struct fal_flash_dev stm32_onchip_flash;
extern struct fal_flash_dev nor_flash0;

//#define SPI_FLASH_PARTITION     {FAL_PART_MAGIC_WROD, "filesystem", "W25Q128", 9 * 1024 * 1024, 16 * 1024 * 1024, 0},

/* flash device table */
#define FAL_FLASH_DEV_TABLE  \
    {                        \
        &stm32_onchip_flash, \
            &nor_flash0,     \
    }
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG

//#define SPI_FLASH_PARTITION                                                             \
    {                                                                                   \
        FAL_PART_MAGIC_WROD, "filesystem", "norflash0", 256 * 1024, 16 * 1792 * 1024, 0 \
    }

/* partition table */
#define FAL_PART_TABLE                                                                            \
    {                                                                                             \
        {FAL_PART_MAGIC_WROD, "bootloader", "onchip_flash", 0, 32 * 1024, 0},                     \
            {FAL_PART_MAGIC_WROD, "app", "onchip_flash", 32 * 1024, (512 - 32) * 1024, 0},        \
            {FAL_PART_MAGIC_WROD, "download", "norflash0", 0, 512 * 1024, 0},                     \
            {FAL_PART_MAGIC_WROD, "filesystem", "norflash0", 512 * 1024, (2048 - 512) * 1024, 0}, \
    }
#endif /* FAL_PART_HAS_TABLE_CFG */
#endif /* _FAL_CFG_H_ */
