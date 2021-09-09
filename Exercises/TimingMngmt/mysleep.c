#include <linux/module.h>
#include <linux/kernel.h>

static struct timer_list my_timer;

static void my_timer_callback(struct timer_list *t)
{
	printk("My Timer call-back called (%ld).\n", jiffies);
}

// TODO Implement my_sleep using a kernel timer
static int my_sleep(unsigned long msecs) 
{
    int ret = 0;
    return ret;
}

int init_module(void)
{
	// This would make insmod to sleep for 2 seconds
	my_sleep(2000);

	return 0;
}

void cleanup_module(void)
{
	int ret;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Kernel Timers Demo");
