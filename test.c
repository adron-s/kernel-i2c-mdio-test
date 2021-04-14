#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/time.h>
#include <linux/netdevice.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/phy.h>

/* small mdio driver for access phy registers of sfp-coper phy.

DTS:

&eth1 {
	mdio2: mdio {
		status = "okay";

		compatible = "virtual,i2c-mdio";
		#address-cells = <1>;
		#size-cells = <0>;

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

static struct i2c_adapter *i2c = NULL;
static struct mii_bus *mii_bus = NULL;

static int testmdio_probe(struct platform_device *pdev)
{
	//struct device *amdev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	i2c = i2c_get_adapter(0); //TODO: !!!
	if (IS_ERR(i2c)) {
		printk(KERN_WARNING "OWL: Error #1\n");
		i2c = NULL;
		goto error;
	}
	printk(KERN_INFO "i2c adapt = 0x%lx is ready\n", (unsigned long)i2c);

	if (!i2c_check_functionality(i2c, I2C_FUNC_I2C))
		return -EINVAL;

	mii_bus = mdio_i2c_alloc(&pdev->dev, i2c);
	if (IS_ERR(mii_bus)){
		goto error; //TODO: !
		return PTR_ERR(mii_bus);
	}

	mii_bus->name = "SFP I2C Bus";
	mii_bus->phy_mask = ~0;

	//ret = mdiobus_register(mii_bus);
	ret = of_mdiobus_register(mii_bus, np);
	if (ret < 0) {
		mdiobus_free(mii_bus);
		goto error; //TODO: !
		return ret;
	}
	printk(KERN_INFO "OWL: %s - done\n", __func__);

	return 0;
error:
	if (i2c)
		i2c_put_adapter(i2c);
	return -ENODEV;
}

static int testmdio_remove(struct platform_device *pdev)
{
	//struct testmdio *am = platform_get_drvdata(pdev);
	//mdiobus_unregister(am->mii_bus);
	if (mii_bus) {
		mdiobus_unregister(mii_bus);
		mdiobus_free(mii_bus);
		mii_bus = NULL;
	}
	if (i2c) {
		i2c_put_adapter(i2c);
		i2c = NULL;
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
