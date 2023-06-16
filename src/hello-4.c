// Example using module documentation

#include <linux/init.h> // Needed for the macros
#include <linux/module.h> // Needed by all modules
#include <linux/printk.h> // Needed for pr_info()

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKMPG");
MODULE_DESCRIPTION("Sample driver description");

static int __init init_hello_4(void)
{
  pr_info("Hello 4\n");
  return 0;
}

static void __exit exit_hello_4(void)
{
  pr_info("Goodbye 4\n");
}

module_init(init_hello_4);
module_exit(exit_hello_4);
