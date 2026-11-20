#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>

static ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buffer)
{
    const u32 *value = dev_get_drvdata(dev);

    return sysfs_emit(buffer, "%u\n", *value);
}
static DEVICE_ATTR_RO(value);

static struct attribute *reading_attrs[] = {
    &dev_attr_value.attr,
    NULL,
};
static const struct attribute_group reading_group = {
    .attrs = reading_attrs,
};

static int reading_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    u32 *value;
    int error;

    // 1. 分配随设备解绑自动释放的值，再从设备树读属性。
    value = devm_kmalloc(dev, sizeof(*value), GFP_KERNEL);
    if (!value)
        return -ENOMEM;
    error = device_property_read_u32(dev, "demo-value", value);
    if (error)
        return dev_err_probe(dev, error, "missing demo-value\n");

    // 2. 属性准备好后才暴露只读 sysfs 节点。
    platform_set_drvdata(pdev, value);
    return devm_device_add_group(dev, &reading_group);
}

static const struct of_device_id reading_of_match[] = {
    { .compatible = "code-kb,reading-demo" },
    { }
};
MODULE_DEVICE_TABLE(of, reading_of_match);

static struct platform_driver reading_driver = {
    .probe = reading_probe,
    .driver = {
        .name = "kb-reading",
        .of_match_table = reading_of_match,
    },
};
module_platform_driver(reading_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Read-only platform driver example");
