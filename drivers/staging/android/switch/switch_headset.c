/*
 *  drivers/switch/switch_gpio.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 * xgc change for A20 2014.07
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <linux/switch.h>
#include <mach/sys_config.h>
#include <mach/system.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>


#undef SWITCH_DBG
#if (0)
    #define SWITCH_DBG(format,args...)  printk("[SWITCH] "format,##args)    
#else
    #define SWITCH_DBG(...)    
#endif

#define TP_CTRL0 			(0x0)
#define TP_CTRL1 			(0x4)
#define TP_INT_FIFO_CTR		(0x10)
#define TP_INT_FIFO_STATUS	(0x14)
#define TP_DATA				(0x24)
#define TP_CTRL_CLK_PARA  	(0x00a6002f)

#define FUNCTION_NAME "h2w"
#define TIMER_CIRCLE 50
//int  pa_dde=(volatile int *)0xf1c22c28;
static int gpio_earphone_switch = 0;
static void __iomem *tpadc_base;
static int count_state;
static int switch_used = 0;
static script_item_u    spk_mute_ctl;
static	int start_val=-2;
static	int end_val=-1;
static struct gpio_hdle {
	script_item_u	val;
	script_item_value_type_e  type;	
}audio_gpio_hdle;


struct gpio_switch_data {
	struct switch_dev sdev;
	int pio_hdle;	
	int state;
	int pre_state;
	unsigned int gpio_pa_shutdown;
	struct work_struct work;
	struct timer_list timer;
};

static void earphone_hook_handle(unsigned long data)
{	 	
	struct gpio_switch_data	*switch_data = (struct gpio_switch_data *)data;	

	unsigned int  reg_val;
	volatile unsigned int key_val;

	SWITCH_DBG("enter:%s,line:%d\n", __func__, __LINE__);

	start_val = __gpio_get_value(audio_gpio_hdle.val.gpio.gpio);
	SWITCH_DBG("%s,start_val=%d,end_val=%d\n", __func__, start_val, end_val);

	if(start_val!=end_val)
	{
		end_val = start_val;
		if(start_val)	
		{
			switch_data->state = 2;			   	
		}
		else
		{		
			switch_data->state = 0;
		}		  
		schedule_work(&switch_data->work);
	}

	mod_timer(&switch_data->timer, jiffies + msecs_to_jiffies(200));	
}

static void earphone_switch_work(struct work_struct *work)
{
	struct gpio_switch_data	*data =
		container_of(work, struct gpio_switch_data, work);
	SWITCH_DBG("%s,line:%d, data->state:%d\n", __func__, __LINE__, data->state);
	switch_set_state(&data->sdev, data->state);
}
static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{	
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);
	
	return sprintf(buf, "%d\n", switch_data->state);	
}

static ssize_t print_headset_name(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);
	
	return sprintf(buf, "%s\n", switch_data->sdev.name);
}

static int gpio_switch_resume(struct platform_device *pdev)
{	
	int temp;
	tpadc_base = (void __iomem *)SW_VA_TP_IO_BASE;	
	writel(TP_CTRL_CLK_PARA, tpadc_base + TP_CTRL0);	
	temp = readl(tpadc_base + TP_CTRL1);
	temp |= ((1<<3) | (0<<4)); //tp mode function disable and select ADC module.
	temp |=0x1;//X2 channel	
	writel(temp, tpadc_base + TP_CTRL1);

	temp = readl(tpadc_base + TP_INT_FIFO_CTR);
	temp |= (1<<16); //TP FIFO Data available IRQ Enable
	temp |= (0x3<<8); //4次采样数据的平均值 (3+1)
	writel(temp, tpadc_base + TP_INT_FIFO_CTR);
	
	temp = readl(tpadc_base + TP_CTRL1);
	temp |= (1<<4);	
	writel(temp, tpadc_base + TP_CTRL1);

	return 0;
}
static int gpio_switch_probe(struct platform_device *pdev)
{
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_switch_data *switch_data;
	int ret = 0;
	int temp;
	
	SWITCH_DBG("enter:%s,line:%d\n", __func__, __LINE__);
	
	if (!pdata) {
		return -EBUSY;
	}

	tpadc_base = (void __iomem *)SW_VA_TP_IO_BASE;	
	writel(TP_CTRL_CLK_PARA, tpadc_base + TP_CTRL0);	
	temp = readl(tpadc_base + TP_CTRL1);
	temp |= ((1<<3) | (0<<4)); //tp mode function disable and select ADC module.
	temp |=0x1;//X2 channel	
	writel(temp, tpadc_base + TP_CTRL1);

	temp = readl(tpadc_base + TP_INT_FIFO_CTR);
	temp |= (1<<16); //TP FIFO Data available IRQ Enable
	temp |= (0x3<<8); //4次采样数据的平均值 (3+1)
	writel(temp, tpadc_base + TP_INT_FIFO_CTR);
	
	temp = readl(tpadc_base + TP_CTRL1);
	temp |= (1<<4);	
	writel(temp, tpadc_base + TP_CTRL1);	
	
	SWITCH_DBG("TP adc 0xf1c25000 is:%x\n", *(volatile int *)0xf1c25000);
	SWITCH_DBG("TP adc 0xf1c25004 is:%x\n", *(volatile int *)0xf1c25004);
	SWITCH_DBG("TP adc 0xf1c25008 is:%x\n", *(volatile int *)0xf1c25008);
	SWITCH_DBG("TP adc 0xf1c2500c is:%x\n", *(volatile int *)0xf1c2500c);
	SWITCH_DBG("TP adc 0xf1c25010 is:%x\n", *(volatile int *)0xf1c25010);
	SWITCH_DBG("TP adc 0xf1c25014 is:%x\n", *(volatile int *)0xf1c25014);
	SWITCH_DBG("TP adc 0xf1c25018 is:%x\n", *(volatile int *)0xf1c25018);
	SWITCH_DBG("TP adc 0xf1c2501c is:%x\n", *(volatile int *)0xf1c2501c);
	SWITCH_DBG("TP adc 0xf1c25020 is:%x\n", *(volatile int *)0xf1c25020);
	
	switch_data = kzalloc(sizeof(struct gpio_switch_data), GFP_KERNEL);
	if (!switch_data) {
		return -ENOMEM;
	}
	//gpio_earphone_switch = gpio_request_ex("audio_para", "audio_earphone_ctrl");
//	if(!gpio_earphone_switch) {
//		printk("earphone request gpio fail!\n");
//		ret = gpio_earphone_switch;
//		goto err_gpio_request;
//	}
//	
	switch_data->sdev.state = 0;
	switch_data->pre_state = -1;
	switch_data->sdev.name = pdata->name;	
	switch_data->pio_hdle = gpio_earphone_switch;
	switch_data->sdev.print_name = print_headset_name;
	switch_data->sdev.print_state = switch_gpio_print_state;
	INIT_WORK(&switch_data->work, earphone_switch_work);

    ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}

#if 0		
	setup_timer(&switch_data->timer, earphone_hook_handle, (unsigned long)switch_data);
	mod_timer(&switch_data->timer, jiffies + HZ/2);
#endif

#if 1
	init_timer(&switch_data->timer);
	switch_data->timer.expires = jiffies + 1 * HZ;
	switch_data->timer.function = &earphone_hook_handle;
	switch_data->timer.data = (unsigned long)switch_data;
	add_timer(&switch_data->timer);
#endif
	return 0;

err_switch_dev_register:
		//gpio_release(switch_data->pio_hdle, 1);
err_gpio_request:
		kfree(switch_data);

	return ret;
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data = platform_get_drvdata(pdev);
	
	SWITCH_DBG("enter:%s,line:%d\n", __func__, __LINE__);
	
	//gpio_release(switch_data->pio_hdle, 1);
	//gpio_release(gpio_earphone_switch, 1);
	SWITCH_DBG("enter:%s,line:%d\n", __func__, __LINE__);
	del_timer(&switch_data->timer);	
	SWITCH_DBG("enter:%s,line:%d\n", __func__, __LINE__);
    switch_dev_unregister(&switch_data->sdev);
    SWITCH_DBG("enter:%s,line:%d\n", __func__, __LINE__);
	kfree(switch_data);
	SWITCH_DBG("enter:%s,line:%d\n", __func__, __LINE__);
	return 0;
}

static struct platform_driver gpio_switch_driver = {
	.probe		= gpio_switch_probe,
	.remove		= __devexit_p(gpio_switch_remove),
	.resume		= gpio_switch_resume,
	.driver		= {
		.name	= "switch-gpio",
		.owner	= THIS_MODULE,
	},
};

static struct gpio_switch_platform_data headset_switch_data = { 
    .name = "h2w",
};

static struct platform_device gpio_switch_device = { 
    .name = "switch-gpio",
    .dev = { 
            .platform_data = &headset_switch_data,
    }   
};

static int __init gpio_switch_init(void)
{
	int ret = 0;	
	static script_item_u   val;	
	script_item_value_type_e  type;	/* 获取audio_used值 */	
	type = script_get_item("switch_para", "switch_used", &val);	
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type){		
		printk("type err!");	}	
		pr_info("value is %d\n", val.val);    
		switch_used=val.val;
    if (switch_used) {
/* direct access to the hp_det pin */

	audio_gpio_hdle.type = script_get_item("switch_para", "hp_det", &(audio_gpio_hdle.val));
	
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != audio_gpio_hdle.type)
		printk(KERN_ERR "ERROR: Audio gpio type err! \n");
	
	printk("audio_gpio_hdle value is: gpio %d, mul_sel %d, pull %d, drv_level %d, data %d\n", 
		audio_gpio_hdle.val.gpio.gpio, audio_gpio_hdle.val.gpio.mul_sel, audio_gpio_hdle.val.gpio.pull,  
		audio_gpio_hdle.val.gpio.drv_level, audio_gpio_hdle.val.gpio.data);
	 
	if(0 != gpio_request(audio_gpio_hdle.val.gpio.gpio, NULL)) {	
		printk(KERN_ERR "ERROR: Audio Gpio_request is failed\n");
	}

	
	if (0 != sw_gpio_setall_range(&audio_gpio_hdle.val.gpio, 1)) {
		printk(KERN_ERR "ERROR: Audio gpio set err!");
		//goto end;
	}
		ret = platform_device_register(&gpio_switch_device);
		if (ret == 0) {
			ret = platform_driver_register(&gpio_switch_driver);
		}
	} else {
		SWITCH_DBG("[switch]switch headset cannot find any using configuration for controllers, return directly!\n");
		return 0;
	}
	return ret;
}

static void __exit gpio_switch_exit(void)
{
	SWITCH_DBG("enter:%s,line:%d\n", __func__, __LINE__);
	if (switch_used) {
		switch_used = 0;
		platform_driver_unregister(&gpio_switch_driver);
		platform_device_unregister(&gpio_switch_device);
	}
}
module_init(gpio_switch_init);
module_exit(gpio_switch_exit);

MODULE_AUTHOR("Mike Lockwood <lockwood@android.com>");
MODULE_DESCRIPTION("GPIO Switch driver");
MODULE_LICENSE("GPL");
