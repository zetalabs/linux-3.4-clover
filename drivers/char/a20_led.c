/*
 *  a20 led driver
 *
 *  Based on a20_led.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/input/matrix_keypad.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

static script_item_u				led_val;
static script_item_value_type_e		led_type;
static script_item_u				led_val_2;
static script_item_value_type_e		led_type_2;

struct a20_gpio_led {	
	struct delayed_work work;	
};
struct a20_gpio_led *a20_led;

static void a20_led_scan(struct work_struct *work)
{	
	
	 __gpio_set_value(led_val.gpio.gpio, 0);
	 __gpio_set_value(led_val_2.gpio.gpio, 0);
	 
	 msleep(1000);
	  __gpio_set_value(led_val.gpio.gpio, 1);
	  __gpio_set_value(led_val_2.gpio.gpio, 1);
	  schedule_delayed_work(&a20_led->work,	msecs_to_jiffies(1000));	 
}
static int __devinit a20_led_gpio(void)
{
	int err = -EINVAL;

	led_type= script_get_item("led_para", "led_gpio1", &led_val);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != led_type) {
		printk("led_gpio1 type err!");
		gpio_free(led_val.gpio.gpio);
		return err;
	}
	
	if(0 != gpio_request(led_val.gpio.gpio, NULL)) {
		printk("ERROR: audio_det Gpio_request is failed\n");		
		return err;
	}

	if (0 != sw_gpio_setall_range(&led_val.gpio, 1)) {
		printk("led_gpio1 sw_gpio_setall_range  err!");
		return err;
	}	

	led_type_2= script_get_item("led_para", "led_gpio2", &led_val_2);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != led_type) {
		printk("led_gpio1 type err!");
		gpio_free(led_val_2.gpio.gpio);
		return err;
	}
	
	if(0 != gpio_request(led_val_2.gpio.gpio, NULL)) {
		printk("ERROR: audio_det Gpio_request is failed\n");		
		return err;
	}

	if (0 != sw_gpio_setall_range(&led_val_2.gpio, 1)) {
		printk("led_gpio1 sw_gpio_setall_range  err!");
		return err;
	}	

	return 0;
}

static int __devexit a20_led_remove(struct platform_device *pdev)
{
	return 0;
}

static int __devinit a20_led_probe(struct platform_device *pdev)
{
	int led_used;
	script_item_u	val;
	script_item_value_type_e  type;
	
	int err;
	
	
    printk("led_para!\n");
	type = script_get_item("led_para", "led_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("%s script_get_item \"led_para\" led_used = %d\n",
				__FUNCTION__, val.val);
		return -1;
	}
	led_used = val.val;
	printk("%s script_get_item \"led_para\" led_used = %d\n",
				__FUNCTION__, val.val);

	if(!led_used) {
		printk("%s led_used is not used in config,  led_used=%d\n", __FUNCTION__,led_used);
		return -1;
	}
	
	a20_led = kzalloc(sizeof(struct a20_gpio_led), GFP_KERNEL);
	
	INIT_DELAYED_WORK(&a20_led->work,a20_led_scan);	

	err = a20_led_gpio();
	if (err)
		return -1;
	

    schedule_delayed_work(&a20_led->work,msecs_to_jiffies(200));
	
	
	return 0;
}


struct platform_device a20_led_device = {
	.name		= "a20_led",	
};
static struct platform_driver a20_led_driver = {
	.probe		= a20_led_probe,
	.remove		= __devexit_p(a20_led_remove),
	.driver		= {
		.name	= "a20_led",
		.owner	= THIS_MODULE,
	},
};


static int __init a20_led_init(void)
{	
	
	if (platform_device_register(&a20_led_device)) {
        printk("%s: register gpio device failed\n", __func__);
    }
    if (platform_driver_register(&a20_led_driver)) {
        printk("%s: register gpio driver failed\n", __func__);
    }

	
	return 0;
}

static void __exit a20_led_exit(void)
{
	platform_driver_unregister(&a20_led_driver);	
}
module_init(a20_led_init);
module_exit(a20_led_exit);

MODULE_AUTHOR("rervision");
MODULE_DESCRIPTION("a20 led Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:a20-led");

