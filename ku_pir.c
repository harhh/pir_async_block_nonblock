#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>	//for kmalloc, kfree

#include <linux/irqflags.h>//for local_irq_save
#include <linux/uaccess.h>//for copy_to_user, copy_from_user
#include <linux/wait.h>//for wait_event, wake_up
#include <linux/sched.h>//for wait_event, wake_up, task
#include <linux/signal.h>//for signal

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/delay.h>

#include "ku_pir.h"

MODULE_LICENSE("Dual BSD/GPL");

static dev_t dev_num;
static struct cdev *cd_cdev;

static struct timer_list my_timer;

static int irq_num;

DECLARE_WAIT_QUEUE_HEAD(waitqueue_t);

struct test_list{
	struct list_head list;
	struct ku_pir_data data;
};
struct test_list my_list;
struct task_list task_list;
static unsigned long flags;

static int count_max_msg = 0;

static int gpio_value=0;
static struct test_list *insert_entry = 0;
static struct test_list *tmp =0;	
static struct list_head *pos = 0;

static long unsigned int rcnt_ts = 0;

static int userSigno = SIGUSR2;
struct task_struct *buftask =0;

struct task_list{
	struct list_head list;
	struct task_struct *buftask;
	long unsigned int timestamp;
};

static int enroll_recv_task(long unsigned int ts) {
	
	struct task_list *new_async = (struct task_list*)kmalloc(sizeof(struct task_list), GFP_KERNEL);
	new_async->timestamp = ts;
	new_async->buftask = current;	
	printk("%d..%d", ts, current);
	list_add_tail(&(new_async->list), &(task_list.list));
	return 0;
}
static int send2User(long unsigned int ts) {
	struct siginfo sinfo;
	struct pid *bufpid;
	long unsigned int data = ts;
	struct task_list *sig_tmp =0;	
	struct list_head *sig_pos = 0;	
	struct list_head *n = 0;

	sinfo.si_signo = userSigno;
	sinfo.si_errno = 0;
	sinfo.si_code = SI_MESGQ;
	sinfo.si_addr = 0;
	sinfo.si_value.sival_int = data;
	
	list_for_each_safe(sig_pos, n, &task_list.list) {
		sig_tmp = list_entry(sig_pos, struct task_list, list);
		if(sig_tmp->timestamp < ts) {
			bufpid = task_pid(sig_tmp->buftask);
			kill_pid_info_as_cred(userSigno, &sinfo, bufpid, 0, 0);
			list_del_init(sig_pos);
			kfree(sig_tmp);
		}
	}
	return 0;

}

static long ku_pir(struct file *file, unsigned int cmd, unsigned long arg){
	struct ku_pir_data *request;
	

	struct test_list *tmp_tmp = 0;
	struct list_head *tmp_pos = 0;

	struct test_list *_tmp =0;	
	struct list_head *_pos = 0;
	struct test_list *insert_data=0;
	request = (struct ku_pir_data*)kmalloc(sizeof(struct ku_pir_data), GFP_KERNEL);
	copy_from_user(request, arg, sizeof(struct ku_pir_data));
			
	switch ( cmd ) {

	case KU_NONBLOCKING:
		printk("nonblocking start\n");
		local_irq_save(flags);	
		if(count_max_msg == 0) {
			local_irq_restore(flags);	
			return -1;//fail
		}
		else {
			list_for_each(_pos, &my_list.list) {
				_tmp = list_entry(_pos, struct test_list, list);
				if(_tmp->data.timestamp >request->timestamp) {
					request->rf_flag = _tmp->data.rf_flag;
					request->timestamp = _tmp->data.timestamp;
					local_irq_restore(flags);	
					copy_to_user(arg, request, sizeof(struct ku_pir_data));
					kfree(request);
					return 0;//success
				}
			}
			local_irq_restore(flags);
			printk("fail nonblok");
			return -1;//fail
		}
		break;
	
	case KU_BLOCKING:
		printk("blocking start\n");
		local_irq_save(flags);
		if(count_max_msg == 0) {
			printk("data not exist\n");///
			local_irq_restore(flags);
			wait_event(waitqueue_t, rcnt_ts > request->timestamp); 
		}
		else {
			list_for_each(_pos, &my_list.list) {
				_tmp = list_entry(_pos, struct test_list, list);
				if(_tmp->data.timestamp > request->timestamp) {
					request->rf_flag = _tmp->data.rf_flag;
					request->timestamp = _tmp->data.timestamp;
					local_irq_restore(flags);	
					copy_to_user(arg, request, sizeof(struct ku_pir_data));
					kfree(request);//
					return 0;
				}
			}
				printk("data exist but request data is not!\n");///
				local_irq_restore(flags);
				wait_event(waitqueue_t, rcnt_ts > request->timestamp); 
		}
				printk("wakeup");
		local_irq_save(flags);
		list_for_each(_pos, &my_list.list) {
			_tmp = list_entry(_pos, struct test_list, list);
			if(_tmp->data.timestamp >request->timestamp) {
				printk("seraching");
				request->rf_flag = _tmp->data.rf_flag;
				request->timestamp = _tmp->data.timestamp;
				local_irq_restore(flags);	
				copy_to_user(arg, request, sizeof(struct ku_pir_data));
				kfree(request);//
				return 0;
			}
		}
		local_irq_restore(flags);
		break;

	case KU_ASYNCHRONOUS:
		printk("asynchronous start\n");
		local_irq_save(flags);	
		if(count_max_msg == 0) {
			enroll_recv_task(request->timestamp);
			local_irq_restore(flags);
			return -1;//fail and async
		}
		else {
			list_for_each(_pos, &my_list.list) {
				_tmp = list_entry(_pos, struct test_list, list);
				if(_tmp->data.timestamp >request->timestamp) {
					request->rf_flag = _tmp->data.rf_flag;
					request->timestamp = _tmp->data.timestamp;
					local_irq_restore(flags);	
					copy_to_user(arg, request, sizeof(struct ku_pir_data));
					kfree(request);//
					return 0;//success
				}
			}
			enroll_recv_task(request->timestamp);
			kfree(request);//
			local_irq_restore(flags);
			return -1;//fail and async
		}
		break;

	case KU_INSERT:
		insert_data = (struct test_list*)kmalloc(sizeof(struct test_list), GFP_KERNEL);
		insert_data->data.timestamp = request->timestamp;
		insert_data->data.rf_flag = request->rf_flag;
	
		local_irq_save(flags);
		if(count_max_msg < KUPIR_MAX_MSG) {
		list_add_tail(&(insert_data->list), &(my_list.list));
		count_max_msg++;
		}
		else {		
			_pos = my_list.list.next;
			_tmp = list_entry(_pos, struct test_list, list);
			list_del_init(_pos);
			kfree(_tmp);
			count_max_msg--;/////
			list_add_tail(&(insert_data->list), &(my_list.list));
			count_max_msg++;/////
		}
		_tmp = 0;	
		_pos = 0;
		kfree(request);//
		send2User(insert_data->data.timestamp);
		local_irq_restore(flags);
		wake_up(&waitqueue_t);	
		return 0;	
		break;

	default:
		return -1;
	}

	return 0;
}

