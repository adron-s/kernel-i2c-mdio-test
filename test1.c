#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/time.h>
#include <linux/netdevice.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/phy.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
struct mii_bus *mdio_i2c_alloc(struct device *, struct i2c_adapter *);
#else
#include <linux/mdio/mdio-i2c.h>
#endif

static struct i2c_adapter *i2c_adap = NULL;
static struct i2c_client *i2c_client = NULL;
struct platform_device *pdev = NULL;

/* SFP modules appear to always have their PHY configured for bus address
 * 0x56 (which with mdio-i2c, translates to a PHY address of 22).
 */
#define SFP_PHY_ADDR	22

static int do_i2c(void) {
	struct i2c_adapter *i2c = i2c_adap;
	struct mii_bus *i2c_mii;
	int ret, phy_reg, phy_id1, phy_id2;

	if (!i2c_check_functionality(i2c, I2C_FUNC_I2C))
		return -EINVAL;

	i2c_mii = mdio_i2c_alloc(&pdev->dev, i2c);
	if (IS_ERR(i2c_mii))
		return PTR_ERR(i2c_mii);

	i2c_mii->name = "SFP I2C Bus";
	i2c_mii->phy_mask = ~0;

	ret = mdiobus_register(i2c_mii);
	if (ret < 0) {
		mdiobus_free(i2c_mii);
		return ret;
	}
	printk(KERN_INFO "OWL: %s - done\n", __func__);

	//для начала прочитаем ID-шки phy устройства
	phy_id1 = mdiobus_read(i2c_mii, SFP_PHY_ADDR, MII_PHYSID1);
	phy_id2 = mdiobus_read(i2c_mii, SFP_PHY_ADDR, MII_PHYSID2);
	printk(KERN_INFO "OWL: %s phy ids: %04x:%04x\n", __func__, phy_id1, phy_id2);
	phy_reg = mdiobus_read(i2c_mii, SFP_PHY_ADDR, MII_BMSR);
	printk(KERN_INFO "OWL: %s phy BMSR: 0x%08x\n", __func__, phy_reg);

	mdiobus_unregister(i2c_mii);
	mdiobus_free(i2c_mii);

	return 0;
}

static int __init
testdog_init(void) {
	i2c_adap = i2c_get_adapter(0);
	if (IS_ERR(i2c_adap)) {
		printk(KERN_WARNING "OWL: Error #1\n");
		i2c_adap = NULL;
		i2c_client = NULL;
		goto end;
	}

	/* i2c_client = i2c_new_client_device(i2c_adap, &apu4_i2c_board_info);
	if (IS_ERR(i2c_client)) {
		printk(KERN_WARNING "OWL: Error #2\n");
		i2c_client = NULL;
		goto end;
	} */
	printk(KERN_INFO "i2c adapt = 0x%lx is ready\n", (unsigned long)i2c_adap);

	pdev = platform_device_alloc("owl-x991y", -1);

	if (pdev) {
		//platform_device_add_data(pdev, pdata, sizeof(*pdata);
		platform_device_add(pdev);
		printk(KERN_INFO "platform device = 0x%lx is ready\n", (unsigned long)pdev);
		do_i2c();
	}

end:
	if(pdev) {
		platform_device_del(pdev);
		platform_device_put(pdev);
	}
	if (i2c_client)
		i2c_unregister_device(i2c_client);
	if (i2c_adap)
		i2c_put_adapter(i2c_adap);
	return -ENODEV;
}
static void __exit
testdog_exit(void) {
}

module_init(testdog_init);
module_exit(testdog_exit);
MODULE_AUTHOR("Serhii Serhieiev <adron@mstnt.com>");
MODULE_DESCRIPTION("Test driver)");
MODULE_LICENSE("GPL");
