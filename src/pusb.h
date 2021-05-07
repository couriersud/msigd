/*
 * pusb.h
 *
 */

#ifndef PUSB_H_
#define PUSB_H_

#include <string>
#include <array>

#if defined(_WIN32)
  //#include <usb0_usb.h>    // this is libusb, see http://libusb.sourceforge.net/
	#include <windows.h>
	#include <libusb-compat/usb.h>    // this is libusb, see http://libusb.sourceforge.net/
#else
	#include <usb.h>        // this is libusb, see http://libusb.sourceforge.net/
	#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
		#include <sys/types.h>
		#include <sys/stat.h>
		#include <sys/ioctl.h>
		#include <errno.h>
		#include <fcntl.h>
		#ifndef __APPLE__
			#include <linux/usbdevice_fs.h>
		#endif
	#endif
#endif

#include <string.h> // strerror

#include "pcommon.h"
#include "plogger.h"

class usbdev_t
{
public:

	static constexpr const unsigned DEFAULT_TIMEOUT = 1000;

	usbdev_t(logger_t &logger, const device_info &info,
		const std::string &sProduct, const std::string &sSerial)
	: m_log(logger)
	, m_device(nullptr)
	, m_handle(nullptr)
	, m_interface(nullptr)
	, m_ep{nullptr}
	, m_buf{nullptr}
	, m_ep_out(0)
	, m_ep_in(0)
	, m_detached(false)
	{
		if (init(info, sProduct, sSerial) > 0)
			cleanup();
	}
	~usbdev_t()
	{
		cleanup();
	}

	operator bool() { return (m_device != nullptr) && (m_handle != nullptr); }

	std::string product() { return m_product; }
	std::string manufacturer() { return m_manufacturer; }
	std::string serial() { return m_serial; }
	unsigned vendor_id() { return m_vendor_id; }
	unsigned product_id() { return m_product_id; }

	template<typename... Args>
	void log(log_level level, const char *fmt, Args&&... args)
	{
		m_log(level, fmt, std::forward<Args>(args)...);
	}

