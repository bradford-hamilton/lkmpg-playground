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
#include <linux/string.h>

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
  struct mutex pcd_lock;
};

static struct gpiodrv_private_data {
  int total_devices;
  struct class* class_gpio;
  struct device** dev;
};

struct gpiodrv_private_data gpio_drv_data;

struct of_device_id gpio_device_match[] = {
  { .compatible = "org,bone-gpio-sysfs" },
  {},
};

ssize_t direction_show(struct device* dev, struct device_attribute* attr, char* buf)
{
  struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
  int dir;
  char* direction;

  mutex_lock(&dev_data->pcd_lock);

  dir = gpiod_get_direction(dev_data->desc);
  if (dir < 0) {
    mutex_unlock(&dev_data->pcd_lock);
    return dir;
  }

  direction = (dir == 0) ? "out" : "in";
  ssize_t written = sprintf(buf, "%s\n", direction);

  mutex_unlock(&dev_data->pcd_lock);

  return written;
}

ssize_t direction_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
  struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
  int ret;

  mutex_lock(&dev_data->pcd_lock);

  if (sysfs_streq(buf, "in")) {
    ret = gpiod_direction_input(dev_data->desc);
  } else if (sysfs_streq(buf, "out")) {
    ret = gpiod_direction_output(dev_data->desc, 0);
  } else {
    ret = -EINVAL;
  }

  mutex_unlock(&dev_data->pcd_lock);

  return ret ? ret : count;
}

ssize_t value_show(struct device* dev, struct device_attribute* attr, char* buf)
{
  struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
  int value;
  
  mutex_lock(&dev_data->pcd_lock);

  value = gpiod_get_value(dev_data->desc);
  ssize_t written = sprintf(buf, "%d\n", value);

  mutex_unlock(&dev_data->pcd_lock);

  return written;
}

ssize_t value_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
  struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
  int ret;
  long value;

  mutex_lock(&dev_data->pcd_lock);

  ret = kstrtol(buf, 0, &value);
  if (ret) {
    return ret;
  }
  gpiod_set_value(dev_data->desc, value);

  mutex_unlock(&dev_data->pcd_lock);

  return count;
}

ssize_t label_show(struct device* dev, struct device_attribute* attr, char* buf)
{
  struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
  
  mutex_lock(&dev_data->pcd_lock);
  ssize_t written = sprintf(buf, "%s\n", dev_data->label);
  mutex_unlock(&dev_data->pcd_lock);
  
  return written;
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
  .attrs = gpio_attrs,
};

static const struct attribute_group* gpio_attr_groups[] = {
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
  const char* name;
  int i = 0;

  gpio_drv_data.total_devices = of_get_child_count(parent);
  if (!gpio_drv_data.total_devices) {
    dev_err(dev, "No devices found\n");
    return -EINVAL;
  }

  dev_info(dev, "Total devices == %d\n", gpio_drv_data.total_devices);

  gpio_drv_data.dev = devm_kzalloc(dev, sizeof(struct device*) * gpio_drv_data.total_devices, GFP_KERNEL);

  for_each_available_child_of_node(parent, child) {
    dev_data = devm_kzalloc(dev, sizeof(*dev_data), GFP_KERNEL);
    if (!dev_data) {
      dev_err(dev, "Cannot allocate memory\n");
      return -ENOMEM;
    }

    mutex_init(&dev_data->pcd_lock);

    if (of_property_read_string(child, "label", &name)) {
      dev_warn(dev, "Missing label information\n");
      snprintf(dev_data->label, sizeof(dev_data->label), "unkngpio%d", i);
    } else {
      strcpy(dev_data->label, name);
      dev_info(dev, "GPIO label = %s\n", dev_data->label);
    }

    dev_data->desc = devm_fwnode_get_gpiod_from_child(dev, "bone", &child->fwnode, GPIOD_ASIS, dev_data->label);
    if (IS_ERR(dev_data->desc)) {
      ret = PTR_ERR(dev_data->desc);
      if (ret == -ENOENT) {
        dev_err(dev, "No gpio has been assigned to the requested function and/or index\n");
      }
      return ret;
    }

    // Takes into account ACTIVE_LOW/HIGH, so writing 1 to this func will always set
    // the gpio to "active" regardless of whether it's active when it's high or low.
    ret = gpiod_direction_output(dev_data->desc, 0);
    if (ret) {
      dev_err(dev, "gpio direction set failed\n");
      return ret;
    }

    gpio_drv_data.dev[i] = device_create_with_groups(gpio_drv_data.class_gpio, dev, 0, dev_data, gpio_attr_groups, dev_data->label);
    if (IS_ERR(gpio_drv_data.dev[i])) {
      dev_err(dev, "Error during device_create\n");
      return PTR_ERR(gpio_drv_data.dev[i]);
    }

    i++;
  }

  return 0;
}

static int gpio_sysfs_remove(struct platform_device* pdev)
{
  dev_info(&pdev->dev, "Remove called\n");
  int i;
  for (i = 0; i < gpio_drv_data.total_devices; i++) {
    of_device_unregister(gpio_drv_data.dev[i]);
  }
  return 0;
}

const static struct platform_driver gpio_sysfs_platform_driver = {
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

module_init(gpio_sysfs_init);
module_exit(gpio_sysfs_exit);
