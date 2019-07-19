// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2003 INTEL Corporation

/*
 * Provides access to the "data log" saved when there is about to be
 * a watchdog timeout.  Setting up the datalogger is described
 * elsewhere.  As part of the datalogger setup, memory is reserved to
 * store registers etc.  The address of this memory is in the device
 * tree -- and this driver will read the device tree to determine the
 * address.  The device tree entry should look like the following.
 *
 *       reserved-memory { #address-cells = <2>; #size-cells = <2>;
 *       ranges;
 *
 *              datalogger_reserved: datalogger@30000000 { compatible =
 *                       "datalogger"; reg = <0 0x30000000 0
 *                       0x00020000>; no-map; };
 *
 *
 *
 *       };
 *
 * To recover the last values saved, do the following after the system
 * reboots.
 *
 * -1-
 * Create a device node in Linux /dev directory.
 *
 *     mknod /dev/datalogger c 60 0
 *
 * -2-
 * Load the module created in this directory.
 *
 *     insmod datalog_reader.ko
 *
 * -3-
 * Display the contents.
 *
 *     cat /dev/datalogger
 */

#include <linux/ioport.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/uaccess.h>

#include "datalog_reader.h"

/* Global Variables */

void * data;
int datalog_reader_major = 60;

unsigned long long baseAddress = 0;
unsigned long long int memorySize = 0;

/* File Access Function Structure */

struct file_operations datalog_reader_fops = {
read: datalog_reader_read,
open: datalog_reader_open,
release: datalog_reader_release
};

/*
  ------------------------------------------------------------------------------
  datalog_reader_open
*/

int
datalog_reader_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
  ------------------------------------------------------------------------------
  datalog_reader_release
*/

int
datalog_reader_release(struct inode *inode, struct file *filp)
{

	return 0;
}

/*
  ------------------------------------------------------------------------------
  datalog_reader_read
*/

ssize_t
datalog_reader_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{

	if (*f_pos >= memorySize)
		/* If the start position is beyond the end of the data
		 * log, return 0 bytes.
		 */
		return 0;

	if ((*f_pos + count) > memorySize)
		/* If the request goes beyond the end of the data log,
		 * truncate.
		 */
		count = memorySize - *f_pos;

   	if (copy_to_user(buf, (data + *f_pos), count))
		return -EFAULT;

	*f_pos += count;

	return count;
}

/*
  ------------------------------------------------------------------------------
  datalog_reader_init
*/

int
datalog_reader_init(void)
{
	struct device_node *node;
	struct device_node *dataloggerNode;
	const unsigned long long * p;
	unsigned int size;
	int result;

	/* Register Device */
	result = register_chrdev(datalog_reader_major,
				 "datalog_reader", &datalog_reader_fops);

	if (0 > result) {
		printk("Could not obtain major number %d!\n",
		       datalog_reader_major);

		return result;
	}

	/* Search for Data Log Memory Address in the Device Tree */
	node = of_find_node_by_name(NULL, "reserved-memory");
	if (!node) {
		printk("Failed to find memory node in dts!\n");
		datalog_reader_exit();

		return 1;
	}

	dataloggerNode = of_get_child_by_name(node, "datalogger");
	if (!dataloggerNode) {
		printk("Failed to find datalogger_reserved node in dts!\n");
		datalog_reader_exit();

		return 1;
	}

	p = of_get_property(dataloggerNode, "reg", &size);
	if (!p | (16 != size)) {
		printk("Failed to find correctly formatted datalogger_reserved reg property in dts!\n");
		datalog_reader_exit();

		return 1;
	}

	baseAddress = be64_to_cpu(p[0]);
	memorySize = be64_to_cpu(p[1]);

	/* Request Data Log Memory */
	if(!request_mem_region(baseAddress, memorySize, "datalog reader")) {
		printk("Failed to get datalogger memory region!\n");
		datalog_reader_exit();

		return 1;
	}

	/* Get Data Log Memory */
	data = ioremap(baseAddress, memorySize);
	if (!data) {
		printk("I/O remap failed!\n");
		datalog_reader_exit();

		return 1;
	}

	printk("Inserted datalog_reader Module\n");

	return 0;
}

module_init(datalog_reader_init);

/*
  ------------------------------------------------------------------------------
  datalog_reader_exit
*/

void datalog_reader_exit(void)
{
	/* Free the major number */
	unregister_chrdev(datalog_reader_major, "datalog_reader");

	/* Free memory */
	release_mem_region(baseAddress, memorySize);
	iounmap(data);

	printk("Removed datalog_reader Module\n");
}

module_exit(datalog_reader_exit);
