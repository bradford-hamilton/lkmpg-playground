#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

// Format every pr_* message with the current running function name
#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define NO_OF_DEVICES 4

#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 1024
#define MEM_SIZE_MAX_PCDEV4 512

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

char device_buf_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buf_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buf_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buf_pcdev4[MEM_SIZE_MAX_PCDEV4];

// Device private data structure
static const struct pcdevice_priv_data {
  char *buf;
  unsigned size;
  const char *serial_num;
  int perm;
  struct cdev pcd_cdev;
};

// Driver's private data structure
static const struct pcdriver_priv_data {
  int total_devices;
  dev_t device_num;
  struct class* pcd_class;
  struct device* pcd_device;
  struct pcdevice_priv_data pcdevice_data[NO_OF_DEVICES];
};

struct pcdriver_priv_data pcdrv_data = {
  .total_devices = NO_OF_DEVICES,
  .pcdevice_data = {
    [0] = {
      .buf = device_buf_pcdev1,
      .size = MEM_SIZE_MAX_PCDEV1,
      .serial_num = "PCDEV1XYIOWEFJ",
      .perm = RDONLY,
    },
    [1] = {
      .buf = device_buf_pcdev2,
      .size = MEM_SIZE_MAX_PCDEV2,
      .serial_num = "PCDEV2XYIOWEFJ",
      .perm = WRONLY,
    },
    [2] = {
      .buf = device_buf_pcdev3,
      .size = MEM_SIZE_MAX_PCDEV3,
      .serial_num = "PCDEV3XYIOWEFJ",
      .perm = RDWR,
    },
    [3] = {
      .buf = device_buf_pcdev4,
      .size = MEM_SIZE_MAX_PCDEV4,
      .serial_num = "PCDEV4XYIOWEFJ",
      .perm = RDWR,
    },
  },
}

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
  int i;
  
  // Dynamically allocate a chrdev region using device num as the base (ex. 127:0) of all your devices nums.
  ret = alloc_chrdev_region(pcdrv_data.device_num, 0, NO_OF_DEVICES, "pcd_devices");
  if (ret < 0) {
    pr_err("Alloc chrdev failed\n");
    goto out;
  }

  // Create device class under /sys/class/
  pcdrv_data.pcd_class = class_create(THIS_MODULE, "pcd_class");
  if (IS_ERR(pcdrv_data.pcd_class)) {
    pr_err("Class creation failed\n");
    ret = PTR_ERR(pcdrv_data.pcd_class);
    goto unreg_chrdev;
  }

  for (i = 0; i < NO_OF_DEVICES; i++) {
    pr_info(
      "Device number <major>:<minor> = %d:%d\n",
      MAJOR(pcdrv_data.device_num + i),
      MINOR(pcdrv_data.device_num + i),
    );

    // Initialize cdev structure with fops
    cdev_init(&pcdrv_data.pcdevice_data[i].cdev, &pcd_fops);

    // Register a device (cdev structure) with VFS
    pcdrv_data.pcdevice_data[i].cdev.owner = THIS_MODULE;
    ret = cdev_add(&pcdrv_data.pcdevice_data[i].cdev, pcdrv_data.device_num + i, 1);
    if (ret < 0) {
      pr_err("Cdev add failed\n");
      goto cdev_destroy;
    }

    // Populate with device information
    pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, pcdrv_data.device_num + i, NULL, "pcdev-%d", i + 1);
    if (IS_ERR(pcdrv_data.pcd_device)) {
      pr_err("Device create failed\n");
      ret = PTR_ERR(pcdrv_data.pcd_device);
      goto cls_destroy;
    }
  }

  pr_info("Module init successful\n");

  return 0;

cdev_destroy:
cls_destroy:
  for (; i >= 0; i--) {
    device_destroy(pcdrv_data.pcd_class, pcdrv_data.device_num + i);
    cdev_del(&pcdrv_data.pcdevice_data[i].cdev);
  }
  class_destroy(pcdrv_data.pcd_class);

unreg_chrdev:
  unregister_chrdev_region(pcdrv_data.device_num, NO_OF_DEVICES);

out:
  pr_err("Module insertion failed\n");
  return ret;
}

