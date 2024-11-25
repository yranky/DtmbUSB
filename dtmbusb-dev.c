// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * DtmbUSB driver
 *
 * Copyright (C) 2023 Xiaodong Ni <nxiaodong520@gmail.com>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#include <dvb_frontend.h>
#else
#include <media/dvb_frontend.h>
#endif

#include "dvb-usb.h"
#include "dtmbusb-fe.h"

#define CONFIG_DYNAMIC_DEBUG

#define USB_PID_USBDTMB                                0x1004

static struct dtmbusb_config dtmbusb_cfg = {
	.demod_address = 0,
};

static int dtmbusb_frontend_attach(struct dvb_usb_adapter *adap)
{
	if ((adap->fe_adap[0].fe = dvb_attach(dtmbusb_fe_attach, adap, &dtmbusb_cfg)) != NULL) {
		return 0;
	}
	info("not attached DtmbUSB");
	return -ENODEV;
}

static struct usb_device_id dtmbusb_table[] = {
	{USB_DEVICE(USB_VID_CYPRESS, USB_PID_USBDTMB)},
/*	{USB_DEVICE(USB_VID_OPERA1, USB_PID_USBDTMB)},*/
	{}
};

MODULE_DEVICE_TABLE(usb, dtmbusb_table);

static struct dvb_usb_device_properties dtmbusb_properties = {
	.num_adapters = 1,

	.adapter = {
		{
		.num_frontends = 1,
		.fe = {{
			.caps = DVB_USB_ADAP_HAS_PID_FILTER,
			.frontend_attach  = dtmbusb_frontend_attach,

	/* parameter for the MPEG2-data transfer */
			.stream = {
				.type = USB_BULK,
				.count = 7,
				.endpoint = 0x82,
				.u = {
					.bulk = {
						.buffersize = 4096,
					}
				}
			},
		}},	
		},
	},
	.num_device_descs = 1,
	.devices = {
		{"DtmbUSB DTMB demodulator",
			{NULL},
			{&dtmbusb_table[0], NULL},
		},
	}
};

static int dtmbusb_probe(struct usb_interface *intf,
			const struct usb_device_id *id)
{
	if (0 != dvb_usb_device_init(intf, &dtmbusb_properties,
				     THIS_MODULE, NULL, adapter_nr))
		return -EINVAL;
	return 0;
}

static struct usb_driver dtmbusb_driver = {
	.name = "DtmbUSB",
	.probe = dtmbusb_probe,
	.disconnect = dvb_usb_device_exit,
	.id_table = dtmbusb_table,
};

static int __init dtmbusb_module_init(void)
{
	int result = 0;
	if ((result = usb_register(&dtmbusb_driver))) {
		err("usb_register failed. Error number %d", result);
	}
	return result;
}

static void __exit dtmbusb_module_exit(void)
{
	usb_deregister(&dtmbusb_driver);
}

module_init(dtmbusb_module_init);
module_exit(dtmbusb_module_exit);

MODULE_DESCRIPTION("DtmbUSB usb driver");
MODULE_AUTHOR("Xiaodong Ni <nxiaodong520@gmail.com>");
MODULE_LICENSE("GPL");

