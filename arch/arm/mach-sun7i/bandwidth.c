/*
 * arch/arch/mach-sun7i/bandwidth.c
 *
 * (C) Copyright 2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <mach/sys_config.h>

static ssize_t get_dram_config(char *key, char *buf)
{
    script_item_value_type_e type;
    script_item_u item;
    ssize_t outsize;

    type = script_get_item("dram_para", key, &item);
    if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
        pr_err("%s: script_get_item failed\n", __func__);
        return -EINVAL;
    } else {
        sprintf(buf, "%d", item.val);
        outsize = strlen(buf);
    }

    return outsize;
}

ssize_t dram_clk_show(struct class *class, struct class_attribute *attr, char *buf)
{
    return get_dram_config("dram_clk", buf);
}

ssize_t dram_bus_width_show(struct class *class, struct class_attribute *attr, char *buf)
{
    return get_dram_config("dram_bus_width", buf);
}

static struct class_attribute bandwidth_class_attrs[] = {
    __ATTR(dram_clk, 0444, dram_clk_show, NULL),
    __ATTR(dram_bus_width, 0444, dram_bus_width_show, NULL),
    __ATTR_NULL,
};

static struct class bandwidth_class = {
    .name = "bandwidth",
    .owner = THIS_MODULE,
    .class_attrs = bandwidth_class_attrs,
};

static int __init bandwidth_sysfs_init(void)
{
    int status;

    /* create /sys/class/bandwidth */
    status = class_register(&bandwidth_class);
    if (status < 0) {
        pr_err("%s: status = %d\n", __func__, status);
    } else {
        pr_info("%s success\n", __func__);
    }

    return status;
}

postcore_initcall(bandwidth_sysfs_init);
