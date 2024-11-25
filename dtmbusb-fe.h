// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * DtmbUSB driver
 *
 * Copyright (C) 2023 Xiaodong Ni <nxiaodong520@gmail.com>
 */

#ifndef DTMBUSB-FE_H
#define DTMBUSB-FE_H

struct dtmbusb_config {
	/* the demodulator's i2c address */
	u8 demod_address;
};

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

struct dvb_frontend *dtmbusb_fe_attach(struct dvb_usb_adapter *adap, const struct dtmbusb_config *config);

#endif