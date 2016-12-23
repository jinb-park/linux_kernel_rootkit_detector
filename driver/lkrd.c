/*
 * lkrd.c - lkrd driver
 *
 * Copyright (C) 2016 Jinbum Park <jinb.park7@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/unistd.h>
#include <linux/workqueue.h>
#include <linux/security.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/uaccess.h>

/* lkrd ioctl commands */
enum
{
	LKRD_GET_INSPECT_LKM = 0,
	LKRD_SEND_INSPECT_RESULT = 1,
};

/* lkrd inspect status */
enum
{
	LKRD_RESULT_NORMAL = 0,
	LKRD_RESULT_ROOTKIT = 1,
	LKRD_RESULT_NONE = 2,
};

#define LKRD_MODULE_LEN 256

struct lkrd_data
{
	char name[LKRD_MODULE_LEN];
	unsigned long result;
	struct completion comp;
	struct list_head list;
};

/* inspect list */
static LIST_HEAD(inspect_list);

/* spinlock for inspect list */
static DEFINE_SPINLOCK(list_lock);

/* mutex for lkrd inspection */
//static DEFINE_MUTEX(lkrd_mutex);

/* completion for lkrd daemon */
static DECLARE_COMPLETION(lkrd_comp);

static int lkrd_open(struct inode *inode, struct file *file)
{
    pr_info("lkrd_open\n");
    return 0;
}

static int lkrd_close(struct inode *inodep, struct file *filp)
{
    pr_info("lkrd_close\n");
    return 0;
}

/* Functions to process inspect data */
static void lkrd_get_first_data(struct lkrd_data **data, char *buf)
{
	spin_lock(&list_lock);
	*data = list_first_entry(&inspect_list, struct lkrd_data, list);
	strcpy(buf, (*data)->name);
	spin_unlock(&list_lock);
}

static void lkrd_update_result_first_data(unsigned long result)
{
	struct lkrd_data *data;

	spin_lock(&list_lock);
	data = list_first_entry(&inspect_list, struct lkrd_data, list);
	data->result = result;
	spin_unlock(&list_lock);

	complete(&data->comp);
}

static void lkrd_del_first_data(void)
{
	struct lkrd_data *data;

	spin_lock(&list_lock);
	data = list_first_entry(&inspect_list, struct lkrd_data, list);
	list_del(&data->list);
	spin_unlock(&list_lock);
}

static void lkrd_add_data(char *name)
{
	struct lkrd_data *data;

	data = (struct lkrd_data*)kzalloc(sizeof(struct lkrd_data), GFP_KERNEL);
	if(!data)
		return;

	strcpy(data->name, name);
	data->result = LKRD_RESULT_NONE;
	init_completion(&data->comp);

	spin_lock(&list_lock);
	list_add(&data->list, &inspect_list);
	spin_unlock(&list_lock);

	pr_info("lkrd_add_data : %s\n", data->name);

	//mutex_lock(&lkrd_mutex);
	complete(&lkrd_comp);
	//mutex_unlock(&lkrd_mutex);

	/* wait for inspect result */
	wait_for_completion(&data->comp);

	pr_info("lkrd_add_data result [%s] : [%ld]\n", data->name, data->result);
	lkrd_del_first_data();
}

static int lkrd_get_inspect_lkm(unsigned long arg)
{
	int ret;
	struct lkrd_data *data;
	char buf[LKRD_MODULE_LEN] = {0,};

	spin_lock(&list_lock);
	ret = list_empty(&inspect_list);
	spin_unlock(&list_lock);
	
	if(ret)
	{
		wait_for_completion(&lkrd_comp);
	}
	
	/* [ToDo] Apply mutex_lock with timeout */
	//mutex_lock(&lkrd_mutex);
	lkrd_get_first_data(&data, buf);
	pr_info("lkrd_get_inspect_lkm : %s", buf);
	copy_to_user((char*)arg, buf, sizeof(buf));

	return 0;
}

static int lkrd_send_inspect_result(unsigned long result)
{
	lkrd_update_result_first_data(result);

/*
	if(mutex_is_locked(&lkrd_mutex))
	{
		mutex_unlock(&lkrd_mutex);
	}*/
	return 0;
}