static void __exit pcd_exit(void)
{
  int i;
  for (i = 0; i < NO_OF_DEVICES; i++) {
    device_destroy(pcdrv_data.pcd_class, pcdrv_data.device_num + i);
    cdev_del(&pcdrv_data.pcdevice_data[i].cdev);
  }
  class_destroy(pcdrv_data.pcd_class);
  unregister_chrdev_region(pcdrv_data.device_num, NO_OF_DEVICES);

  pr_info("Module unloaded\n");
}

static loff_t pcd_lseek(struct file* filp, loff_t offset, int whence)
{
  pr_info("Lseek requested\n");
  pr_info("Current value of the file position = %lld\n", filp->f_pos);

  struct pcdevice_priv_data* pcdev_data = (struct pcdevice_priv_data*)filp->private_data;
  int max_size = pcdev_data->size;
  loff_t temp;

  switch(whence) {
    case SEEK_SET:
      if (offset > max_size || offset < 0) {
        return -EINVAL;
      }
      filp->f_pos = offset;
      break;
    case SEEK_CUR:
      temp = filp->f_pos + offset;
      if (temp > max_size || temp < 0) {
        return -EINVAL;
      }
      filp->f_pos += temp;
      break;
    case SEEK_END:
      temp = max_size + offset;
      if (temp > max_size || temp < 0) {
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
  pr_info("Read requested for %zu bytes\n", count);
  pr_info("Current file position %lld = \n", *f_pos);

  struct pcdevice_priv_data* pcdev_data = (struct pcdevice_priv_data*)filp->private_data;
  int max_size = pcdev_data->size;

  if ((*f_pos + count) > max_size) {
    count = max_size - *f_pos;
  }

  if (copy_to_user(buff, pcdev_data->buf + (*f_pos), count)) {
    return -EFAULT;
  }

  *f_pos += count;

  pr_info("Number of bytes successfully read = %zu\n", count);
  pr_info("Updated file position %lld = \n", *f_pos);

  // Number of bytes successfully read
  return count;
}

static ssize_t pcd_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos)
{
  pr_info("Read requested for %zu bytes\n", count);
  pr_info("Current file position %lld = \n", *f_pos);

  struct pcdevice_priv_data* pcdev_data = (struct pcdevice_priv_data*)filp->private_data;
  int max_size = pcdev_data->size;

  if ((*f_pos + count) > max_size) {
    count = max_size - *f_pos;
  }

  if (!count) {
    pr_err("No space left on the device\n");
    return -ENOMEM;
  }

  if (copy_from_user(pcdev_data->buf + (*f_pos), buff, count)) {
    return -EFAULT;
  }

  *f_pos += count;

  pr_info("Number of bytes successfully written = %zu\n", count);
  pr_info("Updated file position %lld = \n", *f_pos);

  return count;
}

static int check_permission(int dev_perm, int access_mode)
{
  if (dev_perm == RDWR) {
    return 0;
  }
  // Read only
  if ((dev_perm == RDONLY) && ((access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE))) {
    return 0;
  }
  // Write only
  if ((dev_perm == WRONLY) && ((access_mode & FMODE_WRITE) && !(access_mode & FMODE_READ))) {
    return 0;
  }

  return -EPERM;
}

static int pcd_open(struct inode* inod, struct file* filp)
{
  int ret;
  int minor_num;

  // Get the pcdevice_priv_data structure from the inode
  struct pcdevice_priv_data* pcdev_data;
  pcdev_data = CONTAINER_OF(inod->i_cdev, struct pcdevice_priv_data, pcd_cdev);
  
  // Supply device private data to other methods of the driver
  filp->private_data = pcdev_data;

  // Find out on which device file open was attempted by the user space
  minor_num = MINOR(inod->i_rdev);
  pr_info("Minor access = %d\n", minor_num);

  // Check permission
  ret = check_permission(pcdev_data->perm, filp->f_mode);

  (!ret) ? pr_info("open successful\n") : pr_info("open was unsuccessful\n");
  
  return ret;
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
MODULE_DESCRIPTION("psuedo character driver handling n devices");
MODULE_VERSION("1.0");
