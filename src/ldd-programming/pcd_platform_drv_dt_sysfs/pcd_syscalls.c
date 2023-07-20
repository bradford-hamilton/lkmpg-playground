#include "pcd_platform_driver_dt_sysfs.h"

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