static long lkrd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long result;

	switch(cmd)
	{
	case LKRD_GET_INSPECT_LKM:
		//copy_to_user((char*)arg, "/home/jinb.park/test.ko", sizeof("/home/jinb.park/test.ko"));
		//return 0;
		return lkrd_get_inspect_lkm(arg);
	case LKRD_SEND_INSPECT_RESULT:
		copy_from_user(&result, (unsigned long*)arg, sizeof(result));
		pr_info("lkrd_ioctl - result : %ld\n", result);
		return lkrd_send_inspect_result(result);
	default:
		pr_info("unsupported command\n");
		break;
	}
	return 0;
}

static const struct file_operations lkrd_fops = 
{
    .owner = THIS_MODULE,
    .open = lkrd_open,
    .release = lkrd_close,
    	.unlocked_ioctl = lkrd_ioctl,
	.compat_ioctl = lkrd_ioctl,
};

struct miscdevice lkrd_device = 
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "lkrd_device",
    .fops = &lkrd_fops,
};

/* original init module */
static int (*orig_kernel_module_request)(char*) = NULL;
static int (*orig_kernel_module_from_file)(struct file*) = NULL;

static int lkrd_kernel_module_request(char *kmod_name)
{
	pr_info("lkrd_kernel_module_request : %s\n", kmod_name);

	if(orig_kernel_module_request)
		return orig_kernel_module_request(kmod_name);

	return 0;
}

static int lkrd_kernel_module_from_file(struct file *file)
{
	char *full_path = NULL, *buf = NULL;

	if(!file)
	{
		return 0;
	}

	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if(!buf)
		return -1;

	full_path = dentry_path_raw(file->f_path.dentry, buf, PAGE_SIZE);
	if(!full_path)
	{
		kfree(buf);
		return -1;
	}

	/* add data */
	lkrd_add_data(full_path);
	kfree(buf);

	if(orig_kernel_module_from_file)
		return orig_kernel_module_from_file(file);

	return 0;
}

static int lkrd_hook_insmod(void)
{
	unsigned long *sops_addr = (unsigned long *)kallsyms_lookup_name("security_ops");
	if(!sops_addr)
	{
		pr_err("sops_addr error\n");
		return -1;
	}

	struct security_operations *sops = (struct security_operations *)*sops_addr;

	if(!sops)
	{
		pr_err("sops NULL\n");
		return -1;
	}

	orig_kernel_module_request = sops->kernel_module_request;
	sops->kernel_module_request = lkrd_kernel_module_request;

	orig_kernel_module_from_file = sops->kernel_module_from_file;
	sops->kernel_module_from_file = lkrd_kernel_module_from_file;

	return 0;
}

static void lkrd_restore_insmod(void)
{
	struct security_operations *sops = *((unsigned long *)kallsyms_lookup_name("security_ops"));

	if(!sops || !orig_kernel_module_request || !orig_kernel_module_from_file)
		return;

	sops->kernel_module_request = orig_kernel_module_request;
	sops->kernel_module_from_file = orig_kernel_module_from_file;

	/* sleep for remaining readers of lkrd_hook_insmod */
	msleep(1000 * 4);
	return;
}

/* workqueue for insmod hooking */
static void lkrd_work_handler(struct work_struct *w);
static struct workqueue_struct *wq = NULL;
static DECLARE_DELAYED_WORK(lkrd_work, lkrd_work_handler);

static void lkrd_work_handler(struct work_struct *w)
{
	int ret;

	ret = lkrd_hook_insmod();
	if(ret)
	{
		pr_err("lkrd_hook_insmod error");
		return;
	}

	ret = misc_register(&lkrd_device);
	if(ret)
	{
		pr_err("misc_register error [%d]\n", ret);
		return;
	}

	pr_info("lkrd_work\n");
}

static int lkrd_init(void)
{
	unsigned long wtime = msecs_to_jiffies(2000);

	if(!wq)
		wq = create_singlethread_workqueue("lkrd");

	if(wq)
		queue_delayed_work(wq, &lkrd_work, wtime);

	pr_info("lkrd_init\n");
	return 0;
}

static void lkrd_exit(void)
{
	misc_deregister(&lkrd_device);
	lkrd_restore_insmod();
	if(wq)
		destroy_workqueue(wq);
	return;
}

module_init(lkrd_init);
module_exit(lkrd_exit);
MODULE_LICENSE("GPL");
