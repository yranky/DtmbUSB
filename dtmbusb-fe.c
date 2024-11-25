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

struct dtmbusb_command {
        u8 request;
        u16 value;
        u16 index;
        u16 length;
	u8 *data;
};

static struct dtmbusb_command cmd_get_fw_version = {
        .request = 0xED,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0004,
};
#define CMD_GET_FW_VERSION cmd_get_fw_version

static struct dtmbusb_command cmd_set_frequency = {
        .request = 0xFC,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0004,
};
#define CMD_SET_FREQUENCY cmd_set_frequency

static struct dtmbusb_command cmd_get_tunered = {
        .request = 0xEA,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0001,
};
#define CMD_GET_TUNERED cmd_get_tunered

static struct dtmbusb_command cmd_get_has_signal = {
        .request = 0xEC,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0001,
};
#define CMD_GET_HAS_SIGNAL cmd_get_has_signal

static struct dtmbusb_command cmd_get_signal_info = {
        .request = 0xE7,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0006,
};
#define CMD_GET_SIGNAL_INFO cmd_get_signal_info

static struct dtmbusb_command cmd_get_snr = {
        .request = 0xE8,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0002,
};
#define CMD_GET_SNR cmd_get_snr

static struct dtmbusb_command cmd_get_strength = {
        .request = 0xEB,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0004,
};
#define CMD_GET_STRENGTH cmd_get_strength

static struct dtmbusb_command cmd_get_quality = {
        .request = 0xE9,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0003,
};
#define CMD_GET_QUALITY cmd_get_quality

static struct dtmbusb_command cmd_cvb_send_key = {
        .request = 0xBE,
        .value = 0x00FE,
        .index = 0x0000,
        .length = 0x0009,
	.data = "\x63\x69\x64\x61\x6E\x61\x4B\x45\x59",
};
#define CMD_CVB_SEND_KEY cmd_cvb_send_key

static struct dtmbusb_command cmd_cvb_get_hash = {
	.request = 0xBF,
	.value = 0x00FE,
	.index = 0x0000,
	.length = 0x0016,
};
#define CMD_CVB_GET_HASH cmd_cvb_get_hash

enum DeviceType {LETV, AIWA, CVB};

struct dtmbusb_state {
	struct dvb_usb_device *dev;
        struct dvb_frontend frontend;
	enum DeviceType dev_type;
};

static struct dtmbusb_config dtmbusb_cfg = {
	.demod_address = 0,
};

static int dtmbusb_dev_read(struct dvb_usb_device *dev, struct dtmbusb_command *cmd, void *data)
{
	int ret;

	mutex_lock(&dev->usb_mutex);

	ret = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
			      cmd->request, USB_TYPE_VENDOR | USB_DIR_IN,
			      cmd->value, cmd->index, data, cmd->length, 0);

	mutex_unlock(&dev->usb_mutex);

	if (ret < 0) {
		info("%s = error: %d\n", __func__, ret);
		return ret;
	}

	if (ret != cmd->length) {
		info("%s = no data\n", __func__);
		return -EIO;
	}

	return ret;
}

static int dtmbusb_dev_write(struct dvb_usb_device *dev, struct dtmbusb_command *cmd, void *data)
{
	int ret;

	mutex_lock(&dev->usb_mutex);

	ret = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0), cmd->request,
			       USB_TYPE_VENDOR | USB_DIR_OUT, cmd->value, cmd->index,
			       data, cmd->length, 2000);
	mutex_unlock(&dev->usb_mutex);

	return ret;
}

static int dtmbusb_fe_init(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *props = &fe->dtv_property_cache;

	props->strength.len = 1;
	props->strength.stat[0].scale = FE_SCALE_RELATIVE;
	props->cnr.len = 1;
	props->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	props->block_error.len = 1;
	props->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	props->post_bit_error.len = 1;
	props->post_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	props->post_bit_count.len = 1;
	props->post_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;

	return 0;
}

