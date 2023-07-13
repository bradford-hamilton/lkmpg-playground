#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
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

static const struct file_operations pcd_fops = {
  .open = pcd_open,
  .release = pcd_release,
  .read = pcd_read,
  .write = pcd_write,
  .owner = THIS_MODULE,
};

static const struct platform_driver pcd_platform_driver = {
  .probe = pcd_platform_driver_probe,
  .remove = pcd_platform_driver_remove,
  .driver = {
    .name = "pseudo-char-device",
  }
};

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
  platform_driver_register(&pcd_platform_driver);
  pr_info("Pcd platform driver loaded\n");

  return 0;
}

static void __exit pcd_platform_driver_exit(void)
{
  platform_driver_unregister(&pcd_platform_driver);
  pr_info("Pcd platform driver unloaded\n");
}

// Called when matched platform device is found
static int pcd_platform_driver_probe(struct platform_device* pdev)
{
  pr_info("Device detected\n");
  return 0;
}

// Called when the device is removed from the system
static int pcd_platform_driver_remove(struct platform_device* pdev)
{
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
