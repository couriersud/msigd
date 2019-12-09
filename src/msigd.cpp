
#include <cstddef>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>


#if defined WIN
  #include <lusb0_usb.h>    // this is libusb, see http://libusb.sourceforge.net/
#else
  #include <usb.h>        // this is libusb, see http://libusb.sourceforge.net/
#endif

#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/usbdevice_fs.h>
#endif

static const char *appname = "msigd";
static const char *appversion = "0.1";

enum access_t
{
	READ,
	WRITE,
	READWRITE
};

enum encoding_t
{
	INT,
	STRING,
	STRINGINT,
	STRINGPOS,
	INTPOS
};

using string_list = std::vector<std::string>;

static unsigned msi_stou(std::string s, int base)
{
	unsigned res = 0;
	unsigned b = static_cast<unsigned>(base < 0 ? -base : base);

	for (auto &c : s)
	{
		res = res * b + (static_cast<unsigned>(c) - '0');
		if (base < 0)
			b = 256 - '0';
	}
	return res;
}

static std::string msi_utos(unsigned v, int base, int width)
{
	std::string res = "";
	unsigned b = static_cast<unsigned>(base < 0 ? -base : base);
	for (int i = 0; i<width; i++)
	{
		res = std::string("") + static_cast<char>((v % b) + '0') + res;
		v = v / b;
		if (base < 0)
			b = 256 - '0';
	}
	return res;
}

struct setting_t
{
	setting_t(std::string cmd, std::string opt)
	: m_access(READ), m_enc(STRING), m_cmd(cmd), m_opt(opt)
	{ }

	setting_t(std::string cmd, std::string opt, unsigned min, unsigned max)
	: m_access(READWRITE), m_enc(INT), m_cmd(cmd), m_opt(opt), m_min(min), m_max(max)
	{ }

	setting_t(std::string cmd, std::string opt, unsigned min, unsigned max, int base)
	: m_access(READWRITE), m_enc(INT), m_cmd(cmd), m_opt(opt), m_min(min), m_max(max), m_base(base)
	{ }

	setting_t(std::string cmd, std::string opt, string_list values)
	: m_access(READWRITE), m_enc(STRINGINT), m_cmd(cmd), m_opt(opt), m_min(0),
	  m_max(static_cast<unsigned>(values.size())), m_values(values)
	{ }

	setting_t(access_t access, std::string cmd, std::string opt, string_list values)
	: m_access(access), m_enc(STRINGINT), m_cmd(cmd), m_opt(opt), m_min(0),
	  m_max(static_cast<unsigned>(values.size())), m_values(values)
	{ }

	std::string encode(std::string val)
	{
		if (m_enc == INT)
		{
			char *eptr;
			unsigned long v = std::strtoul(val.c_str(), &eptr, 10);
			if (*eptr != 0)
				return ""; // FIXME - must be checked by caller!
			else if (v<m_min || v > m_max)
				return ""; // FIXME - must be checked by caller!
			else
			{
				return msi_utos(static_cast<unsigned>(v), m_base, 3);
				//char buf[100];
				//snprintf(buf, 100, "%03d", v);
				//return buf;
			}
		}
		else if (m_enc == STRINGINT)
		{
			for (std::size_t i=0; i < m_values.size(); i++)
				if (m_values[i] == val)
				{
					char buf[100];
					snprintf(buf, 100, "%03d", static_cast<unsigned>(i));
					return buf;
				}
			return "";
		}
		else
			return "";
	}

	std::string decode(std::string val)
	{
		if (m_enc == STRING)
			return val;
		if (m_enc == INT)
		{
			// char *eptr;
			//int v = strtol(val.c_str(), &eptr, 10);
			unsigned v = msi_stou(val, m_base);
			if (v < m_min || v > m_max)
				return ""; // FIXME - must be checked by caller!
			else
			{
				char buf[100];
				std::snprintf(buf, 100, "%d", v);
				return buf;
			}
		}
		else if (m_enc == STRINGINT)
		{
			char *eptr;
			auto v = strtoul(val.c_str(), &eptr, 10);
			if (*eptr != 0)
				return ""; // FIXME - must be checked by caller!
			else if (v<m_min || v > m_max)
				return ""; // FIXME - must be checked by caller!
			return m_values[v];
		}
		else
			return "";
	}

