obj-m += pcd_sysfs.o

pcd_sysfs-objs += pcd_platform_driver_dt_sysfs.o pcd_syscalls.o

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