	static int get_device_list(logger_t &logger, unsigned idVendor, unsigned idProduct,
		device_info_list &list)
	{
		build_device_list(logger, idVendor, idProduct);
		list = m_list;

		// FIXME: let caller handle this
		if (!list.empty())
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

protected:
	int control_msg_write(int requesttype, int request, int value, int index,
		void *bytes, int size, int timeout)
	{
		auto p(static_cast<char *>(bytes));

		unsigned hidreport0 = ((value & 0xff00) == 0x0300 && (value & 0x00ff) == p[0]
			&& p[0] == 0 && requesttype == 0x21 && request == 0x09 && index == 0);

		if (int result = usb_control_msg(m_handle, requesttype | USB_ENDPOINT_OUT,
			request, value, index, p + hidreport0, size - hidreport0, timeout) < 0)
		{
			m_log(DEBUG, "Error %i writing ctrlmsg to USB device", result);
			return 1;
		}
		return 0;
	}

	int control_msg_read(int requesttype, int request, int value, int index,
		void *bytes, int size, int timeout)
	{
		if (int result = usb_control_msg(m_handle, requesttype | USB_ENDPOINT_IN,
			request, value, index, static_cast<char *>(bytes), size, timeout) < 0)
		{
			m_log(DEBUG, "Error %i writing ctrlmsg to USB device", result);
			return 1;
		}
		return 0;
	}

	int write(unsigned ep, const void *data, std::size_t len, unsigned timeout = DEFAULT_TIMEOUT)
	{
		// FIXME: allow different USB_TYPE
		int result(-1);

		if (ep_is_type(ep, USB_ENDPOINT_TYPE_INTERRUPT))
		{
			auto bs = ep_buf_size(ep);
			if (bs<len)
			{
				m_log(DEBUG, "Endpoint %i: transfer size %d too big", ep, len);
				return 1;
			}
			unsigned i=0;
			while (i < len)
			{
				m_buf[ep][i] = static_cast<const char *>(data)[i];
				i++;
			}
			if (is_class(USB_CLASS_HID))
			{
				while (i < bs)
					m_buf[ep][i++] = 0;
				len = bs;
			}
			result = usb_interrupt_write(m_handle,
				static_cast<int>(USB_ENDPOINT_OUT | USB_TYPE_STANDARD | ep),
				m_buf[ep], static_cast<int>(len), static_cast<int>(timeout));
		}
		else if (ep_is_type(ep, USB_ENDPOINT_TYPE_BULK))
			result = usb_bulk_write(m_handle,
				static_cast<int>(USB_ENDPOINT_OUT | USB_TYPE_STANDARD | ep),
				static_cast<const char *>(data), static_cast<int>(len),
				static_cast<int>(timeout));
		else
		{
			m_log(DEBUG, "Endpoint %i: unsupported type %02x", ep, m_ep[ep]->bmAttributes);
			return 1;
		}
		if (result  < 0)
		{
			m_log(DEBUG, "Error %i writing to USB device: %s", result, usb_strerror());
			return 1;
		}
		return 0;
	}

	int write(unsigned ep, const std::string &s, unsigned timeout = DEFAULT_TIMEOUT)
	{
		return write(ep, s.c_str(), s.size() + 1, timeout);
	}

	// use auto detected endpoint or ep 0
	int write(const void *data, std::size_t len, unsigned timeout = DEFAULT_TIMEOUT)
	{
		return write(m_ep_out, data, len, timeout);
	}

	int write(const std::string &s, unsigned timeout = DEFAULT_TIMEOUT)
	{
		return write(m_ep_out, s.c_str(), s.size() + 1, timeout);
	}

	int read(unsigned ep, void *data, unsigned len, unsigned timeout = DEFAULT_TIMEOUT)
	{
		int result(-1);

		if (ep_is_type(ep, USB_ENDPOINT_TYPE_INTERRUPT))
		{
			auto bs = ep_buf_size(ep);
			if (bs<len)
			{
				m_log(DEBUG, "Endpoint %i: transfer size %d too big", ep, len);
				return 1;
			}
			auto rlen(is_class(USB_CLASS_HID) ? bs : len);
			for (std::size_t i = 0; i < rlen; i++)
				m_buf[ep][i] = 0;
			result = usb_interrupt_read(m_handle,
				static_cast<int>(USB_ENDPOINT_IN | USB_TYPE_STANDARD | ep),
				m_buf[ep], static_cast<int>(rlen), static_cast<int>(timeout));
			unsigned i=0;
			while (i < len)
			{
				static_cast<char *>(data)[i] = m_buf[ep][i];
				i++;
			}
		}
		else if (ep_is_type(ep, USB_ENDPOINT_TYPE_BULK))
			result = usb_bulk_read(m_handle,
				static_cast<int>(USB_ENDPOINT_IN | USB_TYPE_STANDARD | ep),
				static_cast<char *>(data), static_cast<int>(len),
				static_cast<int>(timeout));
		else
		{
			m_log(DEBUG, "Endpoint %i: unsupported type %02x", ep, m_ep[ep]->bmAttributes);
			return 1;
		}
		if (result  < static_cast<int>(len))
		{
			m_log(DEBUG, "Error %i reading from USB device", result);
			return 1;
		}
		return 0;
	}

	// use auto detected endpoint or ep 0
	int read(void *data, unsigned len, unsigned timeout = DEFAULT_TIMEOUT)
	{
		return read(m_ep_in, data, len, timeout);
	}

	int checkep(unsigned ep, bool write)
	{
		auto *p = m_ep[ep];
		if (!p)
		{
			m_log(DEBUG, "Endpoint %i does not exist", ep);
			return 1;
		}
		if (write && ((p->bEndpointAddress & USB_ENDPOINT_DIR_MASK)== USB_ENDPOINT_OUT))
		{
			return 0;
		}
		if (!write && ((p->bEndpointAddress & USB_ENDPOINT_DIR_MASK)== USB_ENDPOINT_IN))
		{
			return 0;
		}
		m_log(DEBUG, "Endpoint %i does is not of type %s", ep, write ? "output" : "input");
		return 1;
	}

	void cleanup()
	{
		if (m_handle != nullptr)
		{
			for (std::size_t i=0; i <= USB_ENDPOINT_ADDRESS_MASK; i++)
				if (m_buf[i])
					delete[] m_buf[i];

			m_log(DEBUG, "Releasing interface %d\n", m_interface->bInterfaceNumber);

			if (int err = usb_release_interface(m_handle, m_interface->bInterfaceNumber) < 0)
				m_log(DEBUG, "Error %d releasing Interface \n", err);

#if !defined(__APPLE__) && !defined(_WIN32) && defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
			if (m_detached)
			{
				// FIXME: BIG HACK - this assumes the dev handle has the file descriptor as first member
				int fd = *reinterpret_cast<int *>(m_handle);
				struct usbdevfs_ioctl command;
				command.ifno = m_interface->bInterfaceNumber;
				command.ioctl_code = USBDEVFS_CONNECT;
				command.data = nullptr;
				if (ioctl(fd, USBDEVFS_IOCTL, &command) < 0)
					m_log(DEBUG, "reattach kernel driver failed: %s", strerror(errno));
			}
#endif
			usb_close(m_handle);
			m_handle = nullptr;
		}
#if 0
#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
		// This code does not work on Ubuntu 16.04. USBDEVFS_CONNECT has to be issued before the descriptor is closed.
		if (m_detached)
		{
			std::string filename = std::string("/dev/bus/usb/") + m_device->bus->dirname + "/" + m_device->filename;
			m_log(DEBUG, "device usb path: %s", filename.c_str());
			if (int fd = open(filename.c_str(), O_RDONLY) > 0)
			{
				struct usbdevfs_ioctl command;
				command.ifno = m_interface->bInterfaceNumber;
				command.ioctl_code = USBDEVFS_CONNECT;
				command.data = nullptr;
				if (ioctl(fd, USBDEVFS_IOCTL, &command) < 0)
					m_log(DEBUG, "reattach kernel driver failed: %s", strerror(errno));
				close(fd);
			}
			else
				m_log(DEBUG, "Error reattaching device: %s not found", filename.c_str());
		}
#endif
#endif
		m_device = nullptr;
	}

private:

	static void build_device_list(logger_t &logger, unsigned idVendor, unsigned idProduct)
	{
		if (!m_initialized)
		{
			usb_init();
			// Enumerate the USB device tree
			usb_find_busses();
			usb_find_devices();
			m_initialized = 1;
			// FIXME: use atexit to regist usb_exit
			struct usb_bus *bus = nullptr;
			struct usb_device *device = nullptr;

			logger(DEBUG, "Scanning USB devices...");

			// Iterate through attached busses and devices
			for (bus = usb_get_busses(); bus != nullptr; bus = bus->next)
			{
				for (device = bus->devices; device != nullptr; device = device->next)
				{
					// Check to see if each USB device matches the Vendor and Product ID
					if((device->descriptor.idVendor == idVendor) && (device->descriptor.idProduct == idProduct))
					{
						//printf("bus port %s %d\n", bus->dirname, device->devnum);
						device_info entry;
						entry.path = device->filename;
						entry.idVendor = idVendor;
						entry.idProduct = idProduct;

						entry.dev = static_cast<void *>(device);

						entry.release_number = 0;
						m_list.push_back(entry);
					}
				}
			}
		}
	}


	int init(const device_info &info, const std::string &sProduct,
		const std::string &sSerial, int interface_class = USB_CLASS_HID)
	{
		m_device = find_device(info, sProduct, sSerial);
		if (m_device != nullptr)
		{
			int numInterfaces = m_device->config->bNumInterfaces;
			// Iterate over interfaces
			m_interface = nullptr;
			for (int i=0; i < numInterfaces; i++)
			{
				for (int j=0; j < m_device->config->interface[i].num_altsetting; j++)
				{
					m_log(DEBUG,"Interface %d alt %d class %d", i, j,
						m_device->config->interface[i].altsetting[j].bInterfaceClass);
					if (m_device->config->interface[i].altsetting[j].bInterfaceClass == interface_class)
					{
						m_interface = &(m_device->config->interface[i].altsetting[j]);
						for (int k = 0; k< m_interface->bNumEndpoints; k++)
						{
							auto ep(m_interface->endpoint[k].bEndpointAddress  & USB_ENDPOINT_ADDRESS_MASK);
							auto mps(m_interface->endpoint[k].wMaxPacketSize);
							m_log(DEBUG,"Endpoint %02x %04x", ep, mps);
							m_ep[ep] = &m_interface->endpoint[k];
							m_buf[ep] = new char[mps];
							if (!checkep(ep, true) && !m_ep_out)
								m_ep_out = ep;
							if (!checkep(ep, false) && !m_ep_in)
								m_ep_in = ep;

						}
						break;
					}
				}
				if (m_interface)
					break;
			}
			if (m_interface)
			{
				m_log(DEBUG, "Found %i interfaces, using interface %d", numInterfaces, m_interface->bInterfaceNumber);
				m_log(DEBUG, "Setting Configuration");
				if (int err = usb_set_configuration(m_handle, m_device->config->bConfigurationValue) < 0)
				{
#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
					char buf[1024];
					if (usb_get_driver_np( m_handle, m_interface->bInterfaceNumber, buf, sizeof(buf))==0)
						m_log(DEBUG, "Driver is %s", buf);

					if ((err=usb_detach_kernel_driver_np( m_handle, m_interface->bInterfaceNumber)) < 0)
					{
						m_log(DEBUG, "failed to detach kernel driver from USB device: %d", err);
						return 1;
					}
					m_detached = true;
					if ((err = usb_set_configuration(m_handle, m_device->config->bConfigurationValue)) < 0)
					{
						m_log(DEBUG, "Error %i setting configuration to %i", err, m_device->config->bConfigurationValue);
						return 1;
					}
#else
					m_log(DEBUG, "Error %i setting configuration to %i", err, m_device->config->bConfigurationValue);
					return 1;
#endif
				}
				if (int err = usb_claim_interface(m_handle, m_interface->bInterfaceNumber) < 0)
				{
					m_log(DEBUG, "Error %i claiming Interface %i: %s", err, m_interface->bInterfaceNumber, usb_strerror());
					return 1;
				}
				return 0;
			}
			else
				return 1;
		}
		return 1;
	}

	bool is_class(int iclass)
	{
		return m_interface->bInterfaceClass == iclass;
	}

	unsigned ep_buf_size(unsigned ep)
	{
		return m_ep[ep]->wMaxPacketSize;
	}

	int ep_is_type(unsigned ep, unsigned type)
	{
		// we assume ep existence has been checked!
		return ((m_ep[ep]->bmAttributes & USB_ENDPOINT_TYPE_MASK) == type);
	}

	/* https://stackoverflow.com/questions/31119014/open-a-device-by-name-using-libftdi-or-libusb */
	static int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid, std::string &ret) {
		// make standard request GET_DESCRIPTOR, type string and given index
		// (e.g. dev->iProduct)
		int rval;
		char buffer[64];
#if 1
		rval = usb_control_msg(dev,
			USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid,
			buffer, sizeof(buffer), 1000);
		if (rval < 0) // error
			return 1;

		// rval should be bytes read, but buffer[0] contains the actual response size
		const auto response_size(static_cast<uint8_t>(buffer[0]));
		if (response_size < rval)
			rval = response_size; // string is shorter than bytes read

		if (buffer[1] != USB_DT_STRING) // second byte is the data type
			return 1; // invalid return type

		// we're dealing with UTF-16LE here so actual chars is half of rval,
		// and index 0 doesn't count
		rval /= 2;
		ret = "";
		/* lossy conversion to ISO Latin1 */
		for (std::size_t i = 1; i < static_cast<std::size_t>(rval) && i < sizeof(buffer); i++)
		{
			if (buffer[2 * i + 1] == 0)
				ret += buffer[2 * i];
			else
				ret += '?'; /* outside of ISO Latin1 range */
		}

		return 0;
#else
		rval = usb_get_string_simple(dev, index, buffer, sizeof(buffer));
		if (rval >= 0)
		{
			ret = buffer;
			return 0;
		}
		else
			return 1;
#endif
	}

