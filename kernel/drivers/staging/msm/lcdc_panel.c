

#include "msm_fb.h"

static int lcdc_panel_on(struct platform_device *pdev)
{
	return 0;
}

static int lcdc_panel_off(struct platform_device *pdev)
{
	return 0;
}

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
	.driver = {
		.name   = "lcdc_panel",
	},
};

static struct msm_fb_panel_data lcdc_panel_data = {
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
};

static int lcdc_dev_id;

int lcdc_device_register(struct msm_panel_info *pinfo)
{
	struct platform_device *pdev = NULL;
	int ret;

	pdev = platform_device_alloc("lcdc_panel", ++lcdc_dev_id);
	if (!pdev)
		return -ENOMEM;

	lcdc_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &lcdc_panel_data,
		sizeof(lcdc_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init lcdc_panel_init(void)
{
	return platform_driver_register(&this_driver);
}

module_init(lcdc_panel_init);