static irqreturn_t sensor_isr(int irq, void* data){

	printk("interrupt \n");/////

	insert_entry = (struct test_list*)kmalloc(sizeof(struct test_list), GFP_KERNEL);	
	gpio_value = gpio_get_value(KUPIR_SENSOR);
	
	rcnt_ts = insert_entry->data.timestamp = jiffies;

	if(gpio_value == RISING) {
		insert_entry->data.rf_flag = RISING;
	}
	else if(gpio_value == FALLING) {		//else{ 	//if(gpio_value == FALLING) 
		insert_entry->data.rf_flag = FALLING;
	}
	local_irq_save(flags);
	if(count_max_msg < KUPIR_MAX_MSG) {
		list_add_tail(&(insert_entry->list), &(my_list.list));
		count_max_msg++;
	}
	else {		
		pos = my_list.list.next;
		tmp = list_entry(pos, struct test_list, list);
		list_del_init(pos);
		kfree(tmp);
		count_max_msg--;/////
		list_add_tail(&(insert_entry->list), &(my_list.list));
		count_max_msg++;/////
	}
	
	//.,,,test print...//
	pos = 0;
	list_for_each(pos, &my_list.list) {
		tmp = list_entry(pos, struct test_list, list);
		printk("r/f[%d], jiffies[%u]\n", tmp->data.rf_flag, tmp->data.timestamp);
	}
	printk("count[%d]\n", count_max_msg);
	//....test print...//
	//gpio_value = 0;
	insert_entry = 0;
	tmp = 0;	
	pos = 0;
	
	local_irq_restore(flags);
	
	wake_up(&waitqueue_t);
	
	send2User(rcnt_ts);
	
	return IRQ_HANDLED;
}


struct file_operations ku_pir_fops=
{
	.unlocked_ioctl = ku_pir,
	//.open = ku_pir_open,
	//.release = ku_pir_release,
};

static int __init sensor_init(void)
{
	int ret;

	printk("init start\n");

	gpio_request_one(KUPIR_SENSOR, GPIOF_IN, "sensor");

	irq_num = gpio_to_irq(KUPIR_SENSOR);
	
	ret = request_irq(irq_num, sensor_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "sensor_irq", NULL);	
	if (ret) {
		printk(KERN_ERR "Unable to reqeuest IRQ: %d\n", ret);
		free_irq(irq_num, NULL);

		return -1;
	}
	
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &ku_pir_fops);
	if (cdev_add(cd_cdev, dev_num, 1) < 0)
	{
		printk("Failed to add character device\n");
		return -1;
	}
	init_timer(&my_timer);
	INIT_LIST_HEAD(&my_list.list);
	INIT_LIST_HEAD(&task_list.list);
	return 0;
}

static void __exit sensor_exit(void)
{
	printk("exit start\n");
	
	del_timer(&my_timer);
	
	free_irq(irq_num, NULL);

	gpio_free(KUPIR_SENSOR);	
	
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);
}

module_init(sensor_init);
module_exit(sensor_exit);
