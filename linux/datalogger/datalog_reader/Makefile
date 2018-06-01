obj-m := datalog_reader.o 
PWD := $(shell pwd) 

default:
	make EXTRA_CFLAGS+='-DDEBUG -DDEBUG_OUTPUT -g ' \
		-C $(KDIR) M=$(PWD) modules

clean:   
	make -C $(KDIR) M=$(PWD) clean