	struct usb_device *find_device(const device_info &info,
		const std::string &sProduct, const std::string &sSerial)
	{
		//struct usb_bus *bus = nullptr;
		//struct usb_device *device = nullptr;

		build_device_list(m_log, info.idVendor, info.idProduct);

		for (auto &d : m_list)
		{
			// Check to see if each USB device matches the Vendor and Product ID
			if((d.idVendor == info.idVendor) && (d.idProduct == info.idProduct))
			{
				auto *device = static_cast<struct usb_device *>(d.dev);
				// check the path
				if (!info.path.empty() && info.path != device->filename)
					continue;

				/* we need to open the device in order to query strings */
				if (!(m_handle = usb_open(device)))
				{
					m_log(DEBUG, "cannot open USB device: %s", usb_strerror());
					continue;
				}
				/* get product name */

				if (usbGetDescriptorString(m_handle, device->descriptor.iProduct, 0x0409, m_product))
				{
					m_log(DEBUG, "cannot query product for device: %s", usb_strerror());
					continue;
				}

				if (usbGetDescriptorString(m_handle, device->descriptor.iSerialNumber, 0x0409, m_serial))
				{
					m_log(DEBUG, "cannot query serial for device: %s", usb_strerror());
			 	}

				if (usbGetDescriptorString(m_handle, device->descriptor.iManufacturer, 0x0409, m_manufacturer))
				{
					m_log(DEBUG, "cannot query manufacturer for device: %s", usb_strerror());
			 	}

				m_log(DEBUG, "Found device <%s> with serial <%s> \n", m_product, m_serial);

				if (m_product == sProduct && (sSerial.empty() || sSerial == m_serial))
				{

					/* get product name */
					m_vendor_id = d.idVendor;
					m_product_id = d.idProduct;
					return device;
				}
				usb_close(m_handle);
				m_handle = nullptr;
			}
		}
		return nullptr;
	}

	static int m_initialized;
	static device_info_list m_list;

	logger_t &m_log;
	std::string m_product;
	std::string m_serial;
	std::string m_manufacturer;
	unsigned m_vendor_id;
	unsigned m_product_id;
	struct usb_device *m_device;
	struct usb_dev_handle *m_handle;
	struct usb_interface_descriptor *m_interface;
	std::array <struct usb_endpoint_descriptor *, USB_ENDPOINT_ADDRESS_MASK+1> m_ep;
	std::array <char *, USB_ENDPOINT_ADDRESS_MASK+1> m_buf;
	unsigned m_ep_out;
	unsigned m_ep_in;
	bool m_detached;
};

int usbdev_t::m_initialized = 0;
device_info_list usbdev_t::m_list = {};

#endif /* PUSB_H_ */
