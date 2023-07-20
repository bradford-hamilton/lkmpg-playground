#ifndef PCD_PLATFORM_DRIVER_DT_SYSFS_H
#define PCD_PLATFORM_DRIVER_DT_SYSFS_H

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <platform.h>

// Format every pr_* message with the current running function name
#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

static int pcd_open(struct inode* inod, struct file* filp);
static int pcd_release(struct inode* inod, struct file* filp);
static ssize_t pcd_read(struct file* filp, char __user* buff, size_t count, loff_t* f_pos);
static ssize_t pcd_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos);
static loff_t pcd_lseek(struct file* filp, loff_t offset, int whence);
static int pcd_platform_driver_probe(struct platform_device* pdev);
static int pcd_platform_driver_remove(struct platform_device* pdev);

enum pcdev_names {
  PCDEVA1X,
  PCDEVB1X,
  PCDEVC1X,
  PCDEVD1X,
};

struct device_config {
  int config_item1;
  int config_item2;
};

// Driver private data structure
static const struct pcdrv_private_data {
  int total_devices;
  dev_t device_num_base;
  struct class* pcd_class;
  struct device* pcd_dev;
};

// Device private data structure
static const struct pcdev_private_data {
  struct pcdev_platform_data pdata;
  char *buf;
  dev_t device_num;
  struct cdev chdev;
};

#endif // PCD_PLATFORM_DRIVER_DT_SYSFS_H
