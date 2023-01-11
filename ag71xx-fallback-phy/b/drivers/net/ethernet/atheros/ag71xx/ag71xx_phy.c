/*
 *  Atheros AR71xx built-in ethernet mac driver
 *
 *  Copyright (C) 2008-2010 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  Based on Atheros' AG7100 driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/of_mdio.h>
#include "ag71xx.h"

static void ag71xx_phy_link_adjust(struct net_device *dev)
{
	struct ag71xx *ag = netdev_priv(dev);
	struct phy_device *phydev = ag->phy_dev;
	unsigned long flags;
	int status_change = 0;

	spin_lock_irqsave(&ag->lock, flags);

	if (phydev->link) {
		if (ag->duplex != phydev->duplex
		    || ag->speed != phydev->speed) {
			status_change = 1;
		}
	}

	if (phydev->link != ag->link)
		status_change = 1;

	ag->link = phydev->link;
	ag->duplex = phydev->duplex;
	ag->speed = phydev->speed;

	if (status_change)
		ag71xx_link_adjust(ag);

	spin_unlock_irqrestore(&ag->lock, flags);
}

int ag71xx_phy_connect(struct ag71xx *ag)
{
	struct device_node *np = ag->pdev->dev.of_node;
	struct device_node *phy_node;
	int ret, a;

	if (of_phy_is_fixed_link(np)) {
		ret = of_phy_register_fixed_link(np);
		if (ret < 0) {
			dev_err(&ag->pdev->dev,
				"Failed to register fixed PHY link: %d\n", ret);
			return ret;
		}

		phy_node = of_node_get(np);
	} else {
		phy_node = of_parse_phandle(np, "phy-handle", 0);
	}

	for (a = 0; a < 2; a++) {
		if (!phy_node) {
			dev_err(&ag->pdev->dev,
				"Could not find valid phy node\n");
			return -ENODEV;
		}

		ag->phy_dev = of_phy_connect(ag->dev, phy_node, ag71xx_phy_link_adjust,
					     0, ag->phy_if_mode);
		if (ag->phy_dev && of_property_read_bool(phy_node, "suspend-on-init")) {
			dev_info(&ag->pdev->dev, "suspend-on-init flag is set => doing phy_suspend()\n");
			phy_suspend(ag->phy_dev);
		}

		/* on the first loop, we try to use fallback-phy as the last chance phy */
		if (!ag->phy_dev && !a) {
			int need_defer = 1;
			struct device_node *fallback_np;
			struct mii_bus *_parent_bus;

			fallback_np = of_find_node_by_name(np, "fallback-phy");
			if (!fallback_np)
				break;
			if (phy_node->parent) {
				/* here we determine: if mdio bus(of our phy) is ready.
					 if yes,then trying phy_connect and if we get an error,
					 => the sfp-rj45 module is simply missing and we need
					 to use the fallback_phy. */
				_parent_bus = of_mdio_find_bus(phy_node->parent);
				if (_parent_bus) {
					put_device(&_parent_bus->dev);
					need_defer = 0;
				}
			}
			if (need_defer)
				return -EPROBE_DEFER;
			dev_err(&ag->pdev->dev, "Trying to use the fallback-phy node\n");
			if (!of_phy_is_fixed_link(fallback_np)) {
				of_node_put(fallback_np);
				break;
			}
			ret = of_phy_register_fixed_link(fallback_np);
			if (ret < 0) {
				of_node_put(fallback_np);
				break;
			}
			of_node_put(phy_node);
			phy_node = of_node_get(fallback_np);
			of_node_put(fallback_np);
			continue;
		}

		break;
	}

	of_node_put(phy_node);

	if (!ag->phy_dev) {
		dev_err(&ag->pdev->dev,
			"Could not connect to PHY device. Deferring probe.\n");
		return -EPROBE_DEFER;
	}

	dev_info(&ag->pdev->dev, "connected to PHY at %s [uid=%08x, driver=%s]\n",
		    phydev_name(ag->phy_dev),
		    ag->phy_dev->phy_id, ag->phy_dev->drv->name);

	return 0;
}

void ag71xx_phy_disconnect(struct ag71xx *ag)
{
	phy_disconnect(ag->phy_dev);
}