	access_t m_access;
	encoding_t m_enc;
	std::string m_cmd;
	std::string m_opt;
	unsigned m_min = 0;
	unsigned m_max = 100;
	int m_base = 10;
	unsigned m_ret_start = 7;
	string_list m_values;
};

static std::vector<setting_t> settings =
{
	// FIXME: missing: RGB LED On/Off, alarm clock

	setting_t("00100", "unknown00"),  // returns 001
	setting_t("00110", "unknown02"),  // returns 000 called frequently by OSD app
	setting_t(READ, "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "reader", "cinema", "designer"}),
	setting_t("00130", "unknown04"),  // returns 13 blanks
	setting_t("00140", "unknown03"),  // returns 00; called frequently by OSD app
	setting_t("00150", "unknown01"),  // returns V18
	//setting_t("00160", "unknown05"),  // returns incomplete packet - kills communication
	setting_t("00170", "frequency"), // returns 060
	setting_t("00200", "game_mode", {"user", "fps", "racing", "rts", "rpg"}),  // returns 000
	setting_t("00220", "response_time", {"normal", "fast", "fastest"}),  // returns 000 0:normal, 1:fast, 2:fastest
	setting_t("00240", "hdcr", {"off", "on"}),  // returns 000
	setting_t("00250", "refresh_rate_display", {"off", "on"}),  // returns 000
	setting_t("00251", "refresh_rate_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),  // returns 003 0: LT, 1: RT, 2:LB, 3:RB
	setting_t("00260", "alarm_clock", {"off", "1", "2", "3", "4"}),  // returns 000 0:OFF,1..4 alarm clock
	setting_t("00261", "alarm_clock_index", 1, 4),  // returns 000
	setting_t("00262", "alarm_clock_time", 0, 99*60+59, -60),  // returns 000
	setting_t("00263", "alarm_clock_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),  // returns 000
	setting_t("00270", "screen_assistance", 0, 12),  // returns 000, value: '0' + mode, max: "<"
	setting_t("00280", "unknown08"),  // returns 000
	setting_t("00290", "zero_latency", {"off", "on"}),  // returns 001
	setting_t("002:0", "screen_size", {"19", "24", "4:3", "16:9"}),  // returns 003 -> 19",24",4:3,16:9
	setting_t("002;0", "night_vision", {"off", "normal", "strong", "strongest", "ai"}),  // returns 002 0:OFF, 1:Normal,2:Strong,3:Strongest,4:A.I.
	setting_t("00300", "pro_mode", {"user", "reader", "cinema", "designer"}),  // returns 000
	setting_t("00310", "eye_saver", {"off", "on"}),  // returns 000
	setting_t("00340", "image_enhancement", {"off","weak","medium","strong","strongest"}),
	setting_t("00400", "brightness", 0, 100),  // returns 048
	setting_t("00410", "contrast", 0, 100),  // returns 050
	setting_t("00420", "sharpness", 0, 5),  // returns 000
	setting_t("00430", "color_preset", {"cool", "normal", "warm", "custom"}),  // returns 001
	setting_t("00431", "red", 0, 100),
	setting_t("00432", "green", 0, 100),
	setting_t("00433", "blue", 0, 100),
	//setting_t("00434", "rgb"),  // returns bbb  -> value = 'b' - '0' = 98-48=50
	setting_t("00500", "input",  {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 002  -> 0=hdmi1, 1=hdmi2, 2=dp, 3=usbc
	setting_t("00600", "pip", {"off", "pip", "pbp"}),  // returns 000 0:off, 1:pip, 2:pbp
	setting_t("00610", "pip_input", {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 000 0=hdmi1, 1=hdmi2, 2=dp, 3=usbc    FIXME:Verify this
	setting_t("00620", "pbp_input", {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 000 0=hdmi1, 1=hdmi2, 2=dp, 3=usbc    FIXME:Verify this
	setting_t("00630", "pip_size", {"small", "medium", "large"}),
	setting_t("00640", "pip_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),
	setting_t("00800", "osd_language", 0, 19, -100),  // returns 001 -> value = '0' + language, 0 chinese, 1 English, 2 French, 3 German, ... maximum value "C"
	setting_t("00810", "osd_transparency", 0, 5),  // returns 000
	setting_t("00820", "osd_timeout",0, 30),  // returns 020
	setting_t("00850", "unknown26"),  // returns 001
	setting_t("00900", "navi_up", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),  // returns 006
	setting_t("00910", "navi_down", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),  // returns 003
	setting_t("00920", "navi_left", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),  // returns 004
	setting_t("00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),  // returns 005
};

#if 0
large:
0040   01 35 62 30 30 31 67 31 31 30 32 33 30 30 30 30   .5b001g110230000
0050   30 30 30 30 30 30 30 30 30 30 0d 00 00 00 00 00   0000000000......
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
middle:
0040   01 35 62 30 30 31 67 31 31 30 31 33 30 30 30 30   .5b001g110130000
0050   30 30 30 30 30 30 30 30 30 30 0d 00 00 00 00 00   0000000000......
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................


pbp:
0040   01 35 62 30 30 31 67 32 30 30 31 33 30 30 30 30   .5b001g200130000
0050   30 30 30 30 30 30 30 30 30 30 0d 00 00 00 00 00   0000000000......
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................

alarm clock:
0040   01 35 62 30 30 31 66 3f 32 4e 30 5d 30 6c 30 31   .5b001f?2N0]0l01
0050   30 30 30 30 30 30 30 30 30 30 0d 00 00 00 00 00   0000000000......
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................

0040   01 35 62 30 30 31 66 3f 32 4e 30 5d 30 6c 30 31   .5b001f?2N0]0l01
0050   30 30 30 30 30 30 30 30 30 30 0d 00 00 00 00 00   0000000000......
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................

//sent on mode switch - no response packet is sent
0040   01 35 62 30 30 31 68 ff 30 30 30 30 30 30 30 30   .5b001hÿ00000000
0050   30 30 30 30 30 30 30 30 30 0d 00 00 00 00 00 00   000000000.......
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................


//sent on mode switch - response packet "5600+"
0040   01 35 62 30 30 31 65 ff 31 ff 30 30 30 30 30 30   .5b001eÿ1ÿ000000
0050   ff 30 31 80 6c 32 31 62 62 62 0d 00 00 00 00 00   ÿ01.l21bbb......
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................

// sent on PIP start - response packet "56006"
0040   01 35 38 30 30 31 39 30 0d 00 00 00 00 00 00 00   .5800190........
0050   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................


// RGBs at back - these are control messages to endpoint 0
// all blue
// data
0040   71 01 00 00 00 01 64 00 00 00 00 00 01 00 00 00   q.....d.........
0050   01 64 00 00 00 00 00 ff 00 00 ff 00 00 ff 00 00   .d.....ÿ..ÿ..ÿ..
0060   ff 00 00 ff 00 00 ff 00 00 ff 00 00 ff 00 00 ff   ÿ..ÿ..ÿ..ÿ..ÿ..ÿ
0070   00 00 01 01 ff 01 01 ff 01 01 ff 01 01 ff 01 01   ....ÿ..ÿ..ÿ..ÿ..
0080   ff 01 01 ff 01 01 ff 01 01 ff 01 01 ff 00         ÿ..ÿ..ÿ..ÿ..ÿ.

#endif

struct led_data
{
	led_data()
	{
		for (int i=0; i<9; i++)
		{
			f12[i*3 + 0] = 0xff;
			f12[i*3 + 1] = 0x00;
			f12[i*3 + 2] = 0x00;

			rgb[i*3 + 0] = 0xff;  // red
			rgb[i*3 + 1] = 0xff;  // green
			rgb[i*3 + 2] = 0xff;  // blue
		}
	}

	void set_rgb(uint8_t r, uint8_t g, uint8_t b)
	{
		for (int i=0; i<9; i++)
		{
			rgb[i*3 + 0] = r;
			rgb[i*3 + 1] = g;
			rgb[i*3 + 2] = b;
		}
	}
	// total size should be 78
	//0040   71 01 00 00 00 01 64 00 00 00 00 00 01 00 00 00   q.....d.........
	uint16_t f00 = 0x0171;
	uint16_t f01 = 0x0000;
	uint16_t f02 = 0x0100;
	uint16_t f03 = 0x0064;
	uint16_t f04 = 0x0000;
	uint16_t f05 = 0x0000;
	uint16_t mode = 0x0001; // Mode
	uint16_t f07 = 0x0000;
	//0050   01 64 00 00 00 00 00 ff 00 00 ff 00 00 ff 00 00   .d.....ÿ..ÿ..ÿ..
	uint16_t f08 = 0x6401;
	uint16_t f09 = 0x0000;
	uint16_t f10 = 0x0000;
	uint8_t  f11 = 0x00;
	uint8_t  f12[9*3];
	uint8_t  rgb[9*3];
	uint8_t  f13 = 0x00;
};

// Mode:
// Off: 0
// Static: 0x01
// Game: 0x01
// Blind : 0x06
// Meteor : 0x08
// Rainbow : 0x1a
// Flash : 0x05
// Breath : 0x02
// Blink : 0x03
// Random : 0x1f
// Synched: 0x01

enum log_level
{
	DEBUG,
	VERBOSE,
	WARNING,
	ERROR
};

class logger_t
{
public:
	logger_t()
	: m_enabled{false,false,true,true}
	{
	}

	virtual ~logger_t() = default;

	void log(log_level level, const char *s)
	{
		if (m_enabled[level])
			vlog(level_str(level) + ": " + s);
	}

	void log(log_level level, std::string s)
	{
		if (m_enabled[level])
			vlog(level_str(level) + ": " + s);
	}

	template<typename... Args>
	void log(log_level level, const char *fmt, Args&&... args)
	{
		if (m_enabled[level])
		{
			char buf[1024];
			std::snprintf(buf, 1024, fmt, std::forward<Args>(args)...);
			vlog(level_str(level) + ": " + buf);
		}
	}

	void set_level(log_level level, bool val) { m_enabled[level] = val; }

protected:
	virtual void vlog(std::string msg) = 0;

	std::string level_str(log_level level)
	{
		switch (level)
		{
			case DEBUG:
				return "DEBUG";
			case VERBOSE:
				return "VERBOSE";
			case WARNING:
				return "WARNING";
			case ERROR:
				return "ERROR";
		}
		return ""; // please compiler
	}

	bool m_enabled[4];
};

class std_logger_t : public logger_t
{
public:
	std_logger_t()
	{
	}

protected:
	void vlog(std::string msg) override
	{
		printf("%s\n", msg.c_str());
	}
};

class usbdev_t
{
public:
	usbdev_t(logger_t &logger, unsigned idVendor, unsigned idProduct, const std::string &sProduct)
	: m_log(logger), m_device(nullptr), m_devHandle(nullptr), m_interface(nullptr), m_detached(false)
	{
		if (init(idVendor, idProduct, sProduct) > 0)
			cleanup();
	}
	~usbdev_t()
	{
		cleanup();
		if (--reference() == 0)
		{

		}
	}

	int write_led(led_data &data)
	{
		if (int result = usb_control_msg(m_devHandle, 0x21, 0x09, 0x371, 0,
			reinterpret_cast<char *>(&data), static_cast<int>(sizeof(led_data)), 1000) < 0)
		{
			m_log.log(DEBUG, "Error %i writing ctrlmsg to USB device", result);
			return 1;
		}
		return 0;
	}

	int write_string(const std::string &s)
	{
		std::string s1 = "\001" + s;
		const int len=static_cast<int>(s1.length());

#if 0
		for (int i=0; i<len; i++)
		{
			DEBUGL("Writing character \"%c\" to DigiSpark.\n", s[i]);
			//CHECKERR_RET(1, usb_control_msg(m_devHandle, (0x01 << 5), 0x09, 0, s[i], 0, 0, 1000), "Error %i writing to USB device\n", result);
			CHECKERR_RET(1, usb_interrupt_write(m_devHandle, (0x01 << 5), 0x09, 0, s[i], 0, 0, 1000), "Error %i writing to USB device\n", result);
		}
#else
		if (int result = usb_interrupt_write(m_devHandle, 2, s1.c_str(), len+1, 1000) < 0)
		{
			m_log.log(DEBUG, "Error %i writing to USB device", result);
			return 1;
		}
		return 0;
#endif
	}

#if 0
	std::string read_string(int num_char, char eol, bool ign13=true)
	{
		char x;
		std::string r;
		int n = 0;
		while (true)
		{
			int err = 0;
			CHECKERR_NORET(err, usb_control_msg(m_devHandle, (0x01 << 5) | 0x80, 0x01, 0, 0, &x, 1, 1000), "Error %i reading from USB device\n", err);
			if (x==eol || err < 0)
				return r;
			else if ((x != '\r' && ign13) || !ign13)
			{
				r += x;
				n++;
			}
			if (n >= num_char)
			{
				return r;
			}
		}
	}
#endif

	int set_setting(const setting_t &setting, std::string &s)
	{
		//read_return();
		auto err = write_command(std::string("5b") + setting.m_cmd + s);
		if (!err)
		{
			auto ret = read_return();
			return ret == "";
		}
		else
			return err;
	}

	int get_setting(const setting_t &setting, std::string &s)
	{
		auto err = write_command(std::string("58") + setting.m_cmd);
		if (!err)
		{
			auto ret = read_return();
			if (ret != "")
			{
				if (ret.size() <= setting.m_ret_start)
				{
					s = ret;
					return 0;
				}
				s = ret.substr(setting.m_ret_start);
				return 0;
			}
			else
				return 1;
		}
		else
			return err;
	}


	operator bool() { return (m_device != nullptr) && (m_devHandle != nullptr); }

	std::string product() { return m_product; }
	std::string serial() { return m_serial; }
	unsigned vendor_id() { return m_vendor_id; }
	unsigned product_id() { return m_product_id; }

private:

	int write_command(std::string prefix)
	{
		char buf[256] = "\001";
		std::snprintf(buf + 1, 255, "%s\r", prefix.c_str());
		//printf("command %s\n", buf+1);
		if (int result = usb_interrupt_write(m_devHandle, 2, buf, static_cast<int>(strlen(buf)+1), 1000) < 0)
		{
			m_log.log(DEBUG, "Error %i writing to USB device\n", result);
			return 1;
		}
		return 0;
	}

	std::string read_return()
	{
		char buf[256] = "";
		if (int result = usb_interrupt_read(m_devHandle, 1, buf, 64, 1000) < 0)
		{
			m_log.log(DEBUG, "Error %i reading from USB device\n", result);
			return "";
		}
		//skip 0x01 at beginning and cut off "\r"
		std::string ret(buf+1);
		if (ret[ret.size()-1] == '\r')
			return ret.substr(0,ret.size()-1);
		else
			return ret;
	}

	void cleanup()
	{
		if (m_devHandle != nullptr)
		{
			m_log.log(DEBUG, "Releasing interface %d\n", m_interface->bInterfaceNumber);

			if (int err = usb_release_interface(m_devHandle, m_interface->bInterfaceNumber) < 0)
				m_log.log(DEBUG, "Error %d releasing Interface \n", err);
#if 1
#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
			if (m_detached)
			{
				// FIXME: BIG HACK - this assumes the dev handle has the file descriptor as first member
				int fd = *reinterpret_cast<int *>(m_devHandle);
				struct usbdevfs_ioctl command;
				command.ifno = m_interface->bInterfaceNumber;
				command.ioctl_code = USBDEVFS_CONNECT;
				command.data = nullptr;
				if (ioctl(fd, USBDEVFS_IOCTL, &command) < 0)
					m_log.log(DEBUG, "reattach kernel driver failed: %s", strerror(errno));
			}
#endif
#endif
			usb_close(m_devHandle);
			m_devHandle = nullptr;
		}
#if 0
#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
		// This code does not work on Ubuntu 16.04. USBDEVFS_CONNECT has to be issued before the descriptor is closed.
		if (m_detached)
		{
			std::string filename = std::string("/dev/bus/usb/") + m_device->bus->dirname + "/" + m_device->filename;
			m_log.log(DEBUG, "device usb path: %s", filename.c_str());
			if (int fd = open(filename.c_str(), O_RDONLY) > 0)
			{
				struct usbdevfs_ioctl command;
				command.ifno = m_interface->bInterfaceNumber;
				command.ioctl_code = USBDEVFS_CONNECT;
				command.data = nullptr;
				if (ioctl(fd, USBDEVFS_IOCTL, &command) < 0)
					m_log.log(DEBUG, "reattach kernel driver failed: %s", strerror(errno));
				close(fd);
			}
			else
				m_log.log(DEBUG, "Error reattaching device: %s not found", filename.c_str());
		}
#endif
#endif
		m_device = nullptr;
	}

	int init(unsigned idVendor, unsigned idProduct, const std::string &sProduct)
	{
		if (reference()++ == 0)
		{
			m_log.log(DEBUG, "Initializing usb_lib");
			// Initialize the USB library
			usb_init();
			// Enumerate the USB device tree
			usb_find_busses();
			usb_find_devices();
		}
		m_device = find_device(idVendor, idProduct, sProduct.c_str());
		if (m_device != nullptr)
		{
			m_devHandle = usb_open(m_device);

			if(m_devHandle != nullptr)
			{
				int numInterfaces = m_device->config->bNumInterfaces;
				m_interface = &(m_device->config->interface[0].altsetting[0]);
				m_log.log(DEBUG, "Found %i interfaces, using interface %d", numInterfaces, m_interface->bInterfaceNumber);
				m_log.log(DEBUG, "Setting Configuration");
				if (usb_set_configuration(m_devHandle, m_device->config->bConfigurationValue) < 0)
				{
#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
					char buf[1024];
					if (usb_get_driver_np( m_devHandle, m_interface->bInterfaceNumber, buf, sizeof(buf))==0)
						m_log.log(DEBUG, "Driver is %s", buf);

					if (int err=usb_detach_kernel_driver_np( m_devHandle, m_interface->bInterfaceNumber) < 0)
					{
						m_log.log(DEBUG, "failed to detach kernel driver from USB device: %d", err);
						return 1;
					}
					m_detached = true;
					if (int err = usb_set_configuration(m_devHandle, m_device->config->bConfigurationValue) < 0)
					{
						m_log.log(DEBUG, "Error %i setting configuration to %i\n", err, m_device->config->bConfigurationValue);
						return 1;
					}
#else
					m_log.log(DEBUG, "Error %i setting configuration to %i\n", err, m_device->config->bConfigurationValue);
					return 1;
#endif
				}

				if (int err = usb_claim_interface(m_devHandle, m_interface->bInterfaceNumber) < 0)
				{
					m_log.log(DEBUG, "Error %i claiming Interface %i\n", err, m_interface->bInterfaceNumber);
				}
			}
			else
				return 1;
		}
		return 0;
	}

	/* https://stackoverflow.com/questions/31119014/open-a-device-by-name-using-libftdi-or-libusb */
	int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid, char *buf, int buflen) {
		char buffer[256];
		int rval, i;

		// make standard request GET_DESCRIPTOR, type string and given index
		// (e.g. dev->iProduct)
		rval = usb_control_msg(dev,
			USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid,
			buffer, sizeof(buffer), 1000);

		if (rval < 0) // error
			return rval;

		// rval should be bytes read, but buffer[0] contains the actual response size
		const auto response_size(static_cast<uint8_t>(buffer[0]));
		if (response_size < rval)
			rval = response_size; // string is shorter than bytes read

		if (buffer[1] != USB_DT_STRING) // second byte is the data type
			return 0; // invalid return type

		// we're dealing with UTF-16LE here so actual chars is half of rval,
		// and index 0 doesn't count
		rval /= 2;

		/* lossy conversion to ISO Latin1 */
		for (i = 1; i < rval && i < buflen; i++)
		{
			if (buffer[2 * i + 1] == 0)
				buf[i - 1] = buffer[2 * i];
			else
				buf[i - 1] = '?'; /* outside of ISO Latin1 range */
		}
		buf[i - 1] = 0;

		return i - 1;
	}

	struct usb_device *find_device(unsigned idVendor, unsigned idProduct, const char *sProduct)
	{
		struct usb_bus *bus = nullptr;
		struct usb_device *device = nullptr;

		m_log.log(DEBUG, "Scanning USB devices...");

		// Iterate through attached busses and devices
		for (bus = usb_get_busses(); bus != nullptr; bus = bus->next)
		{
			for (device = bus->devices; device != nullptr; device = device->next)
			{
			// Check to see if each USB device matches the DigiSpark Vendor and Product IDs
				if((device->descriptor.idVendor == idVendor) && (device->descriptor.idProduct == idProduct))
				{
					usb_dev_handle * handle = nullptr;
					/* we need to open the device in order to query strings */
					if (!(handle = usb_open(device)))
					{
						m_log.log(DEBUG, "cannot open USB device: %s", usb_strerror());
						continue;
					}
					/* get product name */
					char buf[256] = "";
					if (usbGetDescriptorString(handle, device->descriptor.iProduct, 0x0409, buf, sizeof(buf)) < 0)
					{
						m_log.log(DEBUG, "cannot query product for device: %s\n", usb_strerror());
						continue;
					}
					m_product = buf;
					m_log.log(DEBUG, "Found device %s \n", m_product.c_str());

					buf[0] = 0;
					if (usbGetDescriptorString(handle, device->descriptor.iSerialNumber, 0x0409, buf, sizeof(buf)) < 0)
					{
						m_log.log(DEBUG, "cannot query serial for device: %s\n", usb_strerror());
					}
					m_serial = buf;

					usb_close(handle);
					m_vendor_id = idVendor;
					m_product_id = idProduct;

					if (m_product == sProduct)
						return device;
				}
			}
		}
		return nullptr;
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
	struct usb_device *m_device;
	struct usb_dev_handle *m_devHandle;
	struct usb_interface_descriptor *m_interface;
	bool m_detached;
};

static int help()
{
	printf(
		"Usage: %s [OPTION]... \n"
		"Query or set monitor settings by usb directly on the device.\n"
		"For supported devices please refer to the documentation.\n"
		"\n"
		"  -q, --query                display all monitor settings. This will also\n"
		"                               list readonly settings and settings whose\n"
		"                               function is currently unknown.\n"
		"      --info                 display device information. This can be used\n"
		"                               with --query\n"
		, appname);
	for (auto &s : settings)
		if (s.m_access == WRITE || s.m_access == READWRITE)
		{
			printf("      --%-20s ", s.m_opt.c_str());
			if (s.m_enc == STRINGINT)
			{
				for (auto &v : s.m_values)
					printf("%s ", v.c_str());
			}
			else
				printf("%d to %d", s.m_min, s.m_max);
			printf("\n");
		}
	printf("%s",
		"  -d, --debug                enable debug output\n"
		"  -h, --help                 display this help and exit\n"
		"      --version              output version information and exit\n"
		"\n"
		"Options are processed in the order they are given. You may specify an option\n"
		"more than once with identical or different values.\n"
		"\n"
		"Exit status:\n"
		" 0  if OK,\n"
		" 1  if error during option parsing,\n"
		" 2  if error during device access,\n"
		"\n"
	);

	return 0;
}

static int version()
{
	printf("%s %s\n"
		"Copyright (C) 2019 Andre Hufschmidt\n"
		"License GPLv2: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n"
		"\n"
		"Written by Andre Hufschmidt\n", appname, appversion);
	return 0;
}

template<typename... Args>
int error(int err, std::string fmt, Args&&... args)
{
	fprintf(stderr, "%s: ", appname);
	fprintf(stderr, fmt.c_str(), std::forward<Args>(args)...);
	fprintf(stderr, "\n");
	fprintf(stderr, "Try '%s --help' for more information.\n", appname);
	return err;
}

int main (int argc, char **argv)
{
	int arg_pointer = 1;
	bool query = false;
	bool debug = false;
	bool info = false;

	std::vector<std::pair<setting_t *, std::string>> set;

	while (arg_pointer < argc)
	{
		std::string cur_opt(argv[arg_pointer]);

		if (cur_opt == "--help" || cur_opt == "-h")
			return help();
		else if (cur_opt == "--version")
			return version();
		else if (cur_opt == "--info")
			info = true;
		else if (cur_opt == "--debug" || cur_opt == "-d")
			debug = true;
		else if (cur_opt == "--query" || cur_opt == "-q")
			query = true;
		else
		{
			if (cur_opt.size() >= 3 && arg_pointer + 1 < argc && cur_opt.substr(0,2) == "--")
			{
				auto search(cur_opt.substr(2));
				bool found(false);
				for (auto &s : settings)
				{
					if (s.m_opt == search && (s.m_access==READWRITE || s.m_access==WRITE))
					{
						std::string val = s.encode(argv[++arg_pointer]);
						if (val == "")
							return error(1, "Unknown value <%s> for option %s", argv[arg_pointer], cur_opt.c_str());
						set.emplace_back(&s, val);
						found = true;
						break;
					}
				}
				if (!found)
					return error(1, "Unknown option: %s", cur_opt.c_str());
			}
			else
				return error(1, "Unknown option: %s", cur_opt.c_str());
		}
		arg_pointer++;
	}

	std_logger_t logger;
	logger.set_level(DEBUG, debug);

	usbdev_t usb(logger, 0x1462, 0x3fa4, "MSI Gaming Controller");

	if (usb)
	{
		if (info)
		{
			printf("Vendor Id:  0x%04x\n", usb.vendor_id());
			printf("Product Id: 0x%04x\n", usb.product_id());
			printf("Product:    %s\n",     usb.product().c_str());
			printf("Serial:     %s\n",     usb.serial().c_str());
		}
		// query first
		if (query)
		{
			for (auto &setting : settings)
			{
				std::string res;
				if (!usb.get_setting(setting, res))
					printf("%s : %s\n", setting.m_opt.c_str(), setting.decode(res).c_str());
				else
					return error(2, "Error querying device on %s", setting.m_opt.c_str());
			}
		}

#if 0
		led_data test;

		//test.set_rgb(0xFF, 0xFF, 0x00);
		test.set_rgb(0x00, 0x00, 0xff);
		usb.write_led(test);
#endif

		// if we want to set a value, do it now
		for (auto &s : set)
			if (usb.set_setting(*s.first, s.second))
				return error(2, "Error setting --%s", s.first->m_opt.c_str());

#if 0
		usb.get_brightness();
		usb.get_pip();
		usb.set_brightness(80);
		usb.set_pip(3);
		usb.set_pip_off();

		usb.write_string(output);
		if (sendLine)
			usb.write_string("\n");
		//usleep(10000);
		while (1)
		{
			std::string s = usb.read_string(255, '\n');
			if (s == "")
			{
				//printf("TimeOut\n");
				//break;
			}
			else if (exit_on != "" && s.substr(0,exit_on.size()) == exit_on)
				break;
			else
			{
				printf("%s\n", s.c_str(), s.size());
//				printf("Got <%s> %d\n", s.c_str(), s.size());
				fflush(stdout);
			}
			usleep(10000);
		}
#endif
	}
	else
		return error(1, "No usbdevice found", 0);

	return 0;
}
