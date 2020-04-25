
#include <cstddef>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <type_traits>
#include <chrono>
#include <thread>

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
static const char *appversion = "0.9";

static const unsigned cMAX_ALARM = 99 * 60 + 59;

static const auto cQUERY_DELAY = std::chrono::milliseconds(25);
static const auto cWAIT_DELAY = std::chrono::milliseconds(250);

enum access_t
{
	READ,
	WRITE,
	READWRITE
};

enum series_t
{
	UNKNOWN = 0x0000,
	MAG32   = 0x0001,
	MAG241  = 0x0002,
	MAG271  = 0x0004,
	MAG272  = 0x0008,
	PS      = 0x0010,

	MAG     = MAG32 | MAG272 | MAG271 | MAG241,
	ALL     = MAG | PS,
};

static series_t operator | (series_t a, series_t b)
{
	return static_cast<series_t>(static_cast<int>(a) | static_cast<int>(b));
}

enum error_e
{
	E_OK = 0,
	E_SYNTAX = 1,
	E_IDENTIFY = 2,
	E_SETTING = 3,
	E_QUERY = 4
};

struct identity_t
{
	series_t    series;
	std::string p140;
	std::string p150;
	std::string name;
	bool        has_mystic;
};

static std::vector<identity_t> known_models =
{
	{ UNKNOWN, "", "", "Unknown", false },
	{ MAG32, "00;", "V18", "MAG32 Series", true },
	{ MAG241, "002", "V18", "MAG241 Series", true },
	{ MAG271, "006", "V19", "MAG271 Series", true },
	{ MAG272, "00O", "V18", "MAG272 Series", true },
	{ PS,  "00?", "V06", "PS Series", false }
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

static string_list splitstr(const std::string &s, char sc)
{
	string_list ret;
	std::string v;
	for (auto &c : s)
	{
		if (c == sc)
		{
			ret.push_back(v);
			v = "";
		}
		else
			v += c;
	}
	ret.push_back(v);
	return ret;
}

template <typename T>
std::vector<T> number_list(const string_list &sl, int base = 10)
{
	std::vector<T> ret;

	for (auto &s : sl)
	{
		std::size_t idx(0);
		auto v = std::stoul(s, &idx, base);
		if (idx != s.size())
			return ret;
		ret.push_back(static_cast<T>(v));
	}
	return ret;
}

static unsigned msi_stou(std::string s, std::size_t *idx, int base)
{
	static const char c0 = '0';
	int res = 0;
	for (std::size_t i=0; i < s.size(); i++)
	{
		char c = s[i];
		int b = (base < 0 ?
			(i < s.size() - 1 ? 256u - c0 :
				-base) : base);
		int v = c - c0;
		if (c < c0 || v >= b)
		{
			*idx = i;
			return res;
		}
		res = res * b + v;
	}
	*idx = s.size();
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
static std::string to_hex(const T *buf, std::size_t len, std::string sep = " ")
{
	auto *p = reinterpret_cast<const unsigned char *>(&buf[0]);
	std::string out("");
	for (std::size_t i = 0; i < len * sizeof(T); i++)
	{
		char b[32];
		snprintf(b, 32, "%02x", p[i]);
		out += b;
		if ( i != len * sizeof(T) - 1)
			out += sep;
	}
	return out;
}

static std::string hexify_if_not_printable(const std::string &str)
{
	bool printable(true);

	for (auto &c : str)
	{
		if (c<32 || c > 126)
		{
			printable = false;
			break;
		}
	}
	if (printable)
		return str;
	return "0x" + to_hex(str.c_str(), str.length(), "");
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
	setting_t(series_t series, std::string cmd, std::string opt)
	: m_series(series), m_access(READ), m_enc(ENC_STRING), m_cmd(cmd), m_opt(opt)
	{ }

	setting_t(series_t series, std::string cmd, std::string opt, unsigned min, unsigned max)
	: m_series(series), m_access(READWRITE), m_enc(ENC_INT), m_cmd(cmd), m_opt(opt), m_min(min), m_max(max)
	{ }

	setting_t(series_t series, std::string cmd, std::string opt, unsigned min, unsigned max, int base)
	: m_series(series), m_access(READWRITE), m_enc(ENC_INT), m_cmd(cmd), m_opt(opt), m_min(min), m_max(max), m_base(base)
	{ }

	setting_t(series_t series, std::string cmd, std::string opt, const string_list &values)
	: m_series(series)
	, m_access(READWRITE)
	, m_enc(ENC_STRINGINT)
	, m_cmd(cmd)
	, m_opt(opt)
	, m_values(values)
	, m_min(0)
	, m_max(static_cast<unsigned>(values.size()))
	{ }

	setting_t(series_t series, std::string cmd, std::string opt, int base, const string_list &values)
	: setting_t(series, cmd, opt, values)
	{
		m_base = base;
	}

	setting_t(series_t series, access_t access, std::string cmd, std::string opt, const string_list &values)
	: m_series(series)
	, m_access(access)
	, m_enc(ENC_STRINGINT)
	, m_cmd(cmd)
	, m_opt(opt)
	, m_values(values)
	, m_min(0)
	, m_max(static_cast<unsigned>(values.size()-1))
	{ }

	setting_t(setting_t &) = delete;
	setting_t(setting_t &&) = delete;

	setting_t *set_access(access_t access) { m_access = access; return this; }

	virtual ~setting_t() { }

	virtual std::string encode(std::string val)
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
				if (m_values[i] == val && m_values[i].substr(0, 1) != "-")
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

	virtual std::string decode_num(std::string val)
	{
		// No point in decoding a string
		if (m_enc == ENC_STRING)
			return val;
		if (m_enc == ENC_INT || m_enc == ENC_STRINGINT)
		{
			std::size_t idx(0);
			unsigned v = msi_stou(val, &idx, m_base);
			if (idx != val.size())
				return val; // return val
			return std::to_string(v);
		}
		return "";
	}

	virtual std::string decode(std::string val)
	{
		if (m_enc == ENC_STRING)
			return val;
		if (m_enc == ENC_INT)
		{
			std::size_t idx(0);
			unsigned v = msi_stou(val, &idx, m_base);
			if (idx != val.size())
				return "";
			char buf[100];
			std::snprintf(buf, 100, "%u", v);
			return buf;
		}
		else if (m_enc == ENC_STRINGINT)
		{
			std::size_t eidx(0);
			auto v = msi_stou(val, &eidx, m_base);
			if (eidx != val.size())
				return ""; // FIXME - must be checked by caller!
			else if (v>=m_values.size())
				return ""; // FIXME - must be checked by caller!
			return m_values[v];
		}
		else
			return "";
	}

	virtual std::string help()
	{
		std::string ret = "values: ";
		if (m_enc == ENC_STRINGINT)
		{
			for (auto &v : m_values)
				if (v.substr(0, 1) != "-")
					ret += (v + " ");
		}
		else
			ret += std::to_string(m_min) + " to " + std::to_string(m_max);
		return ret;
	}

	series_t m_series;
	access_t m_access;
	encoding_t m_enc;
	std::string m_cmd;
	std::string m_opt;
	string_list m_values;
	unsigned m_min = 0;
	unsigned m_max = 100;
	int m_base = 10;
};

struct tripple_t : public setting_t
{
	tripple_t(series_t series, std::string cmd, std::string opt)
	: setting_t(series, READWRITE, cmd, opt, {""})
	{ }

	std::string encode(std::string val) override
	{
		auto vals = number_list<unsigned>(splitstr(val,','));
		if (vals.size() != 3)
			return "";

		std::string r;
		for (auto &n : vals)
		{
			if (n>100)
				return "";
			r = r + msi_utos(n, 1000, 1);
		}
		return r;
	}

	std::string decode_num(std::string val) override
	{
		std::size_t idx(0);
		unsigned v = msi_stou(val, &idx, 1000);
		if (idx != val.size())
			return "";
		return std::to_string(v);
	}

	std::string decode(std::string val) override
	{
		std::size_t idx(0);
		unsigned v = msi_stou(val, &idx, 1000);
		if (idx != val.size())
			return "";
		return std::to_string(v / 1000000) + "," + std::to_string((v / 1000) % 1000) + "," + std::to_string(v % 1000);
	}

	std::string help() override
	{
		return "tripple: v1,v2,v3 where v<=100";
	}
};

struct alarm4x_t : public setting_t
{
	alarm4x_t(series_t series, std::string cmd, std::string opt)
	: setting_t(series, WRITE, cmd, opt, {""})
	{ }

	std::string encode(std::string val) override
	{
		auto vals = number_list<unsigned>(splitstr(val,','));
		if (vals.size() != 5)
			return "";

		std::string r;
		for (std::size_t i=0; i<4; i++)
		{
			if (vals[i] > cMAX_ALARM)
				return "";
			r = r + msi_utos(vals[i], -60, 2);
		}
		if (vals[4] > 4)
			return "";
		r = r + msi_utos(vals[4], 10, 1) + "0000000000";
		return r;
	}

	std::string decode(std::string val) override
	{
		fprintf(stderr, "should never be called");
		return "";
	}

	std::string help() override
	{
		return "a1,a2,a3,a4,n where a<"+ std::to_string(cMAX_ALARM) + " and n<=4";
	}
};

// MPG341CQR:  3DA0
// MAG321CURV  3DA2
// MAG322CQRV  3DA4
// PS341WU:    3DA1

static std::vector<setting_t *> settings(
{
	new setting_t(ALL, WRITE,              "00100", "power", {"off", "-on"}),
	new setting_t(ALL, READ,               "00110", "macro_key", {"off", "pressed"}),  // returns 000 called frequently by OSD app, readonly
	new setting_t(MAG272,                  "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "reader", "cinema", "designer", "HDR"}),
	new setting_t(MAG32,                   "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "reader", "cinema", "designer"}),
	new setting_t(PS,                      "00120", "mode", {"-m0","-m1","-m2","-m3","-m4""-m5","-m6","-m7","-m8","-m9",
		"user", "adobe_rgb", "dci_p3", "srgb", "hdr", "cinema", "reader", "bw", "dicom", "eyecare", "cal1", "cal2", "cal3"}),
	new setting_t(ALL,                     "00130", "serial"), // returns 13 blanks
	new setting_t(UNKNOWN,                 "00160", "unknown160"),  // query kills monitor side on MAG
	new setting_t(ALL,                     "00170", "frequency"),   // returns 060
	new setting_t(PS, READ,                "00180", "quick_charge", {"off", "on"}),  // returns 56006 on MAG, 000 on PS
	new setting_t(PS,                      "00190", "unknown190"),  // returns 56006 on MAG, 000 on PS
	new setting_t(UNKNOWN,                 "001@0", "unknown1@0"),
	new setting_t(MAG,                     "00200", "game_mode", {"user", "fps", "racing", "rts", "rpg"}),
	(new setting_t(MAG32 | MAG272,         "00210", "unknown210", 0, 10))->set_access(WRITE),  // returns "00:" but can only be set to 000 to 009 - no visible effect
	(new setting_t(MAG32 | MAG272,         "00210", "unknown210", 0, 10, -100))->set_access(READ),  // returns "00:" but can only be set to 000 to 009 - no visible effect
	new setting_t(MAG271 | MAG241,         "00210", "black_tuner", 0, 20, -100),
	new setting_t(ALL,                     "00220", "response_time", {"normal", "fast", "fastest"}),  // returns 000 0:normal, 1:fast, 2:fastest
	// FIXME: anti-motion blur? -- MAG272QP MAG271 MAG241
	// FIXME: MAG321 manual says only supported for For Optix MAG322CQRV
	new setting_t(MAG,                     "00230", "enable_dynamic", {"on", "off"}),  // returns 000 - on/off only ==> on disables ZL and HDCR in OSD
	new setting_t(MAG,                     "00240", "hdcr", {"off", "on"}),
	new setting_t(MAG,                     "00250", "refresh_display", {"off", "on"}),
	new setting_t(MAG,                     "00251", "refresh_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),
	new setting_t(ALL,                     "00260", "alarm_clock", {"off", "1", "2", "3", "4"}),
	new setting_t(MAG,                     "00261", "alarm_clock_index", 1, 4),  // FIXME: returns timeout on PS
	new setting_t(MAG,                     "00262", "alarm_clock_time", 0, cMAX_ALARM, -60),  // FIXME: returns timeout on PS
	new setting_t(MAG,                     "00263", "alarm_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),
	new setting_t(PS,                      "00263", "alarm_position", {"left_top", "right_top", "left_bottom", "right_bottom", "custom"}),
	new alarm4x_t(ALL,                     "001f",  "alarm4x"),
	// FIXME:
	new setting_t(MAG,                     "00270", "screen_assistance", 100, {"off", "red1", "red2", "red3", "red4", "red5", "red6",
		"white1", "white2", "white3", "white4", "white5", "white6"}),
	new setting_t(PS,                      "00270", "screen_assistance", {"off", "center", "edge",
		"scale_v", "scale_h", "line_v", "line_h", "grid", "thirds", "3D_assistance"}),
	new setting_t(UNKNOWN,                 "00271", "unknown271", 0, 100),  // returns 000, read only?
	// FIXME: adaptive sync ? game-mode only
	new setting_t(MAG32,                   "00280", "unknown280"),  // returns 000, read only, write fails and monitor needs off/on cycle
	new setting_t(MAG272 | MAG271 | MAG241,"00280", "free_sync", {"off", "on"}),
	new setting_t(MAG32 | MAG272 | MAG271, "00290", "zero_latency", {"off", "on"}),  // returns 001
	// FIXME: MAG241 manual says it is supported but constantly produces time out errors
	// new setting_t(MAG241,                  "002:0", "screen_size", {"4:3", "16:9"}),
	new setting_t(MAG272,                  "002:0", "screen_size", {"auto", "4:3", "16:9"}),
	new setting_t(MAG32 | MAG271,          "002:0", "screen_size", {"19", "24", "4:3", "16:9"}),
	new setting_t(PS,                      "002:0", "screen_size", {"auto", "4:3", "16:9", "21:9", "1:1"}),
	new setting_t(MAG32 | MAG272,          "002;0", "night_vision", {"off", "normal", "strong", "strongest", "ai"}),
	new setting_t(MAG272,                  "00300", "pro_mode", {"user", "reader", "cinema", "designer", "HDR"}),
	new setting_t(MAG32 | MAG271 | MAG241, "00300", "pro_mode", {"user", "reader", "cinema", "designer"}),
	new setting_t(PS,                      "00300", "pro_mode", {"user", "adobe_rgb", "dci_p3", "srgb", "hdr", "cinema", "reader", "bw", "dicom", "eyecare", "cal1", "cal2", "cal3"}),
	new setting_t(MAG | PS,                "00310", "eye_saver", {"off", "on"}),  // returns 000
	new setting_t(UNKNOWN,                 "00320", "unknown320", 0, 100),
	new setting_t(UNKNOWN,                 "00330", "unknown330",0 , 100),
	new setting_t(ALL,                     "00340", "image_enhancement", {"off","weak","medium","strong","strongest"}),
	new setting_t(UNKNOWN,                 "00350", "unknown350", 0, 100),
	new setting_t(ALL,                     "00400", "brightness", 0, 100),  // returns 048
	new setting_t(ALL,                     "00410", "contrast", 0, 100),  // returns 050
	new setting_t(ALL,                     "00420", "sharpness", 0, 5),  // returns 000
	new setting_t(MAG,                     "00430", "color_preset", {"cool", "normal", "warm", "custom"}),
	new setting_t(PS,                      "00430", "color_preset", {"5000K", "5500K", "6500K", "7500K", "9300K", "10000K", "custom"}),
	new setting_t(MAG,                     "00431", "color_red", 0, 100),
	new setting_t(MAG,                     "00432", "color_green", 0, 100),
	new setting_t(MAG,                     "00433", "color_blue", 0, 100),
	new tripple_t(ALL,                     "00434", "color_rgb"),  // returns bbb  -> value = 'b' - '0' = 98-48=50
	new setting_t(MAG,                     "00435", "unknown435"),  // returns 000, read only
	new setting_t(ALL, WRITE,              "00440", "unknown440", {"off", "on"}),
	new setting_t(UNKNOWN,                 "00450", "unknown450",0, 100),
	new setting_t(PS,                      "00460", "gray_level", 0, 20),
	new setting_t(UNKNOWN,                 "00470", "unknown470", 0, 100),
	new setting_t(PS,                      "00480", "low_blue_light", {"off", "on"}),
	new setting_t(PS,                      "00490", "local_dimming", {"off", "on"}),
	new tripple_t(PS,                      "004<0", "hue_rgb"),
	new tripple_t(PS,                      "004<1", "hue_cmy"),
	new setting_t(PS,                      "004=0", "zoom", {"off", "on"}),
	new setting_t(PS,                      "004=1", "zoom_location", {"center", "left_top", "right_top", "left_bottom", "right_bottom"}),
	new tripple_t(PS,                      "004;0", "saturation_rgb"),
	new tripple_t(PS,                      "004;1", "saturation_cmy"),
	new setting_t(PS,                      "004:0", "gamma", {"1.8", "2", "2.2", "2.4", "2.6"}),
	new setting_t(MAG32 | MAG272 | PS,     "00500", "input",  {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 002  -> 0=hdmi1, 1=hdmi2, 2=dp, 3=usbc
	new setting_t(        MAG271 | MAG241, "00500", "input",  {"hdmi1", "hdmi2", "dp"}),
	new setting_t(MAG32 | MAG271,          "00600", "pip", {"off", "pip", "pbp"}),  // returns 000 0:off, 1:pip, 2:pbp
	new setting_t(PS,                      "00600", "pip", {"off", "pip", "pbp_x2", "pbp_x3", "pbp_x4"}),  // returns 000 0:off, 1:pip, 2:pbp
	new setting_t(MAG32,                   "00610", "pip_input", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(MAG32,                   "00620", "pbp_input", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(MAG271,                  "00610", "pip_input", {"hdmi1", "hdmi2", "dp"}),
	new setting_t(MAG271,                  "00620", "pbp_input", {"hdmi1", "hdmi2", "dp"}),
	new setting_t(PS,                      "00620", "pip_input", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS | MAG32 | MAG271,     "00630", "pip_size", {"small", "medium", "large"}),
	new setting_t(PS | MAG32 | MAG271,     "00640", "pip_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),
	new setting_t(PS | MAG32 | MAG271, WRITE, "00650", "toggle_display", {"-off", "on"}),  // returns 56006
	new setting_t(MAG32 | MAG271, WRITE,   "00660", "toggle_sound", {"-off", "on"}),  // returns 56006, but used to toggle audio in app, no response packet - only works with "1"
	new setting_t(PS,                      "00660", "pip_sound_source", {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 56006, but used to toggle audio in app, no response packet - only works with "1"
	new setting_t(PS,                      "00670", "pbp_input1", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS,                      "00680", "pbp_input2", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS,                      "00690", "pbp_input3", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS,                      "006:0", "pbp_input4", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS,                      "006;0", "pbp_sound_source", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(MAG,                     "00800", "osd_language", 0, 19, -100),  // returns 001 -> value = '0' + language, 0 chinese, 1 English, 2 French, 3 German, ... maximum value "C"
	new setting_t(PS,                      "00800", "osd_language", 0, 28, -100),  // returns 001 -> value = '0' + language, 0 chinese, 1 English, 2 French, 3 German, ... maximum value "C"
	new setting_t(ALL,                     "00810", "osd_transparency", 0, 5),  // returns 000
	new setting_t(ALL,                     "00820", "osd_timeout",0, 30),  // returns 020
	new setting_t(PS | MAG272,             "00830", "screen_info", {"off", "on"}),
	new setting_t(ALL, WRITE,              "00840", "reset", {"-off", "on"}),  // returns 56006 - reset monitors
	new setting_t(MAG,                     "00850", "sound_enable", {"off", "on"}),  // returns 001 - digital/anlog as on some screenshots?
	new setting_t(PS,                      "00850", "audio_source", {"analog", "digital"}),  // returns 001 - digital/anlog as on some screenshots?
	new setting_t(MAG,                     "00860", "rgb_led", {"off", "on"}),

	// The following are for experimental purposes
	new setting_t(UNKNOWN,	               "00700", "unknown700", 0, 100),
	new setting_t(UNKNOWN,	               "00710", "unknown710", 0, 100),
	new setting_t(UNKNOWN,	               "00720", "unknown720", 0, 100),
	new setting_t(UNKNOWN,	               "00730", "unknown730", 0, 100),
	new setting_t(UNKNOWN,	               "00740", "unknown740", 0, 100),
	new setting_t(UNKNOWN,	               "00870", "unknown870", 0, 100),
	new setting_t(UNKNOWN,	               "00880", "unknown880", 0, 100),
	new setting_t(UNKNOWN,	               "00890", "unknown890", 0, 100),
	new setting_t(UNKNOWN,	               "008:0", "unknown8:0", 0, 100),

	new setting_t(MAG272,                  "00900", "navi_up", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "refresh_rate" , "info"}),
	new setting_t(MAG272,                  "00910", "navi_down", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "refresh_rate" , "info"}),
	new setting_t(MAG272,                  "00920", "navi_left", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "refresh_rate" , "info"}),
	new setting_t(MAG272,                  "00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "refresh_rate" , "info"}),

	new setting_t(MAG241,                  "00900", "navi_up", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate"}),
	new setting_t(MAG241,                  "00910", "navi_down", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate"}),
	new setting_t(MAG241,                  "00920", "navi_left", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate"}),
	new setting_t(MAG241,                  "00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate"}),

	new setting_t(MAG32 | MAG271,          "00900", "navi_up", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),
	new setting_t(MAG32 | MAG271,          "00910", "navi_down", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),
	new setting_t(MAG32 | MAG271,          "00920", "navi_left", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),
	new setting_t(MAG32 | MAG271,          "00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),

	new setting_t(PS,                      "00900", "navi_up", {"off", "brightness", "pro_mode", "screen_assistance", "alarm_clock", "input", "pip", "zoom_in", "info"}),
	new setting_t(PS,                      "00910", "navi_down", {"off", "brightness", "pro_mode", "screen_assistance", "alarm_clock", "input", "pip", "zoom_in", "info"}),
	new setting_t(PS,                      "00920", "navi_left", {"off", "brightness", "pro_mode", "screen_assistance", "alarm_clock", "input", "pip", "zoom_in", "info"}),
	new setting_t(PS,                      "00930", "navi_right", {"off", "brightness", "pro_mode", "screen_assistance", "alarm_clock", "input", "pip", "zoom_in", "info"}),
});

static setting_t *get_read_setting(series_t series, std::string opt)
{
	for (auto &s : settings)
		if (s->m_opt == opt && (s->m_access==READWRITE || s->m_access==READ)
				&& (s->m_series & series))
			return s;
	return nullptr;
}

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
			// Same applies for PS341 monitor, pos 5,6,7 are 0 bytes.
			// FIXME: looks like the return starts at pos 8
			if (ret.size() > cmd.size()
				&& (ret.substr(0, cmd.size()) == std::string("5b") + setting.m_cmd
				    || cmd == "5800260" || ret.substr(0, cmd.size()) == "5b00___" ))
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
			else
				ret += '_';
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

static void help_set(series_t series, series_t exclude1, series_t exclude2)
{
	static std::array<const char *, 3> access_str = { "R", "W", "RW" };
	for (auto &s : settings)
		if (/*(s->m_access == WRITE || s->m_access == READWRITE)
			&& */((s->m_series & series) == series)
			&& (s->m_series != exclude1)
			&& (s->m_series != exclude2))
		{
			pprintf("      --%-20s %2s %s\n", s->m_opt, access_str[s->m_access], s->help());
		}
}

static int help()
{
	pprintf(
		"Usage: %s [OPTION]... \n"
		"Query or set monitor settings by usb directly on the device.\n"
		"For supported devices please refer to the documentation.\n"
		"\n"
		"Options are processed in the order they are given. You may\n"
		"specify an option more than once with identical or different\n"
		"values. After processing, mystic led settings are sent first\n"
		"to the device. Afterwards all setting changes are sent to\n"
		"the device. Once completed settings specified are queried.\n"
		"The --wait option which will be executed last\n"
		"\n"
		"In addition to preset modes the --mystic option also accepts numeric\n"
		"values. 0xff0000 will set all leds to red. '0,255,0' will set all leds\n"
		"to green.\n"
		"\n"
		"All device settings provide which operations are possible:\n"
		"R: Read, W: Write, RW: Read/Write\n"
		"\n"
		"\nOptions:\n\n"
		"  -q, --query                display all monitor settings. This will also\n"
		"                               list readonly settings and settings whose\n"
		"                               function is currently unknown.\n"
		"      --info                 display device information. This can be used\n"
		"                               with --query\n"
		"  -f, --filter               limits query result to comma separated list\n"
		"                               of settings, e.g. -f contrast,gamma\n"
		"  -w, --wait                 SETTING=VALUE. Wait for SETTING to become\n"
		"                               VALUE, e.g. macro_key=pressed\n"
		"  -n, --numeric              monitor settings are displayed as numeric\n"
	    "                                settings\n"
		, appname);
	pprintf("%s",
		"       --mystic              off, static, breathing, blinking, flashing,\n"
		"                               blinds, meteor, rainbow, random,\n"
		"                               0xRRGGBB, RRR,GGG,BBB\n"
		"                               Only on MAG series monitors.\n");
	pprintf("%s", "\nAll monitors:\n");
	pprintf("%s", "    These options apply to all monitors:\n\n");
	help_set(ALL, UNKNOWN, UNKNOWN);
	pprintf("%s", "\nMAG series monitors:\n");
	pprintf("%s", "    These options apply to MAG272 and MAG32 monitors:\n\n");
	help_set(MAG, ALL, UNKNOWN);
	for (std::size_t i=1; i<known_models.size(); i++)
	{
		pprintf("\n%s:\n", known_models[i].name);
		pprintf("    These options apply to the %s:\n\n", known_models[i].name);
		help_set(known_models[i].series, ALL, MAG);
	}
	pprintf("\n%s", "General options:\n");
	pprintf("%s", "    These options always apply:\n\n");
	pprintf("%s",
		"  -d, --debug                enable debug output\n"
		"                               Enables raw output for query command\n"
		"  -h, --help                 display this help and exit\n"
		"      --version              output version information and exit\n"
		"\n"
		"Exit status:\n"
		" 0  if OK,\n"
		" 1  if error during option parsing,\n"
		" 2  if error during device identification,\n"
		" 3  if error during setting parameters on device,\n"
		" 4  if error during reading parameters from device,\n"
		"\n"
		"Report bugs on <https://github.com/couriersud/msigd/issues>\n"
		"msigd home page: <https://github.com/couriersud/msigd>\n"
	);
	return 0;
}

static int version()
{
	pprintf("%s %s\n"
		"Copyright (C) 2019, 2020 Couriersud\n"
		"License GPLv2: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n"
		"\n"
		"Written by Couriersud\n", appname, appversion);
	return 0;
}

template<typename... Args>
static int error(error_e err, const char *fmt, Args&&... args)
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
	bool numeric = false;
	string_list filters;
	std::string waitfor;

	std::vector<std::pair<std::string, std::string>> setopts;
	std::vector<setting_t *> qsettings;

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
		else if (cur_opt == "--numeric" || cur_opt == "-n")
			numeric = true;
		else if ((cur_opt == "--filter" || cur_opt == "-f") && arg_pointer + 1 < argc)
		{
			filters = splitstr(argv[++arg_pointer], ',');
			query = true;
		}
		else if ((cur_opt == "--wait" || cur_opt == "-w") && arg_pointer + 1 < argc)
		{
			waitfor = argv[++arg_pointer];
		}
		else if (cur_opt == "--mystic" && arg_pointer + 1 < argc)
		{
			if (mystic_opt(argv[++arg_pointer], leds) != 0)
				return error(E_SYNTAX, "Mystic light parameter error: %s", argv[arg_pointer]);
			mystic = true;
		}
		else
		{
			if (cur_opt.size() >= 3 && arg_pointer + 1 < argc && cur_opt.substr(0,2) == "--")
			{
				auto search(cur_opt.substr(2));
				setopts.emplace_back(search, argv[++arg_pointer]);
			}
			else
				return error(E_SYNTAX, "Unknown option: %s", cur_opt);
		}
		arg_pointer++;
	}

	std_logger_t logger;
	logger.set_level(DEBUG, debug);

	mondev_t usb(logger, 0x1462, 0x3fa4, "MSI Gaming Controller");

	if (usb)
	{
		bool notify(false);
		// Try to identify the monitor
		std::string s140;
		std::string s150;
		identity_t &series = known_models[0];

		setting_t sp140(ALL, "00140", "sp140");
		setting_t sp150(ALL, "00150", "sp150");

		if (!usb.get_setting(sp140, s140) && !usb.get_setting(sp150, s150))
		{
			for (auto &m : known_models)
				if (m.p140 == s140 && m.p150 == s150)
					series = m;

			if (series.series == UNKNOWN)
			{
				eprintf("Unexpected id combination <%s><%s>\n", s140, s150);
				notify = true;
			}
		}
		else
		{
			return error(E_IDENTIFY, "Error on device identification");
		}

		if (notify)
		{
			eprintf("Detected an unknown monitor. Please report the output of\n"
				    "'msigd --info --debug --query' as an issue and also provide\n"
					"the ID (MAG...) of your monitor. Thank you!\n");
			// Assume mag series
			series = known_models[1];
		}

		if (info)
		{
			pprintf("Vendor Id:      0x%04x\n", usb.vendor_id());
			pprintf("Product Id:     0x%04x\n", usb.product_id());
			pprintf("Product:        %s\n",     usb.product());
			pprintf("Serial:         %s\n",     usb.serial());
			pprintf("Monitor Series: %s\n",     series.name);
			if (debug)
			{
				pprintf("s140:           <%s>\n", s140);
				pprintf("s150:           <%s>\n", s150);
				usb.debug_cmd("\x01\xb0");
				usb.debug_cmd("\x01\xb4");
				// set after MSI app 20191206 0.0.2.23
				if (series.series != MAG241) // times out on MAG241
					usb.debug_cmd("\x01\xd0""b00100000\r");
				// queried but not supported on MAG321CURV (returns 56006)
				//usb.debug_cmd("\x01""5800190\r");
				//usb.debug_cmd("\x01""5800130\r");
			}
		}

		if (mystic && !series.has_mystic)
			return error(E_SYNTAX, "--mystic only supported on MAG series monitors");

		// Check parameters to be set
		std::vector<std::pair<setting_t *, std::string>> set_encoded;

		// Check setopts ...
		for (auto &opt: setopts)
		{
			bool found(false);
			for (auto &s : settings)
			{
				if (s->m_opt == opt.first && (s->m_access==READWRITE || s->m_access==WRITE)
					&& (s->m_series & series.series))
				{
					std::string val = s->encode(opt.second);
					if (val == "")
						return error(E_SYNTAX, "Unknown value <%s> for option %s", opt.second, opt.first);
					set_encoded.emplace_back(s, val);
					found = true;
					break;
				}
			}
			if (!found)
				return error(E_SYNTAX, "Unknown option: %s", opt.first);
		}

		// Check filters
		if (query)
		{
			unsigned filters_found(0);

			for (auto &setting : settings)
			{
				if ((setting->m_access==READWRITE || setting->m_access==READ)
					&& (setting->m_series & series.series))
				{
					if (filters.empty())
						qsettings.push_back(setting);
					else
					{
						for (auto &f : filters)
						{
							if (f == setting->m_opt)
							{
								qsettings.push_back(setting);
								filters_found++;
								break;
							}
						}
					}
				}
			}
			if (filters_found != filters.size())
				return error(E_SYNTAX, "Unsupported query filter elements: %d of %d", filters.size() - filters_found , filters.size());
		}

		// Set mystic leds
		if (mystic)
			usb.write_led(leds);

		// set values
		for (auto &s : set_encoded)
			if (usb.set_setting(*s.first, s.second))
				return error(E_SETTING, "Error setting --%s", s.first->m_opt);

		// now query
		if (query)
		{
			for (auto &setting : qsettings)
			{
			    std::this_thread::sleep_for(cQUERY_DELAY);

				std::string res;
				if (!usb.get_setting(*setting, res))
				{
					if (debug)
						pprintf("%s : '%s'\n", setting->m_opt, hexify_if_not_printable(res));
					else if (numeric)
						pprintf("%s : %s\n", setting->m_opt, setting->decode_num(res));
					else
						pprintf("%s : %s\n", setting->m_opt, setting->decode(res));
				}
				else
				{
					error(E_IDENTIFY, "Error querying device on %s - got '%s'", setting->m_opt, res);
					if (!debug)
						return E_IDENTIFY;
				}
			}
		}

		// Now wait for setting

		if (waitfor.size() != 0)
		{
			auto sp = splitstr(waitfor, '=');
			if (sp.size() != 2)
				return error(E_SYNTAX, "--wait syntax error: %s", waitfor);
			setting_t *setting = get_read_setting(series.series, sp[0]);
			if (setting == nullptr)
				return error(E_SYNTAX, "--wait setting not found: %s", sp[0]);
			while (true)
			{
			    std::this_thread::sleep_for(cWAIT_DELAY);
				std::string res;
				if (!usb.get_setting(*setting, res))
				{
					if (sp[1] == setting->decode(res))
						return 0;
				}
				else
				{
					return error(E_QUERY, "Error querying device on %s - got '%s'", sp[0], res);
				}

			}
		}

	}
	else
		return error(E_IDENTIFY, "No usb device found", 0);

	return 0;
}

/*
 * MAG272Q
 * DEBUG: Special 01 b0 : 01 5a 40 00 00 00 00 00 00 00 00 00 00 00 00 00
 * DEBUG: Special 01 b4 : 01 5a 71 00 2f 41 00 00 00 00 00 00 00 00 00 00
 *
 * MAG321
 *
 * DEBUG: Special 01 b0 : 01 5a 35 03 00 00 00 00 00 00 00 00 00 00 00 00
 * DEBUG: Special 01 b4 : 01 5a 41 00 1e c9 00 00 00 00 00 00 00 00 00 00
 *
 * PS341WU
 *
 * DEBUG: Special 01 b0 : 01 5a 37 00 00 00 00 00 00 00 00 00 00 00 00 00
 * DEBUG: Special 01 b4 : 01 5a 41 00 19 b2 00 00 00 00 00 00 00 00 00 00
 *
 * MAG241
 *
 * DEBUG: Special 01 b0 : 01 5a 15 00 00 00 00 00 00 00 00 00 00 00 00 00
 * DEBUG: Special 01 b4 : 01 5a 02 50 20 7e 00 00 00 00 00 00 00 00 00 00
 *
 * 2f41  = 0010111101000001
 * 1ec9  = 0001111011001001
 * 19b2  = 0001100110110010
 * 207e  = 0010000001111110
 *
 * ALL   = 0000000000000000
 * MAG   = 0000000001000000
 * PS272 = 0001100010000000
 * PS341 = 0000100100000000
 *
 *
 *
 *
 */
