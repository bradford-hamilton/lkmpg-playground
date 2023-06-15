// Adding example using __init_data macro

#include <linux/init.h> // Needed for the macros
#include <linux/module.h> // Needed by all modules
#include <linux/printk.h> // Needed for pr_info()

static int hello3_data __init_data = 21;

static int __init hello_3_init(void)
{
  pr_info("Hello 3, data: %d\n", hello3_data);
  return 0;
}

static void __exit hello_3_exit(void)
{
  pr_info("Goodbye 3");
}

module_init(hello_3_init);
module_exit(hello_3_exit);

MODULE_LICENSE("GPL");
