/*
 * Axxia Pre-watchdog Datalog Reader
 *
 * Copyright (C) 2015 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* Allows reading contents of data log stored in reserved memory block 'datalogger_reserved' */
/* Device tree should include following definition (can change the addresses to suit your system */

/*
        reserved-memory {
                #address-cells = <2>;
                #size-cells = <2>;
                ranges;

               datalogger_reserved: datalogger@30000000 {
                        compatible = "datalogger";
                        reg = <0 0x30000000 0 0x00020000>;
                        no-map;
                };

 

        };



Create a device node in Linux /dev directory using:

mknod /dev/datalogger  c 60 0 


To use datalogger, use cat command to read contents to a file:

cat /dev/datalogger  > dump.txt

or to screen

cat /dev/datalogger



*/




#include "datalog_reader.h"

#include <linux/ioport.h>
#include <asm/io.h>

#include <linux/of.h>



/* Global vars */
void * data;
int datalog_reader_major = 60;

unsigned long long baseAddress = 0;
unsigned long long int memorySize = 0;

/* Structure that declares the usual file */
/* access functions */
struct file_operations datalog_reader_fops = {
  read: datalog_reader_read,
  open: datalog_reader_open,
  release: datalog_reader_release
};

/* Declaration of the init and exit functions */
module_init(datalog_reader_init);
module_exit(datalog_reader_exit);








int datalog_reader_open(struct inode *inode, struct file *filp) 
{

  //printk("< Open datalog_reader \n");
  /* Success */
  return 0;
}

int datalog_reader_release(struct inode *inode, struct file *filp) 
{
 
   // printk("Close datalog_reader \n");
  /* Success */
  return 0;
}



ssize_t datalog_reader_read(struct file *filp, char *buf, 
                    size_t count, loff_t *f_pos) 
{ 
 
  if (*f_pos >= memorySize)   /* If start position beyond end of datalog, return 0 bytes */
  {
 	return 0;
  }

  if ( (*f_pos + count) > memorySize) /* If request goes beyond end of datalog, truncate */
  {
  	count = memorySize - *f_pos;
  }
  
   	if ( copy_to_user(buf, (data + *f_pos), count)  != 0)
	{
		return -EFAULT;
	}

  *f_pos += count; 

  return count;

}






int datalog_reader_init(void) 
{
 
 struct device_node *node;
 struct device_node *dataloggerNode;	
 const unsigned long long * p;
 unsigned int size;

 int result; 
  

   /* Registering device */
  result = register_chrdev(datalog_reader_major, "datalog_reader", &datalog_reader_fops);
  if (result < 0) 
  {
    	printk("datalog_reader: cannot obtain major number %d\n", datalog_reader_major);
    	return result;
  }
 /* Search for datalogger memory area in dts */ 
  node = of_find_node_by_name(NULL, "reserved-memory");
  if (node ==NULL)
  {
	printk("Failed to find memory node in dts!\n");
	datalog_reader_exit();
	return 1;
  }
  
  dataloggerNode = of_get_child_by_name(node, "datalogger");
  if (dataloggerNode ==NULL)
  {
  	printk("Failed to find datalogger_reserved node in dts!\n");
	datalog_reader_exit();
	return 1;
  }

  p = of_get_property(dataloggerNode, "reg", &size);
  if  ( (p == NULL) | (size !=16) )
 {
	printk("Failed to find correctly formatted datalogger_reserved reg property in dts!\n");
	datalog_reader_exit();
	return 1;
  }	
	
  baseAddress = be64_to_cpu(p[0]);
  memorySize = be64_to_cpu(p[1]);
	

 /* Request datalogger mem range */
  if(request_mem_region(baseAddress, memorySize,  "datalog reader") == NULL)
  {
	printk("Failed to get datalogger memory region !!\n");
	datalog_reader_exit();
	return 1;
  }

   /* Get memory */
   data = ioremap(baseAddress, memorySize);
   if (data == NULL)
   {
	printk("I/O remap failed ! \n");
	datalog_reader_exit();
	return 1;
   }
 
  printk("Inserting datalog_reader module\n"); 
  return 0;

}

void datalog_reader_exit(void) 
{
/* Freeing the major number */

  unregister_chrdev(datalog_reader_major, "datalog_reader");

/* Free memory */
  release_mem_region(baseAddress, memorySize);
  iounmap(data);


  printk("Removing datalog_reader \n");
}




	
