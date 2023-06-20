// Example that creates a read-only char device that prints
// how many times you have read from the dev file

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h> // For sprintf()
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/uaccess.h> // For get_user and put_user

#include <asm/errno.h>

// Protoypes - todo - move to header file
static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);
static ssize_t device_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t device_write(struct file*, const char __user*, size_t, loff_t*);

#define SUCCESS 0
#define DEVICE_NAME "chardev" // Dev name as it appears in /proc/devices
#define BUF_LEN 80            // Max length of the message from the device

// Global variables are declared as static and so are global within the file

// Major number assigned to our device driver
static int major;

enum {
  CDEV_NOT_USED = 0,
  CDEV_EXCLUSIVE_OPEN = 1,
};

// Used to prevent multiple access to device (race conditions)
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

// Message device will give when asked
static char msg[BUF_LEN+1];

static struct class* cls;

// Remaining members of the file_operations structure which we aren't
// explicitly assigning will be initialized to NULL by gcc
static struct file_operations chardev_fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release,
};

static int __init chardev_init(void)
{
  major = register_chrdev(0, DEVICE_NAME, &chardev_fops);

  if (major < 0) {
    pr_alert("Registering char device failed with %d\n", major);
    return major;
  }

  pr_info("Driver assigned major number %d.\n", major);

  cls = class_create(THIS_MODULE, DEVICE_NAME);

  device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

  pr_info("Device created on /dev/%s\n", DEVICE_NAME);

  return SUCCESS;
}

static void __exit chardev_exit(void)
{
  device_destroy(cls, MKDEV(major, 0));
  class_destroy(cls);
  unregister_chrdev(major, DEVICE_NAME);
}

// Called when a process tries to open the device file, like "sudo cat /dev/chardev"
static int device_open(struct inode* inode, struct file* file)
{
  static int counter = 0;

  if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
    return -EBUSY;
  }

  sprintf(msg, "Device opened %d times\n", counter++);

  try_module_get(THIS_MODULE);

  return SUCCESS;
}

// Called when a process closes the device file
static int device_release(struct inode* inode, struct file* file)
{
  // We're now ready for our next caller
  atomic_set(&already_open, CDEV_NOT_USED);

  // Decrement the usage count, or else once you opened
  // the file, you will never get rid of the module
  module_put(THIS_MODULE);

  return SUCCESS;
}

// Called when a process, which already opened the dev file, attempts to read from it.
static ssize_t device_read(struct file* filep, char __user *buffer, size_t length, loff_t* offset)
{
  // Number of bytes actually written to the buffer
  int bytes_read = 0;

  const char* msg_ptr = msg;

  // If we're at the end of the message
  if (!(msg_ptr + *offset)) {
    // Reset the offset
    *offset = 0;
    // Signify end of file
    return 0;
  }

  msg_ptr += *offset;

  // Put the data into the buffer
  while (length && *msg_ptr) {
    // The buffer is in the user data segment, not the kernel segment so "*" assignment won't work.
    // We have to use put_user which copies data from the kernel data segment to the user data segment. 
    put_user(*(msg_ptr++), buffer++);
    length--;
    bytes_read++;
  }

  *offset += bytes_read;

  // Most read functions return the number of bytes that were put into the buffer
  return bytes_read;
}

// Called when a process writes to dev file: echo "hi" > /dev/hello
static ssize_t device_write(struct file* filep, const char __user* buff, size_t length, loff_t* offset)
{
  pr_alert("Sorry, this operation is not supported.\n");
  return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
