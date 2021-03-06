#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <mach/gpio.h>
#include <plat/mux.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include "am335_platform_data.h"

static int gpio_led;
static int gpio_key;
static int irq,fist_flag;
static struct timer_list key_timer;                      //定义一个定时器，用于按键消抖功能

static void key_timer_function(unsigned long data)       //在定时器处理函数中处理按键事件 
{  
        int ledval;  
        if(fist_flag==1){
		ledval =gpio_get_value(gpio_led);  
		if (ledval){
		     gpio_set_value(gpio_led,0);
		     printk("Get an IRQ! LED_USR is ON  \n");
		}else{
		     gpio_set_value(gpio_led,1);
		     printk("Get an IRQ! LED_USR is OFF \n");
		}
        }
} 

static irqreturn_t gpio_keys_isr(int irq, void *dev_id)
{
        fist_flag=1;
        mod_timer(&key_timer,jiffies+HZ/100);            //  更新超时时间，从新启动定时任务
	return IRQ_HANDLED;                              //  jiffies+HZ/1000 = 10ms  
}

static int my_probe(struct platform_device * pdev){

        printk("Driver is probe in platform bus\n");
        int error;
        unsigned long irqflags;

	struct am335_platform_data *pdata = pdev->dev.platform_data;
     
        gpio_led=pdata[0].gpio;
        error =gpio_direction_output(gpio_led,1);
        if(error !=0)
             printk("gpio_directiom(3_18 failed!\n)\n");
        gpio_set_value(gpio_led, 1);

        gpio_key=pdata[1].gpio;                         //获取按键对应的引脚号         
        error =gpio_direction_input(gpio_key);          //引脚设置为输入   
        if(error !=0)
             printk("gpio_directiom(0_22 failed!\n)\n");
        irq = gpio_to_irq(gpio_key);                    //引脚设置为中断模式 
	if (irq < 0) 
	    printk("Unable to get irq number for GPIO %d\n",gpio_key);
        else
            printk("Success to get irq=%d number for GPIO %d\n",irq,gpio_key);
	irqflags = IRQF_TRIGGER_RISING;                //设置中断标志（上升沿触发）
        error = request_irq(irq, gpio_keys_isr, irqflags, "key_led", NULL);  
	if (error < 0)                                 //注册中断（中断号、处理函数、中断标志等）
	    printk("Unable to claim irq %d,erno:%d\n" ,irq,error);
        else
            printk("Success to claim irq %d\n",irq);

	init_timer(&key_timer);                        //初始化定时器
	key_timer.function = key_timer_function;       //指定定时处理函数 
        add_timer(&key_timer);                         //启动定时器
        fist_flag=0;
        return 0;

}

static int my_remove(struct platform_device * dev){

          printk("Driver found device unpluged!\n");
          gpio_set_value(gpio_led, 0);
          free_irq(irq, NULL);
          printk("Success to free irq %d\n",irq);
          return 0;

}

static struct platform_driver my_driver ={
  .probe = my_probe,
  .remove = my_remove,
  .driver ={
       .owner =THIS_MODULE,
       .name  ="am335-demo",
  },
};

static int __init my_driver_init(void){

        return platform_driver_register(&my_driver);
}

static void __exit my_driver_exit(void){

       platform_driver_unregister(&my_driver);
}

module_init(my_driver_init);
module_exit(my_driver_exit);


MODULE_LICENSE("GPL");


