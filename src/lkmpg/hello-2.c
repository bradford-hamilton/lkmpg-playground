// In early kernel versions you had to use the init_module and cleanup_module
// functions, as in the first hello world example, but these days you can name
// those anything you want by using the module_init and module_exit macros.

#include <linux/init.h> // Needed for the macros
#include <linux/module.h> // Needed by all modules
#include <linux/printk.h> // Needed for pr_info()

static int __init hello_2_init(void)
{
  pr_info("Hello 2\n");
  return 0;
}

static void __exit hello_2_exit(void)
{
  pr_info("Goodbye 2\n");
}

module_init(hello_2_init);
module_exit(hello_2_exit);

MODULE_LICENSE("GPL");
