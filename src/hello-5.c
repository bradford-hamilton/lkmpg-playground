// Example using command line argument passing to a module

#include <linux/init.h>
#include <linux/kernel.h> // Needed for ARRAY_SIZE()
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/stat.h>

MODULE_LICENSE("GPL");

static short int my_short = 1;
static int my_int = 350;
static long int my_long = 9999;
static char* my_string = "foo";
static int my_int_array[3] = { 350, 350, 350 };
static int arr_argc = 0;

// module_param(foo, int, 0000); - (parameters name, data type, and permission bits)
// for exposing parameters in sysfs (if non-zero) at a later stage
module_param(my_short, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARAM_DESC(my_short, "A short integer");

module_param(my_int, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(my_int, "An integer");

module_param(my_long, long, S_IRUSR);
MODULE_PARM_DESC(my_long, "A long integer");

module_param(my_string, charp, 0000);
MODULE_PARM_DESC(my_string, "A character string");

// module_param_array(name, type, num, perm); - Same as module_param args except
// for the new 3rd position num arg. This represents a pointer to the var that will
// store the number of elements of the array initialized by user at module loading time
module_param_array(my_int_array, int, &arr_argc, 0000);
MODULE_PARM_DESC(my_int_array, "An array of integers");

static int __init hello_5_init(void)
{
  int i;

  pr_info("Hello 5\n=============\n");
  pr_info("my_short is a short integer: %hd\n", my_short);
  pr_info("my_int is an integer: %d\n", my_int);
  pr_info("my_long is a long integer: %ld\n", my_long);
  pr_info("my_string is a string: %s\n", my_string);

  for (i = 0; i < ARRAY_SIZE(my_int_array); i++) {
    pr_info("my_int_array[%d] = %d\n", i, my_int_array[i]);
  }

  pr_info("got %d arguments for my_int_array.\n", arr_argc);

  return 0;
}

static void __exit hello_5_exit(void)
{
  pr_info("Goodbye 5\n")
}

module_init(hello_5_init);
module_exit(hello_5_exit);
