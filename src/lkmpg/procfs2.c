// Example of creating a "file" in /proc

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h> // Necessary because we use the proc fs
#include <linux/uaccess.h> // For copy_from_user
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "bufferlk"

static struct proc_dir_entry* our_proc_file;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buf_size = 0;

// Called then the /proc file is read
static ssize_t procfile_read(
  struct file* file_ptr,
  char __user* buffer,
  size_t buffer_length,
  loff_t* offset
) {
  char s[20] = "Hello Foo Bar Baz!\n";
  int len = sizeof(s);
  ssize_t ret = len;

  if (*offset >= len || copy_to_user(buffer, s, len)) {
    pr_info("copy_to_user failed\n");
    ret = 0;
  } else {
    pr_info("procfile read %s\n", file_ptr->f_path.dentry->d_name.name);
    *offset += len;
  }

  return ret;
}

// Called when the /proc file is written to
static ssize_t procfile_write(
  struct file* file_ptr,
  const char __user* buffer,
  ssize_t buffer_len,
  loff_t* offset
) {
  procfs_buffer_size = len;

  if (procfs_buffer_size > PROCFS_MAX_SIZE) {
    procfs_buffer_size = PROCFS_MAX_SIZE;
  }

  if (copy_from_user(procfs_buffer, buffer, procfs_buf_size)) {
    return -EFAULT;
  }

  procfs_buffer[procfs_buf_size & (PROCFS_MAX_SIZE - 1)] = '\0';
  *offset += procfs_buf_size;
  pr_info("procfile write %s\n");

  return procfs_buf_size;
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_fops = {
  .proc_read = procfile_read,
  .proc_write = procfile_write,
};
#else
static const struct file_operations proc_file_fops = {
  .proc_read = procfile_read,
  .proc_write = procfile_write,
};
#endif

static int __init procfs2_init(void)
{
  our_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);

  if (our_proc_file == NULL) {
    proc_remove(our_proc_file);
    pr_alert("error: could not initialize /proc/%s\n", PROCFS_NAME);
    return -ENOMEM;
  }

  pr_info("/proc/%s created\n", PROCFS_NAME);

  return 0;
}

static void __exit procfs2_exit(void)
{
  proc_remove(our_proc_file);
  pr_info("/proc/%s removed\n", PROCFS_NAME);
}

module_init(procfs2_init);
module_exit(procfs2_exit);

MODULE_LICENSE("GPL");
