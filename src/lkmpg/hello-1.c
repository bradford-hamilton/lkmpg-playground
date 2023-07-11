// The simplest kernel module

#include <linux/module.h> // Needed by all modules
#include <linux/printk.h> // Needed for pr_info()

int init_module(void)
{
  pr_info("Hello 1\n");
  return 0;
}

void cleanup_module(void)
{
  pr_info("Goodbye 1\n")
}

MODULE_LICENSE("GPL");
