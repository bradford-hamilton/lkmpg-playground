#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>

// Format every pr_* message with the current running function name
#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define DEV_MEM_SIZE 512

char device_buf[DEV_MEM_SIZE];
dev_t device_num;
struct cdev pcd_cdev;
struct class* pcd_class;
struct device* pcd_device;

static DEFINE_MUTEX(pcd_lock);

static int pcd_open(struct inode* inod, struct file* filp);
static int pcd_release(struct inode* inod, struct file* filp);
static ssize_t pcd_read(struct file* filp, char __user* buff, size_t count, loff_t* f_pos);
static ssize_t pcd_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos);
static loff_t pcd_lseek(struct file* filp, loff_t offset, int whence);

static const struct file_operations pcd_fops = {
  .open = pcd_open,
  .release = pcd_release,
  .read = pcd_read,
  .write = pcd_write,
  .owner = THIS_MODULE,
};

static int __init pcd_init(void)
{
  int ret;
  
  // Dynamically allocate a device number
  ret = alloc_chrdev_region(&device_num, 0, 1, "pcd_devices");
  if (ret < 0) {
    pr_err("Alloc chrdev failed\n");
    goto out;
  }

  pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(device_num), MINOR(device_num));

  // Initialize cdev structure with fops
  cdev_init(&pcd_cdev, &pcd_fops);

  // Register a device (cdev structure) with VFS
  pcd_cdev.owner = THIS_MODULE;
  ret = cdev_add(&pcd_cdev, device_num, 1);
  if (ret < 0) {
    pr_err("Cdev add failed\n");
    goto unreg_chrdev;
  }

  // Create device class under /sys/class/
  pcd_class = class_create(THIS_MODULE, "pcd_class");
  if (IS_ERR(pcd_class)) {
    pr_err("Class creation failed\n");
    ret = PTR_ERR(pcd_class);
    goto cdev_del;
  }

  // Populate with device information
  pcd_device = device_create(pcd_class, NULL, device_num, NULL, "pcd");
  if (IS_ERR(pcd_device)) {
    pr_err("Device create failed\n");
    ret = PTR_ERR(pcd_device);
    goto cls_destroy;
  }

  pr_info("Module init successful\n");

  return 0;

cls_destroy:
  class_destroy(pcd_class);
cdev_del:
  cdev_del(&pcd_cdev);
unreg_chrdev:
  unregister_chrdev_region(device_num, 1);
out:
  pr_err("Module insertion failed\n");
  return ret;
}

static void __exit pcd_exit(void)
{
  device_destroy(pcd_class, device_num);
  class_destroy(pcd_class);
  cdev_del(&pcd_cdev);
  unregister_chrdev_region(device_number, 1);

  pr_info("Module unloaded\n");
}

static loff_t pcd_lseek(struct file* filp, loff_t offset, int whence)
{
  pr_info("Lseek requested\n");
  pr_info("Current value of the file position = %lld\n", filp->f_pos);

  loff_t temp;

  switch(whence) {
    case SEEK_SET:
      if (offset > DEV_MEM_SIZE || offset < 0) {
        return -EINVAL;
      }
      filp->f_pos = offset;
      break;
    case SEEK_CUR:
      temp = filp->f_pos + offset;
      if (temp > DEV_MEM_SIZE || temp < 0) {
        return -EINVAL;
      }
      filp->f_pos += temp;
      break;
    case SEEK_END:
      temp = DEV_MEM_SIZE + offset;
      if (temp > DEV_MEM_SIZE || temp < 0) {
        return -EINVAL;
      }
      filp->f_pos = temp;
      break;
    default:
      return -EINVAL;
  }

  pr_info("New value of the file position = %lld\n", filp->f_pos);

  return filp->f_pos;
}

static ssize_t pcd_read(struct file* filp, char __user* buff, size_t count, loff_t* f_pos)
{
  if (mutex_lock_interruptible(&pcd_lock)) {
    return -EINTR;
  }

  pr_info("Read requested for %zu bytes\n", count);
  pr_info("Current file position %lld = \n", *f_pos);

  // Adjust the count
  if ((*f_pos + count) > DEV_MEM_SIZE) {
    count = DEV_MEM_SIZE - *f_pos;
  }

  if (copy_to_user(buff, &device_buf[*f_pos], count)) {
    return -EFAULT;
  }

  *f_pos += count;

  pr_info("Number of bytes successfully read = %zu\n", count);
  pr_info("Updated file position %lld = \n", *f_pos);

  mutex_unlock(&pcd_lock);

  // Number of bytes successfully read
  return count;
}

static ssize_t pcd_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos)
{
  if (mutex_lock_interruptible(&pcd_lock)) {
    return -EINTR;
  }

  pr_info("Read requested for %zu bytes\n", count);
  pr_info("Current file position %lld = \n", *f_pos);

  if ((*f_pos + count) > DEV_MEM_SIZE) {
    count = DEV_MEM_SIZE - *f_pos;
  }

  if (!count) {
    pr_err("No space left on the device\n");
    return -ENOMEM;
  }

  if (copy_from_user(&device_buf[*f_pos], buff, count)) {
    return -EFAULT;
  }

  *f_pos += count;

  pr_info("Number of bytes successfully written = %zu\n", count);
  pr_info("Updated file position %lld = \n", *f_pos);

  mutex_unlock(&pcd_lock);

  return count;
}

static int pcd_open(struct inode* inod, struct file* filp)
{
  pr_info("open successful\n");
  return 0;
}

static int pcd_release(struct inode* inod, struct file* filp)
{
  pr_info("release successful\n");
  return 0;
}

module_init(pcd_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("udemy ldd programming");
MODULE_DESCRIPTION("psuedo character driver handling one device");
MODULE_VERSION("1.0");
