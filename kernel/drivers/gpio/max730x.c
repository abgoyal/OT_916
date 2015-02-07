

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/spi/max7301.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#define PIN_CONFIG_MASK 0x03
#define PIN_CONFIG_IN_PULLUP 0x03
#define PIN_CONFIG_IN_WO_PULLUP 0x02
#define PIN_CONFIG_OUT 0x01

#define PIN_NUMBER 28

static int max7301_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct max7301 *ts = container_of(chip, struct max7301, chip);
	u8 *config;
	u8 offset_bits;
	int ret;

	/* First 4 pins are unused in the controller */
	offset += 4;
	offset_bits = (offset & 3) << 1;

	config = &ts->port_config[offset >> 2];

	mutex_lock(&ts->lock);

	/* Standard GPIO API doesn't support pull-ups, has to be extended.
	 * Hard-coding no pollup for now. */
	*config = (*config & ~(PIN_CONFIG_MASK << offset_bits))
			   | (PIN_CONFIG_IN_WO_PULLUP << offset_bits);

	ret = ts->write(ts->dev, 0x08 + (offset >> 2), *config);

	mutex_unlock(&ts->lock);

	return ret;
}

static int __max7301_set(struct max7301 *ts, unsigned offset, int value)
{
	if (value) {
		ts->out_level |= 1 << offset;
		return ts->write(ts->dev, 0x20 + offset, 0x01);
	} else {
		ts->out_level &= ~(1 << offset);
		return ts->write(ts->dev, 0x20 + offset, 0x00);
	}
}

static int max7301_direction_output(struct gpio_chip *chip, unsigned offset,
				    int value)
{
	struct max7301 *ts = container_of(chip, struct max7301, chip);
	u8 *config;
	u8 offset_bits;
	int ret;

	/* First 4 pins are unused in the controller */
	offset += 4;
	offset_bits = (offset & 3) << 1;

	config = &ts->port_config[offset >> 2];

	mutex_lock(&ts->lock);

	*config = (*config & ~(PIN_CONFIG_MASK << offset_bits))
			   | (PIN_CONFIG_OUT << offset_bits);

	ret = __max7301_set(ts, offset, value);

	if (!ret)
		ret = ts->write(ts->dev, 0x08 + (offset >> 2), *config);

	mutex_unlock(&ts->lock);

	return ret;
}

static int max7301_get(struct gpio_chip *chip, unsigned offset)
{
	struct max7301 *ts = container_of(chip, struct max7301, chip);
	int config, level = -EINVAL;

	/* First 4 pins are unused in the controller */
	offset += 4;

	mutex_lock(&ts->lock);

	config = (ts->port_config[offset >> 2] >> ((offset & 3) << 1))
			& PIN_CONFIG_MASK;

	switch (config) {
	case PIN_CONFIG_OUT:
		/* Output: return cached level */
		level =  !!(ts->out_level & (1 << offset));
		break;
	case PIN_CONFIG_IN_WO_PULLUP:
	case PIN_CONFIG_IN_PULLUP:
		/* Input: read out */
		level = ts->read(ts->dev, 0x20 + offset) & 0x01;
	}
	mutex_unlock(&ts->lock);

	return level;
}

static void max7301_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct max7301 *ts = container_of(chip, struct max7301, chip);

	/* First 4 pins are unused in the controller */
	offset += 4;

	mutex_lock(&ts->lock);

	__max7301_set(ts, offset, value);

	mutex_unlock(&ts->lock);
}

int __devinit __max730x_probe(struct max7301 *ts)
{
	struct device *dev = ts->dev;
	struct max7301_platform_data *pdata;
	int i, ret;

	pdata = dev->platform_data;
	if (!pdata || !pdata->base) {
		dev_err(dev, "incorrect or missing platform data\n");
		return -EINVAL;
	}

	mutex_init(&ts->lock);
	dev_set_drvdata(dev, ts);

	/* Power up the chip and disable IRQ output */
	ts->write(dev, 0x04, 0x01);

	ts->chip.label = dev->driver->name;

	ts->chip.direction_input = max7301_direction_input;
	ts->chip.get = max7301_get;
	ts->chip.direction_output = max7301_direction_output;
	ts->chip.set = max7301_set;

	ts->chip.base = pdata->base;
	ts->chip.ngpio = PIN_NUMBER;
	ts->chip.can_sleep = 1;
	ts->chip.dev = dev;
	ts->chip.owner = THIS_MODULE;

	/*
	 * tristate all pins in hardware and cache the
	 * register values for later use.
	 */
	for (i = 1; i < 8; i++) {
		int j;
		/* 0xAA means input with internal pullup disabled */
		ts->write(dev, 0x08 + i, 0xAA);
		ts->port_config[i] = 0xAA;
		for (j = 0; j < 4; j++) {
			int offset = (i - 1) * 4 + j;
			ret = max7301_direction_input(&ts->chip, offset);
			if (ret)
				goto exit_destroy;
		}
	}

	ret = gpiochip_add(&ts->chip);
	if (ret)
		goto exit_destroy;

	return ret;

exit_destroy:
	dev_set_drvdata(dev, NULL);
	mutex_destroy(&ts->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(__max730x_probe);

int __devexit __max730x_remove(struct device *dev)
{
	struct max7301 *ts = dev_get_drvdata(dev);
	int ret;

	if (ts == NULL)
		return -ENODEV;

	dev_set_drvdata(dev, NULL);

	/* Power down the chip and disable IRQ output */
	ts->write(dev, 0x04, 0x00);

	ret = gpiochip_remove(&ts->chip);
	if (!ret) {
		mutex_destroy(&ts->lock);
		kfree(ts);
	} else
		dev_err(dev, "Failed to remove GPIO controller: %d\n", ret);

	return ret;
}
EXPORT_SYMBOL_GPL(__max730x_remove);

MODULE_AUTHOR("Juergen Beisert, Wolfram Sang");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MAX730x GPIO-Expanders, generic parts");
