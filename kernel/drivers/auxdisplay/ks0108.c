

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/parport.h>
#include <linux/uaccess.h>
#include <linux/ks0108.h>

#define KS0108_NAME "ks0108"


static unsigned int ks0108_port = CONFIG_KS0108_PORT;
module_param(ks0108_port, uint, S_IRUGO);
MODULE_PARM_DESC(ks0108_port, "Parallel port where the LCD is connected");

static unsigned int ks0108_delay = CONFIG_KS0108_DELAY;
module_param(ks0108_delay, uint, S_IRUGO);
MODULE_PARM_DESC(ks0108_delay, "Delay between each control writing (microseconds)");


static struct parport *ks0108_parport;
static struct pardevice *ks0108_pardevice;


#define bit(n) (((unsigned char)1)<<(n))

void ks0108_writedata(unsigned char byte)
{
	parport_write_data(ks0108_parport, byte);
}

void ks0108_writecontrol(unsigned char byte)
{
	udelay(ks0108_delay);
	parport_write_control(ks0108_parport, byte ^ (bit(0) | bit(1) | bit(3)));
}

void ks0108_displaystate(unsigned char state)
{
	ks0108_writedata((state ? bit(0) : 0) | bit(1) | bit(2) | bit(3) | bit(4) | bit(5));
}

void ks0108_startline(unsigned char startline)
{
	ks0108_writedata(min(startline,(unsigned char)63) | bit(6) | bit(7));
}

void ks0108_address(unsigned char address)
{
	ks0108_writedata(min(address,(unsigned char)63) | bit(6));
}

void ks0108_page(unsigned char page)
{
	ks0108_writedata(min(page,(unsigned char)7) | bit(3) | bit(4) | bit(5) | bit(7));
}

EXPORT_SYMBOL_GPL(ks0108_writedata);
EXPORT_SYMBOL_GPL(ks0108_writecontrol);
EXPORT_SYMBOL_GPL(ks0108_displaystate);
EXPORT_SYMBOL_GPL(ks0108_startline);
EXPORT_SYMBOL_GPL(ks0108_address);
EXPORT_SYMBOL_GPL(ks0108_page);


static unsigned char ks0108_inited;
unsigned char ks0108_isinited(void)
{
	return ks0108_inited;
}
EXPORT_SYMBOL_GPL(ks0108_isinited);


static int __init ks0108_init(void)
{
	int result;
	int ret = -EINVAL;

	ks0108_parport = parport_find_base(ks0108_port);
	if (ks0108_parport == NULL) {
		printk(KERN_ERR KS0108_NAME ": ERROR: "
			"parport didn't find %i port\n", ks0108_port);
		goto none;
	}

	ks0108_pardevice = parport_register_device(ks0108_parport, KS0108_NAME,
		NULL, NULL, NULL, PARPORT_DEV_EXCL, NULL);
	if (ks0108_pardevice == NULL) {
		printk(KERN_ERR KS0108_NAME ": ERROR: "
			"parport didn't register new device\n");
		goto none;
	}

	result = parport_claim(ks0108_pardevice);
	if (result != 0) {
		printk(KERN_ERR KS0108_NAME ": ERROR: "
			"can't claim %i parport, maybe in use\n", ks0108_port);
		ret = result;
		goto registered;
	}

	ks0108_inited = 1;
	return 0;

registered:
	parport_unregister_device(ks0108_pardevice);

none:
	return ret;
}

static void __exit ks0108_exit(void)
{
	parport_release(ks0108_pardevice);
	parport_unregister_device(ks0108_pardevice);
}

module_init(ks0108_init);
module_exit(ks0108_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Miguel Ojeda Sandonis <miguel.ojeda.sandonis@gmail.com>");
MODULE_DESCRIPTION("ks0108 LCD Controller driver");

