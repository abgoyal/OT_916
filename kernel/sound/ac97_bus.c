

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include <sound/ac97_codec.h>

static int ac97_bus_match(struct device *dev, struct device_driver *drv)
{
	return 1;
}

#ifdef CONFIG_PM
static int ac97_bus_suspend(struct device *dev, pm_message_t state)
{
	int ret = 0;

	if (dev->driver && dev->driver->suspend)
		ret = dev->driver->suspend(dev, state);

	return ret;
}

static int ac97_bus_resume(struct device *dev)
{
	int ret = 0;

	if (dev->driver && dev->driver->resume)
		ret = dev->driver->resume(dev);

	return ret;
}
#endif /* CONFIG_PM */

struct bus_type ac97_bus_type = {
	.name		= "ac97",
	.match		= ac97_bus_match,
#ifdef CONFIG_PM
	.suspend	= ac97_bus_suspend,
	.resume		= ac97_bus_resume,
#endif /* CONFIG_PM */
};

static int __init ac97_bus_init(void)
{
	return bus_register(&ac97_bus_type);
}

subsys_initcall(ac97_bus_init);

static void __exit ac97_bus_exit(void)
{
	bus_unregister(&ac97_bus_type);
}

module_exit(ac97_bus_exit);

EXPORT_SYMBOL(ac97_bus_type);

MODULE_LICENSE("GPL");
