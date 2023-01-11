#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/phy.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
struct mii_bus *mdio_i2c_alloc(struct device *, struct i2c_adapter *);
#else
#include <linux/mdio/mdio-i2c.h>
#endif

struct i2c_mdio_info {
	struct i2c_adapter *i2c;
	struct mii_bus *mii_bus;
};

#define check_for_err(what, msg) \
if (IS_ERR(what)) { 						 \
	dev_err(&pdev->dev, msg);			 \
	ret = PTR_ERR(what);					 \
	what = NULL;									 \
	goto error;										 \
}

static int mdio_sfp_i2c_probe(struct platform_device *pdev)
{
	struct i2c_mdio_info *info;
	struct device_node *i2c_np;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	i2c_np = of_parse_phandle(np, "i2c-bus", 0);
	if (!i2c_np) {
		dev_err(&pdev->dev, "missing 'i2c-bus' property\n");
		return -ENODEV;
	}
	info->i2c = of_find_i2c_adapter_by_node(i2c_np);
	of_node_put(i2c_np);
	check_for_err(info->i2c, "can't find 'i2c-bus' adapter\n");

	if (!i2c_check_functionality(info->i2c, I2C_FUNC_I2C)){
		ret = -EINVAL;
		goto error;
	}

	info->mii_bus = mdio_i2c_alloc(&pdev->dev, info->i2c);
	check_for_err(info->mii_bus, "can't alloc i2c mdio\n");
	info->mii_bus->name = "SFP I2C Bus";
	info->mii_bus->phy_mask = ~0;

	ret = of_mdiobus_register(info->mii_bus, np);
	if (ret < 0)
		goto error;

	platform_set_drvdata(pdev, info);
	return 0;
error:
	if (info->mii_bus)
		mdiobus_free(info->mii_bus);
	if (info->i2c)
		i2c_put_adapter(info->i2c);
	return ret;
}

static int mdio_sfp_i2c_remove(struct platform_device *pdev)
{
	struct i2c_mdio_info *info = platform_get_drvdata(pdev);
	if (info->mii_bus) {
		mdiobus_unregister(info->mii_bus);
		mdiobus_free(info->mii_bus);
		info->mii_bus = NULL;
	}
	if (info->i2c) {
		i2c_put_adapter(info->i2c);
		info->i2c = NULL;
	}

	return 0;
}

static const struct of_device_id mdio_sfp_i2c_match[] = {
	{ .compatible = "virtual,mdio-sfp-i2c" },
	{}
};

static struct platform_driver mdio_sfp_i2c_driver = {
	.probe		= mdio_sfp_i2c_probe,
	.remove		= mdio_sfp_i2c_remove,
	.driver = {
		.name	 = "mdio-sfp-i2c",
		.of_match_table = mdio_sfp_i2c_match,
	}
};

module_platform_driver(mdio_sfp_i2c_driver);
MODULE_ALIAS("platform:mdio-sfp-i2c");
MODULE_AUTHOR("Serhii Serhieiev <adron@mstnt.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Generic driver for MDIO bus emulation using SFP-I2C(phy id 22, i2c id 0x56)");