static void dtmbusb_fe_release(struct dvb_frontend *fe)
{
	struct dtmbusb_state *state = fe->demodulator_priv;

	kfree(state);
}

static int dtmbusb_set_frontend(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *fe_params = &fe->dtv_property_cache;
	struct dtmbusb_state *priv = fe->demodulator_priv;
	int ret;
	u8 *buf;
	u32 frequency;

	frequency = fe_params->frequency;
	
	/* set frequency */
  buf = kmalloc(CMD_SET_FREQUENCY.length, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

	buf[0] = (frequency>>24) & 0xFF;
	buf[1] = (frequency>>16) & 0xFF;
	buf[2] = (frequency>>6) & 0xFF;
	buf[3] = frequency & 0xFF;

	ret = dtmbusb_dev_write(priv->dev, &CMD_SET_FREQUENCY, buf);

	ret = dtmbusb_dev_write(priv->dev, &CMD_GET_TUNERED, buf);

	kfree(buf);

	return 0;
}

static void read_snr(struct dvb_frontend *fe)
{
	struct dtmbusb_state *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *fe_prop = &fe->dtv_property_cache;
	s8 ret;
	u8 *buf;

	buf = kmalloc(CMD_GET_SNR.length, GFP_KERNEL);
	if (buf)
	{
		ret = dtmbusb_dev_read(priv->dev, &CMD_GET_SNR, buf);

		if (ret>0 && fe_prop) {
			fe_prop->cnr.stat[0].scale = FE_SCALE_DECIBEL;
			fe_prop->cnr.stat[0].svalue = buf[0] * 1000 + buf[1] * 10;
		}else {
			fe_prop->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
		}
		kfree(buf);
	}
}

static void read_signal_strength(struct dvb_frontend *fe)
{
	struct dtmbusb_state *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *fe_prop = &fe->dtv_property_cache;
	s8 ret;
	u8 *buf;

	buf = kmalloc(CMD_GET_STRENGTH.length, GFP_KERNEL);
	if (buf) {

		ret = dtmbusb_dev_read(priv->dev, &CMD_GET_STRENGTH, buf);

		if (ret>0 && fe_prop) {
			if (priv->dev_type==CVB) {
				fe_prop->strength.stat[0].scale = FE_SCALE_DECIBEL;
				fe_prop->strength.stat[0].uvalue = -buf[3] * 1000;
			} else {
				fe_prop->strength.stat[0].scale = FE_SCALE_RELATIVE;
				fe_prop->strength.stat[0].uvalue = (u64)(buf[3] * 65535 / 100);
			}
		}else {
			fe_prop->strength.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
		}
		kfree(buf);
	}
}

static int dtmbusb_get_frontend(struct dvb_frontend *fe, struct dtv_frontend_properties *props)
{
	struct dtmbusb_state *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *fe_prop = &fe->dtv_property_cache;
	enum fe_transmit_mode ts_mode[] = {TRANSMISSION_MODE_C1, TRANSMISSION_MODE_C3780};	//Transmission mode	("C=1", "C=3780")
	enum fe_guard_interval guard_intervals[] = {GUARD_INTERVAL_PN945, GUARD_INTERVAL_PN595, GUARD_INTERVAL_PN420};	//Guard interval ("PN945","PN595","PN420","NULL-3")
	enum fe_code_rate code_rate[] = {FEC_2_5, FEC_3_5, FEC_4_5};	//Type of Forward Error Correction (FEC)	("0.4","0.6","0.8","NULL-3")
	enum fe_interleaving interleaving[] = {INTERLEAVING_720, INTERLEAVING_240};	//Interleaving	("720","240")
	enum fe_modulation modulation[] = {QAM_4_NR, QPSK, QAM_16, QAM_32, QAM_64};	//Type of modulation/constellation ("4QAM-NR","4QAM","16QAM","32QAM","64QAM","NULL-6","NULL-7")
	enum fe_spectral_inversion spectral_inversion[] = {INVERSION_ON, INVERSION_OFF};	//Type of inversion band ("Phase: Variable","Phase: Fixed")
	s8 ret;
	u8 *buf;

	buf = kmalloc(CMD_GET_SIGNAL_INFO.length, GFP_KERNEL);
	if (buf) {

		ret = dtmbusb_dev_read(priv->dev, &CMD_GET_SIGNAL_INFO, buf);

		if (ret>0 && fe_prop) {
			props->transmission_mode = (buf[0]>=0 & buf[0]<=1) ? ts_mode[buf[0]]:TRANSMISSION_MODE_AUTO;
			props->guard_interval = (buf[1]>=0 & buf[1]<=2) ? guard_intervals[buf[1]]:GUARD_INTERVAL_AUTO;
			props->fec_inner = (buf[2]>=0 & buf[2]<=2) ? code_rate[buf[2]]:FEC_AUTO;
			props->interleaving = (buf[3]>=0 & buf[3]<=2) ? interleaving[buf[3]]:INTERLEAVING_AUTO;
			props->modulation = (buf[4]>=0 & buf[4]<=2) ? modulation[buf[4]]:QAM_AUTO;
			props->inversion = (buf[5]>=0 & buf[5]<=2) ? spectral_inversion[buf[5]]:INVERSION_AUTO;
		}

		kfree(buf);
	}
	
//	read_snr(fe);
//	read_signal_strength(fe);

	return 0;
}

static int dtmbusb_get_tune_settings(struct dvb_frontend *fe,
			      struct dvb_frontend_tune_settings *fesettings)
{
	fesettings->min_delay_ms = 800;
	fesettings->step_size = 0;
	fesettings->max_drift = 0;
	return 0;
}

static int dtmbusb_read_status(struct dvb_frontend *fe,
			       enum fe_status *fe_status)
{
	struct dtmbusb_state *priv = fe->demodulator_priv;
	s8 ret;
	u8 *buf;
	
	*fe_status = 0;

  buf = kmalloc(CMD_GET_HAS_SIGNAL.length, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

//	ret = dtmbusb_dev_read(priv->dev, &CMD_GET_HAS_SIGNAL, buf);

//	if (ret>0 & buf[0]==1)
//		*fe_status |= FE_HAS_LOCK;
		*fe_status = FE_HAS_SIGNAL |
			FE_HAS_CARRIER |
			FE_HAS_VITERBI |
			FE_HAS_SYNC |
			FE_HAS_LOCK;

	kfree(buf);
	read_snr(fe);
	read_signal_strength(fe);
//	info("%s: ret=%d fe_status=0x%X\n", __func__, ret, *fe_status);
	return 0;
}

static int dtmbusb_read_signal_strength(struct dvb_frontend *fe, u16 *signal)
{
  struct dtmbusb_state *priv = fe->demodulator_priv;
  s8 ret;
  u8 *buf;
	u32 strength;

  buf = kmalloc(CMD_GET_STRENGTH.length, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

  ret = dtmbusb_dev_read(priv->dev, &CMD_GET_STRENGTH, buf);

  if (ret>0) {
    strength = buf[3] + ((buf[2] + ((buf[1] + (buf[0] << 8)) << 8)) << 8);
//		*signal = (u16)(strength * 65534 / 100);
		*signal = -buf[3];
	}
	
//	info("%s: ret=%d strength=%d signal=%d buf=0x%X%X%X%X\n", __func__, ret, strength, *signal, buf[0], buf[1], buf[2], buf[3]);

  kfree(buf);
//        info("%s: strength=%d\n", __func__, *signal);
  return 0;
}

static int dtmbusb_read_snr(struct dvb_frontend *fe, u16 *snr)
{
  struct dtmbusb_state *priv = fe->demodulator_priv;
  s8 ret;
  u8 *buf;
	u32 quality;

  buf = kmalloc(CMD_GET_SNR.length, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

    ret = dtmbusb_dev_read(priv->dev, &CMD_GET_SNR, buf);

    if (ret>0) {
  		quality = buf[0] + 3;
		if (quality<70)
			*snr = (u16)(quality * 65535 / 100);
		else
			*snr = 0;
	} else
		*snr = 0;


  kfree(buf);
//        info("%s: snr=%d\n", __func__, *snr);
  return 0;
}

static struct dvb_frontend_ops dtmbusb_ops = {
	.delsys = { SYS_DVBT },
	.info = {
		.name = "DtmbUSB DTMB demodulator",
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
		.frequency_min = 52000000,
		.frequency_max = 866000000,
		.frequency_stepsize = 10000,
#else
		.frequency_min_hz	= 52 * MHz,
		.frequency_max_hz	= 866 * MHz,
		.frequency_stepsize_hz	= 10 * kHz,
#endif
		.caps =
			FE_CAN_FEC_AUTO |
			FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO
	},

	.release = dtmbusb_fe_release,

	.init = dtmbusb_fe_init,

	.set_frontend = dtmbusb_set_frontend,
	.get_frontend = dtmbusb_get_frontend,
//	.get_tune_settings = dtmbusb_get_tune_settings,

	.read_status = dtmbusb_read_status,
//	.read_signal_strength = dtmbusb_read_signal_strength,
//	.read_snr = dtmbusb_read_snr,
};

struct dvb_frontend *dtmbusb_fe_attach(struct dvb_usb_adapter *adap, const struct dtmbusb_config *config)
{
	struct dtmbusb_state *priv = NULL;
	struct dvb_usb_device *dev; 
	int ret;
	u8 *buf;

	priv = kzalloc(sizeof(struct dtmbusb_state), GFP_KERNEL);
	if (priv == NULL)
		return NULL;

	dev = adap->dev;

        buf = kmalloc(CMD_GET_FW_VERSION.length, GFP_KERNEL);
        if (!buf)
                return -ENOMEM;

	ret = dtmbusb_dev_read(dev, &CMD_GET_FW_VERSION, buf);

	dev_info(&dev->udev->dev, "DtmbUSB DTMB demodulator firmware version:%d.%d.%d%d", buf[0],buf[1],buf[2],buf[3]);

	if (buf[1]==0x08 && buf[2]==0x20 && buf[3]==0x44) {
		if (buf[0]==0x03) {
			priv->dev_type = LETV;
			snprintf(dtmbusb_ops.info.name, 128, "Letv DtmbUSB DTMB demodulator");
		} else if (buf[0]==0x05) {
			priv->dev_type = AIWA;
	  		snprintf(dtmbusb_ops.info.name, 128, "Aiwa DtmbUSB DTMB demodulator");
		} else if (buf[0]==0x06) {
			priv->dev_type = CVB;
			snprintf(dtmbusb_ops.info.name, 128, "CVB DtmbUSB DTMB demodulator");
			dtmbusb_dev_write(dev, &CMD_CVB_SEND_KEY, CMD_CVB_SEND_KEY.data);
			kfree(buf);
			buf = kmalloc(CMD_CVB_GET_HASH.length, GFP_KERNEL);
			dtmbusb_dev_read(dev, &CMD_CVB_GET_HASH, buf);
		}
	}

	memcpy(&priv->frontend.ops, &dtmbusb_ops, sizeof(struct dvb_frontend_ops));

	priv->dev = adap->dev;
	priv->frontend.demodulator_priv = priv;

	kfree(buf);

	return &priv->frontend;
}
EXPORT_SYMBOL_GPL(dtmbusb_fe_attach);

MODULE_DESCRIPTION("DtmbUSB frontend driver");
MODULE_AUTHOR("Xiaodong Ni <nxiaodong520@gmail.com>");
MODULE_LICENSE("GPL");

