#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("udemy ldd programming");
MODULE_DESCRIPTION("gpio sysfs driver");
MODULE_VERSION("1.0");

// Format every pr_* message with the current running function name
#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

static struct gpiodev_private_data {
  char label[20];
  struct gpio_desc* desc;
};

static struct gpiodrv_private_data {
  int total_devices;
  struct class* class_gpio;
};

struct gpiodrv_private_data gpio_drv_data;

struct of_device_id gpio_device_match[] = {
  { .compatible = "org,bone-gpio-sysfs" },
  {},
};

ssize_t direction_show(struct device* dev, struct device_attribute* attr, char* buf)
{
  return 0;
}

ssize_t direction_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
  return 0;
}

ssize_t value_show(struct device* dev, struct device_attribute* attr, char* buf)
{
  return 0;
}

ssize_t value_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
  return 0;
}

ssize_t label_show(struct device* dev, struct device_attribute* attr, char* buf)
{
  return 0;
}

static DEVICE_ATTR_RW(direction);
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(label);

static struct attribute* gpio_attrs[] = {
  &dev_attr_direction.attr,
  &dev_attr_value.attr,
  &dev_attr_label.attr,
  NULL,
};

static struct attribute_group gpio_attr_group = {
  .attr = gpio_attrs,
};

static struct attribute_group* gpio_attr_groups[] = {
  &gpio_attr_group,
  NULL,
};

static int gpio_sysfs_probe(struct platform_device* pdev)
{
  int ret;
  struct device* dev = &pdev->dev;
  struct device_node* parent = pdev->dev.of_node;
  struct device_node* child = NULL;
  struct gpiodev_private_data* dev_data;
  struct device* dev_sysfs;
  const char* name;
  int i = 0;

  for_each_available_child_of_node(parent, child) {
    dev_data = devm_kzalloc(dev, sizeof(*dev_data), GFP_KERNEL);
    if (!dev_data) {
      dev_err(dev, "Cannot allocate memory\n");
      return -ENOMEM;
    }

    if (of_property_read_string(child, "label", &name)) {
      dev_warn(dev, "Missing label information\n");
      snprintf(dev_data->label, sizeof(dev_data->label), "unkngpio%d", i);
    } else {
      strcpy(dev_data->label, name);
      dev_info(dev, "GPIO label = %s\n", dev_data->label);
    }
  }

  dev_data->desc = devm_fwnode_get_gpiod_from_child(dev, "bone", child->fwnode, GPIOD_ASIS, dev_data->label);
  if (IS_ERR(dev_data->desc)) {
    ret = PTR_ERR(dev_data->desc);
    if (ret == -ENOENT) {
      dev_err("No gpio has been assigned to the requested function and/or index\n");
    }
    return ret;
  }

  // Takes into account ACTIVE_LOW/HIGH, so writing 1 to this func will always set
  // the gpio to "active" regardless of whether it's active when it's high or low.
  gpiod_direction_output(dev_data->desc, 0);
  if (ret) {
    dev_err("gpio direction set failed\n");
    return ret;
  }

  dev_sysfs = device_create_with_groups(gpio_drv_data.class_gpio, dev, 0, dev_data, gpio_attr_groups, dev_data->label);
  if (IS_ERR(dev_sysfs)) {
    dev_err("Error during device_create\n");
    return PTR_ERR(dev_sysfs);
  }

  i++;

  return 0;
}

static int gpio_sysfs_remove(struct platform_device* pdev)
{
  return 0;
}

const static struct platform_driver gpio_sysfs_platform_driver {
  .probe = gpio_sysfs_probe,
  .remove = gpio_sysfs_remove,
  .driver = {
    .name = "bone-gpio-sysfs",
    .of_match_table = of_match_ptr(gpio_device_match),
  },
};

static int __init gpio_sysfs_init(void)
{
  gpio_drv_data.class_gpio = class_create(THIS_MODULE, "bone_gpios");
  if (IS_ERR(gpio_drv_data.class_gpio)) {
    pr_err("Error creating class\n");
    return PTR_ERR(gpio_drv_data.class_gpio);
  }

  platform_driver_register(&gpio_sysfs_platform_driver);

  pr_info("Module successfully loaded\n");

  return 0;
}

static void __exit gpio_sysfs_exit(void)
{
  platform_driver_unregister(&gpio_sysfs_platform_driver);
  class_destroy(gpio_drv_data.class_gpio);
}
