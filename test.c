#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/phy.h>

/* small mdio driver for access phy registers of sfp-coper phy(22 - 0x56) via i2c.
	 this is a compilation of code from mdio-gpio.c ag71xx_mdio.c and sfp.c */

/* DTS:

&eth1 {
	mdio2: mdio {
		status = "okay";

		compatible = "virtual,i2c-mdio";
		#address-cells = <1>;
		#size-cells = <0>;

		i2c-bus = <&sfp_i2c>;
		sfp_phy: ethernet-phy@22 {
			reg = <22>;
		};
	};
};

&eth1 {
	status = "okay";
	...
	phy-handle = <&sfp_phy>;
};

*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
struct mii_bus *mdio_i2c_alloc(struct device *, struct i2c_adapter *);
#else
#include <linux/mdio/mdio-i2c.h>
#endif

struct i2c_mdio_info {
	struct i2c_adapter *i2c;
	struct mii_bus *mii_bus;
};

static int testmdio_probe(struct platform_device *pdev)
{
	struct i2c_mdio_info *info;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *i2c_np;
	int ret;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	i2c_np = of_parse_phandle(np, "i2c-bus", 0);
	if (!i2c_np) {
		dev_err(&pdev->dev, "missing 'i2c-bus' property\n");
		return -ENODEV;
	}
	//i2c = i2c_get_adapter(0);
	info->i2c = of_find_i2c_adapter_by_node(i2c_np);
	of_node_put(i2c_np);
	if (IS_ERR(info->i2c)) {
		dev_err(&pdev->dev, "can't find 'i2c-bus' adapter\n");
		ret = PTR_ERR(info->i2c);
		info->i2c = NULL;
		goto error;
	}
	//printk(KERN_INFO "i2c adapt = 0x%lx is ready\n", (unsigned long)i2c);

	if (!i2c_check_functionality(info->i2c, I2C_FUNC_I2C)){
		ret = -EINVAL;
		goto error;
	}
	info->mii_bus = mdio_i2c_alloc(&pdev->dev, info->i2c);
	if (IS_ERR(info->mii_bus)){
		ret = PTR_ERR(info->mii_bus);
		info->mii_bus = NULL;
		goto error;
	}
	info->mii_bus->name = "SFP I2C Bus";
	info->mii_bus->phy_mask = ~0;

	//ret = mdiobus_register(mii_bus);
	ret = of_mdiobus_register(info->mii_bus, np);
	if (ret < 0)
		goto error;

	//printk(KERN_INFO "OWL: %s - done\n", __func__);
	platform_set_drvdata(pdev, info);
	return 0;
error:
	if (info->mii_bus)
		mdiobus_free(info->mii_bus);
	if (info->i2c)
		i2c_put_adapter(info->i2c);
	return ret;
}

static int testmdio_remove(struct platform_device *pdev)
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

static const struct of_device_id testmdio_match[] = {
	{ .compatible = "virtual,i2c-mdio" },
	{}
};

static struct platform_driver testmdio_driver = {
	.probe		= testmdio_probe,
	.remove		= testmdio_remove,
	.driver = {
		.name	 = "sfp-i2c-mdio",
		.of_match_table = testmdio_match,
	}
};

module_platform_driver(testmdio_driver);
MODULE_AUTHOR("Serhii Serhieiev <adron@mstnt.com>");
MODULE_DESCRIPTION("Test driver");
MODULE_LICENSE("GPL");
