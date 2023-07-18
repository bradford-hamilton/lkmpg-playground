#include <linux/module.h>
#include <linux/platform_device.h>
#include <platform.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

static void pcdev_release(void);

struct pcdev_platform_data pcdev_pltfrm_data[] = {
  [0]: {
    .size = 512,
    .perm = RDWR,
    .serial_num = "pcdevabc2023",
  },
  [1]: {
    .size = 1024,
    .perm = RDWR,
    .serial_num = "pcdevdef2023",
  },
  [2]: {
    .size = 128,
    .perm = RDONLY,
    .serial_num = "pcdevghi2023",
  },
  [3]: {
    .size = 32,
    .perm = WRONLY,
    .serial_num = "pcdevjkl2023",
  },
};

struct platform_device platform_pcdev_1 = {
  .name = "pcdev-A1x",
  .id = 0,
  .dev {
    .platform_data = &pcdev_platform_data[0],
    .release = pcdev_release,
  },
};

struct platform_device platform_pcdev_2 = {
  .name = "pcdev-B1x",
  .id = 1,
  .dev {
    .platform_data = &pcdev_platform_data[1],
    .release = pcdev_release,
  },
};

struct platform_device platform_pcdev_3 = {
  .name = "pcdev-C1x",
  .id = 2,
  .dev {
    .platform_data = &pcdev_platform_data[2],
    .release = pcdev_release,
  },
};

struct platform_device platform_pcdev_4 = {
  .name = "pcdev-D1x",
  .id = 3,
  .dev {
    .platform_data = &pcdev_platform_data[3],
    .release = pcdev_release,
  },
};

struct platform_device* platform_pcdevs[] = {
  &platform_pcdev_1,
  &platform_pcdev_2,
  &platform_pcdev_3,
  &platform_pcdev_4,
}

static void pcdev_release(void)
{
  pr_info("Device released\n");
}

static int __init platform_pcdev_init(void)
{
  platform_add_devices(platform_pcdevs, ARRAY_SIZE(platform_pcdevs));

  pr_info("Device setup module loaded\n");

  return 0;
}

static void __exit platform_pcdev_exit(void)
{
  platform_device_unregister(&platform_pcdev_1);
  platform_device_unregister(&platform_pcdev_2);
  platform_device_unregister(&platform_pcdev_3);
  platform_device_unregister(&platform_pcdev_4);

  pr_info("Device setup module unloaded\n");
}

module_init(platform_pcdev_init);
module_exit(platform_pcdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("udemy ldd programming");
MODULE_DESCRIPTION("module which registers platform devices");
MODULE_VERSION("1.0");
