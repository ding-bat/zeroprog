//this is the code for the USB chip on the Zeroplus

#include "/usr/include/libusb-1.0/libusb.h"
#include <stdio.h>
#include "gl.h"

#define CTRL_IN			(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE)
#define CTRL_OUT		(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT | LIBUSB_RECIPIENT_INTERFACE)
const int PACKET_CTRL_LEN=2;

const int PACKET_INT_LEN=2;
const int PACKET_BULK_LEN=64;
const int INTERFACE=0;
const int ENDPOINT_INT_IN=0x81; /* endpoint 0x81 address for IN */
const int ENDPOINT_INT_OUT=0x01; /* endpoint 1 address for OUT */
const int ENDPOINT_BULK_IN=0x81; /* endpoint 0x81 address for IN */
const int ENDPOINT_BULK_OUT=0x02; /* endpoint 1 address for OUT */
const int TIMEOUT=5000; /* timeout in ms */

enum {
	REQ_READBULK = 0x82,
	REQ_WRITEADDR,
	REQ_READDATA,
	REQ_WRITEDATA,
	REQ_GPIOOE = 0x89,
	REQ_GPIOREAD,
	REQ_GPIOWRITE
};

static struct libusb_device_handle *g_devh = NULL;

int gl_gpio_oe(unsigned int mask)
{
	unsigned char packet[8] = {mask & 0xFF};
	int retval = libusb_control_transfer(g_devh, CTRL_OUT, 0xc, REQ_GPIOOE, 0, packet, 1,TIMEOUT);
	if (retval != 1) printf("%s: libusb_control_transfer returned %d\n", __FUNCTION__, retval);
	return retval;
}

int gl_gpio_read(void)
{
	unsigned char packet[8] = {0};
	int retval = libusb_control_transfer(g_devh, CTRL_IN, 0xc, REQ_GPIOREAD, 0, packet, 1,TIMEOUT);
	if (retval != 1) printf("%s: libusb_control_transfer returned %d\n", __FUNCTION__, retval);
	return retval == 1 ? packet[0] : retval;
}

int gl_gpio_write(unsigned int val)
{
	unsigned char packet[8] = {val & 0xFF};
	int retval = libusb_control_transfer(g_devh, CTRL_OUT, 0xc, REQ_GPIOWRITE, 0, packet, 1,TIMEOUT);
	if (retval != 1) printf("%s: libusb_control_transfer returned %d\n", __FUNCTION__, retval);
	return retval;
}

int gl_write_address(unsigned int address)
{
	unsigned char packet[8] = {address & 0xFF};
	int retval = libusb_control_transfer(g_devh, CTRL_OUT, 0xc, REQ_WRITEADDR, 0, packet, 1,TIMEOUT);
	if (retval != 1) printf("%s: libusb_control_transfer returned %d\n", __FUNCTION__, retval);
	return retval;
}

int gl_write_data(unsigned int val)
{
	unsigned char packet[8] = {val & 0xFF};
	int retval = libusb_control_transfer(g_devh, CTRL_OUT, 0xc, REQ_WRITEDATA, 0, packet, 1,TIMEOUT);
	if (retval != 1) printf("%s: libusb_control_transfer returned %d\n", __FUNCTION__, retval);
	return retval;
}

int gl_read_data(void)
{
	unsigned char packet[8] = {0};
	int retval = libusb_control_transfer(g_devh, CTRL_IN, 0xc, REQ_READDATA, 0, packet, 1,TIMEOUT);
	if (retval != 1) printf("%s: libusb_control_transfer returned %d, val=%hhx\n", __FUNCTION__, retval, packet[0]);
	return retval == 1 ? packet[0] : retval;
}

int gl_read_bulk(void *buffer, unsigned int size)
{
	unsigned char packet[8] = {0, 0, 0, 0, size & 0xff, (size & 0xff00) >> 8, (size & 0xff0000) >> 16, (size & 0xff000000)>>24};
	int transferred = 0;

	int retval = libusb_control_transfer(g_devh, CTRL_OUT, 0x4, REQ_READBULK, 0, packet, 8, TIMEOUT);
	if (retval != 8) printf("%s: libusb_control_transfer returned %d\n", __FUNCTION__, retval);

	retval = libusb_bulk_transfer(g_devh, ENDPOINT_BULK_IN, buffer, size,
								  &transferred, TIMEOUT);
	if (retval < 0) {
		fprintf(stderr, "Bulk read error %d\n", retval);
	}
	return transferred;
}

int gl_reg_write(unsigned int reg, unsigned int val)
{
	int retval;
	retval = gl_write_address(reg);
	if (retval < 0)
		return retval;
	retval = gl_write_data(val);
	return retval;
}

int gl_reg_read(unsigned int reg)
{
	int retval;
	retval = gl_write_address(reg);
	if (retval < 0)
		return retval;
	retval = gl_read_data();
	return retval;
}

int gl_open(int vid)
{
	int r = 1;

	r = libusb_init(NULL);
	if (r < 0)
		return GL_ELIBUSB;

	libusb_set_debug(NULL, 0);

	struct libusb_device **devs;
	struct libusb_device *dev;
	size_t i = 0;

	if (libusb_get_device_list(NULL, &devs) < 0) {
		r = GL_EOPEN;
		goto gl_open_error;
	}

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(dev, &desc) < 0)
			break;

		if (desc.idVendor == vid) {
			if (libusb_open(dev, &g_devh) < 0)
				g_devh = NULL;
			break;
		}
	}

	libusb_free_device_list(devs, 1);

	if (!g_devh) {
		r = GL_EOPEN;
		goto gl_open_error;
	}

	r = libusb_set_configuration(g_devh, 1);
	if (r < 0) {
		r = GL_ESETCONFIG;
		goto gl_open_error;
	}

	r = libusb_claim_interface(g_devh, 0);
	if (r < 0) {
		r = GL_ECLAIM;
		goto gl_open_error;
	}

	return GL_OK;

gl_open_error:
	gl_close();
	return r;
}

int gl_close(void)
{
	if (g_devh) {
		libusb_release_interface(g_devh, 0);
		libusb_reset_device(g_devh);
		libusb_close(g_devh);
	}
	libusb_exit(NULL);

	return 0;
}
