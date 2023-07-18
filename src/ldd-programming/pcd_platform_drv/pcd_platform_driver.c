#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <platform.h>

// Format every pr_* message with the current running function name
#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define MAX_DEVICES 10

static int pcd_open(struct inode* inod, struct file* filp);
static int pcd_release(struct inode* inod, struct file* filp);
static ssize_t pcd_read(struct file* filp, char __user* buff, size_t count, loff_t* f_pos);
static ssize_t pcd_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos);
static loff_t pcd_lseek(struct file* filp, loff_t offset, int whence);
static int pcd_platform_driver_probe(struct platform_device* pdev);
static int pcd_platform_driver_remove(struct platform_device* pdev);

static const struct file_operations pcd_fops = {
  .open = pcd_open,
  .release = pcd_release,
  .read = pcd_read,
  .write = pcd_write,
  .owner = THIS_MODULE,
};

struct device_config {
  int config_item1;
  int config_item2;
};

enum pcdev_names {
  PCDEVA1X,
  PCDEVB1X,
  PCDEVC1X,
  PCDEVD1X,
}

struct device_config pcdev_config[] = {
  [PCDEVA1X] = {
    .config_item1 = 60,
    .config_item1 = 21,
  },
  [PCDEVB1X] = {
    .config_item1 = 50,
    .config_item1 = 22,
  },
  [PCDEVC1X] = {
    .config_item1 = 40,
    .config_item1 = 23,
  },
  [PCDEVD1X] = {
    .config_item1 = 30,
    .config_item1 = 24,
  },
};

static const struct platform_device_id pcdevs_ids[] = {
  { .name = "pcdev-A1x", .driver_data = PCDEVA1X },
  { .name = "pcdev-B1x", .driver_data = PCDEVB1X },
  { .name = "pcdev-C1x", .driver_data = PCDEVC1X },
  { .name = "pcdev-D1x", .driver_data = PCDEVD1X },
};

static const struct platform_driver pcd_platform_driver = {
  .probe = pcd_platform_driver_probe,
  .remove = pcd_platform_driver_remove,
  .id_table = pcdevs_ids,
  .driver = {
    .name = "pseudo-char-device", // Kernel will not use this anymore since we provided an id_table
  }
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

struct pcdrv_private_data pcdrv_data;

static int check_permission(int dev_perm, int access_mode)
{
  if (dev_perm == RDWR) {
    return 0;
  }
  // Read only
  if ((dev_perm == RDONLY) && ((acccess_mode & FMODE_READ) && !(access_mode & FMODE_WRITE))) {
    return 0;
  }
  // Write only
  if ((dev_perm == WRONLY) && ((acccess_mode & FMODE_WRITE) && !(access_mode & FMODE_READ))) {
    return 0;
  }

  return -EPERM;
}

static int __init pcd_platform_driver_init(void)
{
  int ret;

  ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, MAX_DEVICES, "pcdevs");
  if (ret < 0) {
    pr_err("Alloc chrdev region failed\n");
    return ret;
  }

  pcdrv_data.pcd_class = class_create(THIS_MODULE, "pcd_class");
  if (IS_ERR(pcdrv_data.pcd_class)) {
    pr_err("Class creation failed\n");
    ret = PTR_ERR(pcdrv_data.pcd_class);
    return ret;
  }

  platform_driver_register(&pcd_platform_driver);

  pr_info("Pcd platform driver loaded\n");

  return 0;
}

static void __exit pcd_platform_driver_exit(void)
{
  platform_driver_unregister(&pcd_platform_driver);
  class_destroy(pcdrv_data.pcd_class);
  unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);

  pr_info("Pcd platform driver unloaded\n");
}

// Called when matched platform device is found
static int pcd_platform_driver_probe(struct platform_device* pdev)
{
  pr_info("A device is detected\n");

  int ret;
  struct pcdev_private_data* dev_data;
  struct pcdev_platform_data* pdata;
  
  // Get the platform data
  pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);
  if (!pdata) {
    pr_info("No platform data available\n");
    return -EINVAL;
  }

  // Dynamically allocate memory for the device private data
  dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data), GFP_KERNEL);
  if (!dev_data) {
    pr_info("Cannot allocate memory\n");
    return -ENOMEM;
  }

  dev_data->pdata.size = pdata->size;
  dev_data->pdata.perm = pdata->perm;
  dev_data->pdata.serial_num = pdata->serial_num;

  // Save the device private data pointer in platform device structure
  dev_set_drvdata(pdev->dev, dev_data);

  pr_info("Device serial number = %s\n", dev_data->pdata.serial_num);
  pr_info("Device size = %d\n", dev_data->pdata.size);
  pr_info("Device permission = %d\n", dev_data->pdata.perm);

  pr_info("Config item 1 = %d\n", pcdev_config[pdev->id_entry->driver_data].config_item1);
  pr_info("Config item 2 = %d\n", pcdev_config[pdev->id_entry->driver_data].config_item2);

  // Dynamically allocate memory for the device buffer using
  // size information from the platform data
  dev_data->buf = devm_kzalloc(&pdev->dev, sizeof(dev_data->pdata.size), GFP_KERNEL);
  if (!dev_data->buf) {
    pr_info("Cannot allocate memory\n");
    return -ENOMEM;
  }

  // Get the device number
  dev_data->device_num = pcdrv_data.device_num_base + pdev->id;
  
  // Do cdev init and cdev add
  cdev_init(&dev_data->chdev, &pcd_fops);
  dev_data->chdev.owner = THIS_MODULE;

  ret = cdev_add(&dev_data->chdev, dev_data->device_num, 1);
  if (ret < 0) {
    pr_err("Cdev add failed\n");
    return ret;
  }

  // Create device file for the detected platform device
  pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, dev_data->device_num, NULL, "pcdev-%d", pdev->id);
  if (IS_ERR(pcdrv_data.pcd_dev)) {
    pr_err("Device create failed\n");
    ret = PTR_ERR(pcdrv_data.pcd_dev);
    cdev_del(&dev_data->cdev);
    return ret;
  }

  pcdrv_data.total_devices++;

  pr_info("Probe was successful\n");

  return 0;
}

// Called when the device is removed from the system
static int pcd_platform_driver_remove(struct platform_device* pdev)
{
  struct pcdev_private_data* dev_data = dev_get_drvdata(&pdev->dev);
  device_destroy(pcdrv_data.pcd_class, dev_data->device_num);
  cdev_del(&dev_data->cdev);
  pcdrv_data.total_devices--;

  pr_info("Device removed\n");

  return 0;
}

static loff_t pcd_lseek(struct file* filp, loff_t offset, int whence)
{
  return 0;
}

static ssize_t pcd_read(struct file* filp, char __user* buff, size_t count, loff_t* f_pos)
{
  return 0;
}

static ssize_t pcd_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos)
{
  return -ENOMEM;
}

static int pcd_open(struct inode* inod, struct file* filp)
{
  return 0;
}

static int pcd_release(struct inode* inod, struct file* filp)
{
  return 0;
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("udemy ldd programming");
MODULE_DESCRIPTION("psuedo character platform driver handling n devices");
MODULE_VERSION("1.0");
