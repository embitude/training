#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *thread_st;

static int thread_fn(void *unused)
{
	while (!kthread_should_stop()) 
	{
		printk(KERN_INFO "Thread Running\n");
		ssleep(5);
	}
	printk(KERN_INFO "Thread Stopping\n");
    do_exit(0);
}


static int __init init_thread(void)
{
	printk(KERN_INFO "Creating Thread\n");
	thread_st = kthread_create(thread_fn, NULL, "mythread");
	if (thread_st)
		wake_up_process(thread_st);
	else
		printk(KERN_INFO "Thread creation failed\n");

	return 0;
}

static void __exit cleanup_thread(void)
{
	printk("Cleaning Up\n");
	if (thread_st != NULL)
	{
		kthread_stop(thread_st);
		printk(KERN_INFO "Thread stopped");
	}
}

module_init(init_thread);
module_exit(cleanup_thread);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Thread Creation Demo");
