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
	#include <hidapi/hidapi.h>    // this is libusb, see http://libusb.sourceforge.net/
#else
	#include <hidapi/hidapi.h>
#endif


#include "plogger.h"

class usbdev_t
{
public:

	static constexpr const unsigned PACKETSIZE = 64;
	static constexpr const unsigned DEFAULT_TIMEOUT = 1000;

	usbdev_t(logger_t &logger, unsigned idVendor, unsigned idProduct,
		const std::string &sProduct, const std::string &sSerial)
	: m_log(logger)
	, m_devHandle(nullptr)
	{
		if (init(idVendor, idProduct, sProduct, sSerial) > 0)
			cleanup();
	}
	~usbdev_t()
	{
		cleanup();
		if (--reference() == 0)
		{
			hid_exit();
		}
	}

public:
	operator bool() { return (m_devHandle != nullptr); }

	std::string product() { return m_product; }
	std::string serial() { return m_serial; }
	unsigned vendor_id() { return m_vendor_id; }
	unsigned product_id() { return m_product_id; }

	template<typename... Args>
	void log(log_level level, const char *fmt, Args&&... args)
	{
		m_log(level, fmt, std::forward<Args>(args)...);
	}

protected:
	int control_msg_write(int requesttype, int request, int value, int index,
		void *bytes, int size, int timeout)
	{
		auto p(static_cast<const unsigned char *>(bytes));
		// do basic checks
		if ((value & 0xff00) != 0x0300 || (value & 0x00ff) != p[0]
			|| requesttype != 0x21 || request != 0x09 || index != 0)
		{
			m_log(DEBUG, "Consistency check failed for value parameter %04x", value);
			return 1;

		}
		if (int result = hid_send_feature_report(m_devHandle, p, size) < 0)
		{
			m_log(DEBUG, "Error %i writing ctrlmsg to HID device", result);
			return 1;
		}
		return 0;
	}

	int control_msg_read(int requesttype, int request, int value, int index,
		void *bytes, int size, int timeout)
	{
#if 0
		if (int result = usb_control_msg(m_devHandle, requesttype | USB_ENDPOINT_IN,
			request, value, index, static_cast<char *>(bytes), size, timeout) < 0)
		{
			m_log(DEBUG, "Error %i writing ctrlmsg to USB device", result);
			return 1;
		}
#endif
		return 1;
	}

	int write(const void *data, std::size_t len)
	{
		// FIXME: allow different USB_TYPE
		int result(-1);
		unsigned char buf[PACKETSIZE + 1];
		const unsigned char *p = static_cast<const unsigned char *>(data);
		unsigned bs = PACKETSIZE + (p[0] == 0);

		if (bs < len)
		{
			m_log(DEBUG, "HID write: transfer size %d too big", len);
			return 1;
		}
		unsigned i=0;
		while (i < len)
		{
			buf[i] = p[i];
			i++;
		}
		while (i < bs)
			buf[i++] = 0;

		result = hid_write(m_devHandle, buf, bs);
		if (result  < 0)
		{
			m_log(DEBUG, "Error %i writing to HID device", result);
			return 1;
		}
		return 0;
	}

	int write(const std::string &s)
	{
		return write(s.c_str(), s.size() + 1);
	}

	int read(void *data, unsigned len, unsigned timeout = DEFAULT_TIMEOUT)
	{
		int result(-1);

		unsigned char buf[PACKETSIZE];
		unsigned char *p = static_cast<unsigned char *>(data);
		unsigned bs = PACKETSIZE;

		if (bs<len)
		{
			m_log(DEBUG, "HID read: transfer size %d too big", len);
			return 1;
		}
		for (std::size_t i = 0; i < bs; i++)
			buf[i] = 0;

		result = hid_read_timeout(m_devHandle, buf, bs, timeout);

		unsigned i=0;
		while (i < len)
		{
			p[i] = buf[i];
			i++;
		}
		if (result  < bs)
		{
			m_log(DEBUG, "Error %i reading from HID device", result);
			return 1;
		}
		return 0;
	}

	void cleanup()
	{
		if (m_devHandle != nullptr)
		{
			hid_close(m_devHandle);
			m_devHandle = nullptr;
		}
	}

private:

	int init(unsigned idVendor, unsigned idProduct, const std::string &sProduct,
		const std::string &sSerial)
	{
		if (reference()++ == 0)
		{
			m_log(DEBUG, "Initializing HID lib");
			// Initialize the USB library
			hid_init();
		}
		m_devHandle = hid_open(idVendor, idProduct, sSerial.empty() ? nullptr : s_to_ws(sSerial).c_str());
		if(m_devHandle != nullptr)
		{
			static const std::size_t BUFSIZE = 256;
			wchar_t buf[BUFSIZE];

			if (hid_get_product_string(m_devHandle, buf, BUFSIZE) < 0)
			{
				m_log(DEBUG, "unable to get product string");
				return 1;
			}
			m_product = w_to_s(buf);
			if (hid_get_serial_number_string(m_devHandle, buf, BUFSIZE) < 0)
			{
				m_log(DEBUG, "unable to get serial string");
				return 1;
			}
			m_serial = w_to_s(buf);

			m_log(DEBUG, "Found <%s> with serial <%s>", m_product, m_serial);

			m_vendor_id = idVendor;
			m_product_id = idProduct;
			if (sProduct != m_product)
			{
				m_log(DEBUG, "Product Id <%s> does not match requested <%s>", m_product, sProduct);
				return 1;
			}
			if (!sSerial.empty() && sSerial != m_serial)
			{
				m_log(DEBUG, "Serial Id <%s> does not match requested <%s>", m_serial, sProduct);
				return 1;
			}

		}
		else
			return 1;
		return 0;
	}

	std::string w_to_s(wchar_t *s)
	{
		// FIXME: dead wrong
		std::string ret = "";
		/* lossy conversion to ISO Latin1 */
		std::size_t i(0);
		wchar_t c;
		while ((c = s[i]) != 0)
		{
			if (c < 256)
				ret += static_cast<char>(c);
			else
				ret += '?'; /* outside of ISO Latin1 range */
			i++;
		}
		return ret;
	}

	std::wstring s_to_ws(const std::string &s)
	{
		return std::wstring(s.begin(), s.end());
	}

	static int &reference()
	{
		static int m_ref = 0;
		return m_ref;
	}

	logger_t &m_log;
	std::string m_product;
	std::string m_serial;
	unsigned m_vendor_id;
	unsigned m_product_id;
	hid_device *m_devHandle;
};

#endif /* PUSB_H_ */
