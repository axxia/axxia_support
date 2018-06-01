#ifndef __datalog_reader__
#define __datalog_reader__


//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */

#include <asm/uaccess.h> /* copy_from/to_user */



MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of datalog_reader.c functions */
int datalog_reader_open(struct inode *inode, struct file *filp);
int datalog_reader_release(struct inode *inode, struct file *filp);
ssize_t datalog_reader_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
void datalog_reader_exit(void);
int datalog_reader_init(void);














#endif /* __datalog_reader__ */
