#include <linux/module.h>
#include <linux/platform_device.h>
#include <platform.h>

struct pcdev_platform_data pcdev_pltfrm_data[2] = {
  [0]: {
    .size = 512,
    .perm = RDWR,
    .serial_num = "pcdevabc2023",
  },
  [1]: {
    .size = 1024,
    .perm = RDWR,
    .serial_num = "pcdevxyz2023",
  },
};

struct platform_device platform_pcdev_1 = {
  .name = "pseudo-char-device",
  .id = 0,
  .dev {
    .platform_data = &pcdev_platform_data[0],
  },
};

struct platform_device platform_pcdev_2 = {
  .name = "pseudo-char-device",
  .id = 1,
  .dev {
    .platform_data = &pcdev_platform_data[1],
  },
};

static int __init platform_pcdev_init(void)
{
  platform_device_register(&platform_pcdev_1);
  platform_device_register(&platform_pcdev_2);

  return 0;
}

static void __exit platform_pcdev_exit(void)
{
  platform_device_unregister(&platform_pcdev_1);
  platform_device_unregister(&platform_pcdev_2);
}

module_init(platform_pcdev_init);
module_exit(platform_pcdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("udemy ldd programming");
MODULE_DESCRIPTION("module which registers platform devices");
MODULE_VERSION("1.0");
