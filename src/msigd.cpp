
#include <cstddef>
#include <string>
#include <vector>
#include <type_traits>

#include <stdio.h>
#include <string.h>

#ifndef USE_HID
#define USE_HID 	(1)
#endif


#if USE_HID
#include "phid.h"
#else
#include "pusb.h"
#endif

static const char *appname = "msigd";
static const char *appversion = "0.3";

enum access_t
{
	READ,
	WRITE,
	READWRITE
};

enum encoding_t
{
	ENC_INT,
	ENC_STRING,
	ENC_STRINGINT,
	ENC_STRINGPOS,
	ENC_INTPOS
};

using string_list = std::vector<std::string>;

static unsigned msi_stou(std::string s, int base)
{
	unsigned res = 0;
	for (std::size_t i=0; i < s.size(); i++)
	{
		unsigned b = static_cast<unsigned>(base < 0 ?
			(i < s.size() - 1 ? static_cast<unsigned>(256u - '0') :
				static_cast<unsigned>(-base)) : static_cast<unsigned>(base));
		res = res * b + (static_cast<unsigned>(s[i]) - '0');
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

template<typename T>
static std::string to_hex(const T *buf, std::size_t len)
{
	auto *p = reinterpret_cast<const unsigned char *>(&buf[0]);
	std::string out("");
	for (std::size_t i = 0; i < len * sizeof(T); i++)
	{
		char b[32];
		snprintf(b, 32, "%02x ", p[i]);
		out += b;
	}
	return out;
}

static std::string to_hex(const std::string &s)
{
	return to_hex(s.c_str(), s.size());
}

template<typename... Args>
static void pprintf(const char *fmt, Args&&... args)
{
	printf(fmt, log_helper(args)...);
}

template<typename... Args>
static void eprintf(const char *fmt, Args&&... args)
{
	fprintf(stderr, fmt, log_helper(args)...);
}


struct setting_t
{
	setting_t(std::string cmd, std::string opt)
	: m_access(READ), m_enc(ENC_STRING), m_cmd(cmd), m_opt(opt)
	{ }

	setting_t(std::string cmd, std::string opt, unsigned min, unsigned max)
	: m_access(READWRITE), m_enc(ENC_INT), m_cmd(cmd), m_opt(opt), m_min(min), m_max(max)
	{ }

	setting_t(std::string cmd, std::string opt, unsigned min, unsigned max, int base)
	: m_access(READWRITE), m_enc(ENC_INT), m_cmd(cmd), m_opt(opt), m_min(min), m_max(max), m_base(base)
	{ }

	setting_t(std::string cmd, std::string opt, string_list values)
	: m_access(READWRITE)
	, m_enc(ENC_STRINGINT)
	, m_cmd(cmd)
	, m_opt(opt)
	, m_values(values)
	, m_min(0)
	, m_max(static_cast<unsigned>(values.size()))
	{ }

	setting_t(access_t access, std::string cmd, std::string opt, string_list values)
	: m_access(access)
	, m_enc(ENC_STRINGINT)
	, m_cmd(cmd)
	, m_opt(opt)
	, m_values(values)
	, m_min(0)
	, m_max(static_cast<unsigned>(values.size()))
	{ }

	std::string encode(std::string val)
	{
		if (m_enc == ENC_INT)
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
			}
		}
		else if (m_enc == ENC_STRINGINT)
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
		if (m_enc == ENC_STRING)
			return val;
		if (m_enc == ENC_INT)
		{
			unsigned v = msi_stou(val, m_base);
			char buf[100];
			std::snprintf(buf, 100, "%d", v);
			return buf;
		}
		else if (m_enc == ENC_STRINGINT)
		{
			std::size_t eidx(0);
			auto v = std::stoul(val, &eidx, 10);
			if (eidx != val.size())
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
	string_list m_values;
	unsigned m_min = 0;
	unsigned m_max = 100;
	int m_base = 10;
};

// MPG341CQR:  3DA0
// MAG321CURV  3DA2
// MAG322CQRV  3DA4

static std::vector<setting_t> settings =
{
	setting_t("00100", "power", {"off", "on"}),  // returns 001
	setting_t("00110", "unknown02"),  // returns 000 called frequently by OSD app, readonly
	setting_t(READ, "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "reader", "cinema", "designer"}),
	//setting_t("00160", "unknown0x"),  // query kills monitor side
	setting_t("00170", "frequency"), // returns 060
	//setting_t("00180", "unknown0x"),  // returns 56006
	setting_t("00200", "game_mode", {"user", "fps", "racing", "rts", "rpg"}),  // returns 000
	setting_t("00210", "unknown06", 0, 100, -100),  // returns "00:" but can only be set to 000 to 009 - no visible effect
	setting_t("00220", "response_time", {"normal", "fast", "fastest"}),  // returns 000 0:normal, 1:fast, 2:fastest
	// FIXME: anti-motion blur?
	setting_t("00230", "enable_dynamic", {"on", "off"}),  // returns 000 - on/off only ==> on disables ZL and HDCR in OSD
	setting_t("00240", "hdcr", {"off", "on"}),  // returns 000
	setting_t("00250", "refresh_rate_display", {"off", "on"}),  // returns 000
	setting_t("00251", "refresh_rate_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),  // returns 003 0: LT, 1: RT, 2:LB, 3:RB
	setting_t("00260", "alarm_clock", {"off", "1", "2", "3", "4"}),  // returns 000 0:OFF,1..4 alarm clock
	setting_t("00261", "alarm_clock_index", 1, 4),  // returns 000
	setting_t("00262", "alarm_clock_time", 0, 99*60+59, -60),  // returns 000
	setting_t("00263", "alarm_clock_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),  // returns 000
	setting_t("00270", "screen_assistance", 0, 12),  // returns 000, value: '0' + mode, max: "<"
	// FIXME: adaptive sync ? game-mode only
	setting_t("00280", "unknown08"),  // returns 000, read only, write fails and monitor needs off/on cycle
	setting_t("00290", "zero_latency", {"off", "on"}),  // returns 001
	// FIXME: MPG341CQR: Auto, 4:3, 16:9, 21:9, 1:1
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
	setting_t("00434", "rgb", 0, 100*100*100, 100),  // returns bbb  -> value = 'b' - '0' = 98-48=50
	setting_t("00435", "unknown09"),  // returns 000, read only
	setting_t(WRITE, "00440", "unknown10", {"off", "on"}),
	//setting_t("00440", "unknown10", 0, 100),
	setting_t("00500", "input",  {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 002  -> 0=hdmi1, 1=hdmi2, 2=dp, 3=usbc
	setting_t("00600", "pip", {"off", "pip", "pbp"}),  // returns 000 0:off, 1:pip, 2:pbp
	setting_t("00610", "pip_input", {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 000 0=hdmi1, 1=hdmi2, 2=dp, 3=usbc    FIXME:Verify this
	setting_t("00620", "pbp_input", {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 000 0=hdmi1, 1=hdmi2, 2=dp, 3=usbc    FIXME:Verify this
	setting_t("00630", "pip_size", {"small", "medium", "large"}),
	setting_t("00640", "pip_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),
	setting_t(WRITE, "00650", "toggle_display", {"off", "on"}),  // returns 56006
	setting_t(WRITE, "00660", "toggle_sound", {"off", "on"}),  // returns 56006, but used to toggle audio in app, no response packet - only works with "1"
	setting_t("00800", "osd_language", 0, 19, -100),  // returns 001 -> value = '0' + language, 0 chinese, 1 English, 2 French, 3 German, ... maximum value "C"
	setting_t("00810", "osd_transparency", 0, 5),  // returns 000
	setting_t("00820", "osd_timeout",0, 30),  // returns 020
	// FIXME: MPG341CQR has an USB quick charge switch 830?
	setting_t(WRITE, "00840", "reset", {"off", "on"}),  // returns 56006 - reset monitors
	setting_t("00850", "sound_enable", {"off", "on"}),  // returns 001 - digital/anlog as on some screenshots?
	setting_t("00860", "back_rgb", {"off", "on"}),  // returns 001
	setting_t("00900", "navi_up", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),  // returns 006
	setting_t("00910", "navi_down", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),  // returns 003
	setting_t("00920", "navi_left", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),  // returns 004
	setting_t("00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),  // returns 005
};

static std::vector<std::pair<setting_t,std::string>> special =
{
	{ setting_t("00140", "unknown03"), "00;" }, // returns 00; called frequently by OSD app, readonly
	{ setting_t("00150", "unknown01"), "V18" }, // returns V18, readonly
	{ setting_t("00130", "unknown04"), std::string(13, ' ') }, // returns 13 blanks
	{ setting_t("00190", "unknown05"), "56006" } // returns 56006
};

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

static int mystic_opt(std::string opt, led_data &leds)
{
	if (opt == "off")
		leds.mode = 0;
	else if (opt == "static")
		leds.mode = 1;
	else if (opt == "breathing")
		leds.mode = 2;
	else if (opt == "blinking")
		leds.mode = 3;
	else if (opt == "flashing")
		leds.mode = 5;
	else if (opt == "blinds")
		leds.mode = 6;
	else if (opt == "meteor")
		leds.mode = 8;
	else if (opt == "rainbow")
		leds.mode = 0x1a;
	else if (opt == "random")
		leds.mode = 0x1f;
	else if (opt.size() == 8 && opt.substr(0, 2) == "0x")
	{
		leds.mode = 1;
		std::size_t idx(0);
		auto v = std::stoul(opt.substr(2), &idx, 16);
		if (idx != opt.size() - 2)
			return 1;
		leds.set_rgb(v >> 4, (v >> 2) & 0xff, v & 0xff);
	}
	else
	{
		leds.mode = 1;
		unsigned r,g,b;
		if (3 != sscanf(opt.c_str(),"%d,%d,%d", &r, &g, &b))
			return 1;
		if (r>255 || g>255 || b>255)
			return 1;
		leds.set_rgb(r, g, b);
	}
	return 0;
}

class mondev_t : public usbdev_t
{
public:
	mondev_t(logger_t &logger, unsigned idVendor, unsigned idProduct, const std::string &sProduct)
	: usbdev_t(logger, idVendor, idProduct, sProduct)
	{
#if !(USE_HID)
		if (!checkep(1, false) && !checkep(2, true))
			return;
		else
			cleanup();
#endif
	}

	~mondev_t()
	{
	}

	int write_led(led_data &data)
	{
		return control_msg_write(0x21, 0x09, 0x371, 0,
			&data, static_cast<int>(sizeof(led_data)), 1000);
	}

	int write_string(const std::string &s)
	{
		std::string s1 = "\001" + s;
		return write(s1);
	}

	int set_setting(const setting_t &setting, std::string &s)
	{
		log(DEBUG, "Setting %s to %s", setting.m_opt, s);
		auto err = write_command(std::string("5b") + setting.m_cmd + s);
		if (!err)
		{
			auto ret = read_return();
			if (ret != "5600+")
			{
				log(DEBUG, "Got unexpected return <%s>", ret);
				return 1;
			}
			return 0;
		}
		else
			return err;
	}

	int get_setting(const setting_t &setting, std::string &s)
	{
		std::string cmd(std::string("58") + setting.m_cmd);
		auto err = write_command(cmd);
		if (!err)
		{
			auto ret = read_return();
			// 260 (alarm clock) does not properly set the return buffer
			if (ret.size() > cmd.size()
				&& (ret.substr(0, cmd.size()) == std::string("5b") + setting.m_cmd
				    || cmd == "5800260"))
			{
				s = ret.substr(cmd.size());
				return 0;
			}
			else
			{
				s = ret;
				return 1;
			}
		}
		else
			return err;
	}

	void debug_cmd(const std::string &cmd)
	{
		auto err = write(cmd);
		if (!err)
		{
			unsigned char buf[64] = {0, 0};
			if (read(buf, 64))
				log(DEBUG,"Error receiving %s: %d", to_hex(cmd), err);
			else
			{
				std::string out("");
				for (int i = 0; i < 16; i++)
				{
					char b[32];
					snprintf(b, 32, "%02x ", buf[i]);
					out += b;
				}
				log(DEBUG,"Special %s: %s", to_hex(cmd), to_hex(buf, 16));
			}
		}
		else
			log(DEBUG,"Error sending %s: %d", to_hex(cmd), err);
	}

private:

	int write_command(std::string prefix)
	{
		std::string cmd("\001" + prefix + "\r");
		return write(cmd);
	}

	std::string read_return()
	{
		char buf[64] = {0, 0};
		if (read(buf, 64))
			return "";
		//skip 0x01 at beginning and cut off "\r"
		std::string ret("");
		for (int i=1; i<64; i++)
		{
			// skip 0
			if (buf[i] == '\r')
				break;
			else if (buf[i] != 0)
				ret += buf[i];
		}
		return ret;
#if 0
		std::string ret(buf+1);
		if (ret.size() > 0 && ret[ret.size()-1] == '\r')
			return ret.substr(0,ret.size()-1);
		else
			return ret;
#endif
	}

};

static int help()
{
	pprintf(
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
	pprintf("%s",
		"      --mystic               off, static, breathing, blinking, flashing, \n"
		"                               blinds, meteor, rainbow, random, \n"
		"                               0xRRGGBB, RRR,GGG,BBB\n");
	for (auto &s : settings)
		if (s.m_access == WRITE || s.m_access == READWRITE)
		{
			pprintf("      --%-20s values: ", s.m_opt);
			if (s.m_enc == ENC_STRINGINT)
			{
				for (auto &v : s.m_values)
					pprintf("%s ", v);
			}
			else
				pprintf("%d to %d", s.m_min, s.m_max);
			pprintf("\n");
		}
	pprintf("%s",
		"  -d, --debug                enable debug output\n"
		"  -h, --help                 display this help and exit\n"
		"      --version              output version information and exit\n"
		"\n"
		"Options are processed in the order they are given. You may specify an option\n"
		"more than once with identical or different values.\n"
		"\n"
		"In addition to preset modes the --mystic option also accepts numeric\n"
		"values. 0xff0000 will set all leds to red. '0,255,0' will set all leds\n"
		"to green.\n"
		"\n"
		"Exit status:\n"
		" 0  if OK,\n"
		" 1  if error during option parsing,\n"
		" 2  if error during device access,\n"
		"\n"
		"Report bugs on <https://github.com/couriersud/msigd/issues>\n"
		"msigd home page: <https://github.com/couriersud/msigd>\n"
	);

	return 0;
}

static int version()
{
	pprintf("%s %s\n"
		"Copyright (C) 2019 Couriersud\n"
		"License GPLv2: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n"
		"\n"
		"Written by Couriersud\n", appname, appversion);
	return 0;
}

template<typename... Args>
static int error(int err, const char *fmt, Args&&... args)
{
	fprintf(stderr, "%s: ", appname);
	fprintf(stderr, fmt, log_helper(args)...);
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
	led_data leds;
	bool mystic = false;

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
		else if (cur_opt == "--mystic" && arg_pointer + 1 < argc)
		{
			if (mystic_opt(argv[++arg_pointer], leds) != 0)
				return error(1, "Mystic light parameter error: %s", argv[arg_pointer]);
			mystic = true;
		}
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
							return error(1, "Unknown value <%s> for option %s", argv[arg_pointer], cur_opt);
						set.emplace_back(&s, val);
						found = true;
						break;
					}
				}
				if (!found)
					return error(1, "Unknown option: %s", cur_opt);
			}
			else
				return error(1, "Unknown option: %s", cur_opt);
		}
		arg_pointer++;
	}

	std_logger_t logger;
	logger.set_level(DEBUG, debug);

	mondev_t usb(logger, 0x1462, 0x3fa4, "MSI Gaming Controller");

	if (usb)
	{
		bool notify(false);
		for (auto &setting : special)
		{
			std::string res;
			/* ignore return */ usb.get_setting(setting.first, res);
			if (res != setting.second)
			{
				eprintf("Unexpected %s : %s\n", setting.first.m_opt, setting.first.decode(res));
				notify = true;
			}
		}
		if (notify)
		{
			eprintf("Detected an unknown monitor. Please report the output of\n"
				    "'msigd --info --debug --query' as an issue and also provide\n"
					"the ID (MAG...) of your monitor. Thank you!\n");
		}

		if (info)
		{
			pprintf("Vendor Id:  0x%04x\n", usb.vendor_id());
			pprintf("Product Id: 0x%04x\n", usb.product_id());
			pprintf("Product:    %s\n",     usb.product());
			pprintf("Serial:     %s\n",     usb.serial());
			if (debug)
			{
				usb.debug_cmd("\x01\xb0");
				usb.debug_cmd("\x01\xb4");
				// set after MSI app 20191206 0.0.2.23
				//usb.debug_cmd("\x01\xd0""b00100000\r");
				// queried but not supported on MAG321CURV (returns 56006)
				usb.debug_cmd("\x01""5800190\r");
				//usb.debug_cmd("\x01""5800130\r");
			}
		}
		// query first
		if (query)
		{
			for (auto &setting : settings)
				if (setting.m_access==READWRITE || setting.m_access==READ)
				{
					std::string res;
					if (!usb.get_setting(setting, res))
						pprintf("%s : %s\n", setting.m_opt, setting.decode(res));
					else
					{
						error(0, "Error querying device on %s - got <%s>", setting.m_opt, res);
						if (!debug)
							return 2;
					}
				}
		}

		if (mystic)
			usb.write_led(leds);

		// set values
		for (auto &s : set)
			if (usb.set_setting(*s.first, s.second))
				return error(2, "Error setting --%s", s.first->m_opt);

	}
	else
		return error(1, "No usb device found", 0);

	return 0;
}
