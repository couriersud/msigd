
#if !defined(USE_HID)
#define USE_HID 	(1)
#endif

#if USE_HID
#include "phid.h"
#else
#include "pusb.h"
#endif

#include "psteelseries.h"

#include <cstddef>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <map>
#include <utility>
#include <type_traits>
#include <chrono>
#include <thread>

#include <stdio.h>
#include <string.h>

static const char *appname = "msigd";
static const char *appversion = "0.12";

static const unsigned cMAX_ALARM = 99 * 60 + 59;

static const auto cQUERY_DELAY = std::chrono::milliseconds(25); // FIXME: do we still need this - tests here say no
static const auto cWAIT_DELAY = std::chrono::milliseconds(250);

//
// Enable writing of parameters - use for experimental brances
//
static constexpr const bool cWRITE_ENABLED = true;

enum access_t
{
	READ,
	WRITE,
	READWRITE
};

enum series_t
{
	MAG321CURV     = 0x00000001,
	MAG321CQR      = 0x00000002,
	MAG241C        = 0x00000004,
	MAG241CR       = 0x00000008,
	MAG271CR       = 0x00000010,
	MAG271CQR      = 0x00000020,
	MAG272         = 0x00000040,
	MAG272CQR      = 0x00000080,
	MAG272QP       = 0x00000100,
	MAG272QR       = 0x00000200,
	MAG274QRFQD    = 0x00000400,
	MAG274QRFQDNEW = 0x00000800,
	PS341WU        = 0x00001000,
	MPG341CQR      = 0x00002000,
	MPG273CQR      = 0x00004000,
	MPG27CQ        = 0x00008000,
	MAG321QR       = 0x00010000,
	QUERYONLY      = 0x00100000,
	UNKNOWN        = 0x01000000,

	MAG241GRP      = MAG241C | MAG241CR | MAG271CR,
	MAG274GRP      = MAG274QRFQD | MAG274QRFQDNEW,
	MAG272GRP      = MAG272 | MAG272CQR | MAG272QP | MAG272QR,

	MAG     = MAG321CURV | MAG321CQR | MAG272GRP | MAG271CQR | MAG241GRP | MAG274GRP,
	MPG     = MPG273CQR | MPG341CQR | MPG27CQ,

	ALL     = MAG | PS341WU | MPG | QUERYONLY | MAG321QR | MPG27CQ,
};

static series_t operator | (series_t a, series_t b)
{
	return static_cast<series_t>(static_cast<int>(a) | static_cast<int>(b));
}

enum led_type_t
{
	LT_NONE,
	LT_MYSTIC,
	LT_MYSTIC_OPTIX,
	LT_STEEL
};

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
	led_type_t  leds;
};

static std::vector<identity_t> known_models =
{
	{ UNKNOWN,           "",     "", "Unknown", LT_NONE },
	{ QUERYONLY,         "",     "", "Unknown Series", LT_NONE },
	// issue #32 says MAG321CURV has "<00;>" "<V43>" combination?
	{ MAG321CURV,        "00;", "V18", "MAG321CURV", LT_MYSTIC },                // MAG321CURV
	{ MAG321CQR,         "00:", "V18", "MAG321CQR", LT_MYSTIC }, 	               // doesn't have USBC
	// FIXME: see #33 - feedback on mystic required
	{ MAG321QR,          "00{", "V51", "MAG321QR",  LT_NONE },                     // Has MPRT, KVM -> see MPG273
	{ MAG241C,           "002", "V18", "MAG241C", LT_NONE },
	// FIXME: Needs separate series (has RGB backlight OSD setting) - above not
	{ MAG241CR,          "004", "V18", "MAG241CR", LT_MYSTIC },                    // MAG241CR
	{ MAG271CR,          "005", "V18", "MAG271CR", LT_MYSTIC },                    // MAG271CR
	{ MAG271CQR,         "006", "V19", "MAG271CQR", LT_MYSTIC },                   // MAG271CQR, MAG271CQP?
	{ MAG272CQR,         "00E", "V18", "MAG272CQR", LT_MYSTIC },                   // MAG272CQR
	{ MAG272QR,          "00G", "V18", "MAG272QR", LT_NONE },                      // MAG272QR - Mystic with 12 leds?
	{ MAG272,            "00L", "V18", "MAG272", LT_MYSTIC },                      // MAG272
	{ MAG272QP,          "00O", "V18", "MAG272QP", LT_MYSTIC },                    // MAG272QP
	{ MPG27CQ,           "001", "V18", "MPG27CQ", LT_STEEL },                      // MPG27CQ
	{ MPG273CQR,         "00[", "V51", "MPG273CQR", LT_MYSTIC_OPTIX },             // MPG273CQR 9 Leds in group 1 (logo) and 15 leds in group 2 (arrow)
	{ MPG341CQR,         "00>", "V09", "MPG341CQR", LT_STEEL },                    // MPG341CQR
	{ MAG274QRFQD,       "00e", "V43", "MAG274QRF-QD FW.011", LT_MYSTIC },         // MAG274QRF-QD FW.011
	{ MAG274QRFQDNEW,    "00e", "V48", "MAG274QRF-QD FW.015/FW.016", LT_MYSTIC },  // MAG274QRF-QD FW.015/FW.016
	{ PS341WU,           "00?", "V06", "PS341WU", LT_NONE }
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
std::vector<T> number_list(const string_list &sl, int base)
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

template<typename... Args>
static int error(error_e err, const char *fmt, Args&&... args)
{
	fprintf(stderr, "%s: ", appname);
	fprintf(stderr, fmt, log_helper(args)...);
	fprintf(stderr, "\n");
	fprintf(stderr, "Try '%s --help' for more information.\n", appname);
	return err;
}

template <typename U>
int string_to_unsigned(std::string s, U &val, U minv, U maxv)
{
	unsigned long v = 0;
	std::size_t idx = 0;
	try {
		v = std::stoul(s, &idx, 10);
	}
	catch (...)
	{
	}
	if (idx != s.size())
		return error(E_SYNTAX, "Error decoding number: %s", s);
	val = static_cast<U>(v);
	if (val < minv || val > maxv)
		return error(E_SYNTAX, "Number outside bounds: %s [%d .. %d]", s, minv, maxv);
	return 0;
}


template <typename T>
int number_list(const string_list &sl, std::vector<T> &ret)
{
	for (auto &s : sl)
	{
		std::size_t idx(0);
		int base = 10;
		std::string sv = s;
		if (s.substr(0,2) == "0x")
		{
			sv = s.substr(2);
			base = 16;
		}
		auto v = std::stoul(sv, &idx, base);
		if (idx != sv.size())
			return error(E_SYNTAX, "Error decoding number list element: %s", sv);
		ret.push_back(static_cast<T>(v));
	}
	return 0;
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
					return msi_utos(static_cast<unsigned>(i), m_base, 3);
					//char buf[100];
					//snprintf(buf, 100, "%03d", static_cast<unsigned>(i));
					//return buf;
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
		auto vals = number_list<unsigned>(splitstr(val,','), 10);
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
		auto vals = number_list<unsigned>(splitstr(val,','), 10);
		if (vals.size() != 5)
			return "";

		std::string r;
		for (std::size_t i=0; i<4; i++)
		{
			if (vals[i] > cMAX_ALARM)
				return "";
			r = r + msi_utos(vals[i], -60, 2);
		}
		if (vals[4] < 1 || vals[4] > 4)
			return "";
		r = r + msi_utos(vals[4], 10, 1) + "0000000000";
		return r;
	}

	std::string decode([[maybe_unused]] std::string val) override
	{
		fprintf(stderr, "should never be called");
		return "";
	}

	std::string help() override
	{
		return "a1,a2,a3,a4,n where a<"+ std::to_string(cMAX_ALARM) + " and 1=<n<=4";
	}
};

// MPG27CQ:    FA3
// MPG341CQR:  3DA0
// MAG321CURV  3DA2
// MAG322CQRV  3DA4
// PS341WU:    3DA1

static std::vector<setting_t *> settings(
{
	// MPG273CQR:
	//   - MPRT missing
	// only verified on MAG321CURV
	new setting_t(MAG321CURV, WRITE,       "00100", "power", {"off", "-on"}),

	new setting_t(ALL, READ,               "00110", "macro_key", {"off", "pressed"}),  // returns 000 called frequently by OSD app, readonly
	new setting_t(MAG272GRP,               "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "reader", "cinema", "designer", "HDR"}),
	new setting_t(MAG274QRFQD,             "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "reader", "cinema", "office"}), //Supported modes in FW.011
	new setting_t(MAG274QRFQDNEW,          "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "reader", "cinema",
	    "office", "srgb", "adobe_rgb", "dci_p3"}), //New moded added to FW.015
	new setting_t(MAG321CURV | MAG321CQR,  "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "reader", "cinema", "designer"}),
	new setting_t(MPG273CQR | MAG321QR,               "00120", "mode", {"user", "fps", "racing", "rts", "rpg", "mode5", "mode6", "mode7", "mode8", "mode9", "user", "anti_blue", "movie", "office", "srgb", "eco"}),

	new setting_t(PS341WU,                 "00120", "mode", {"-m0","-m1","-m2","-m3","-m4""-m5","-m6","-m7","-m8","-m9",
		"user", "adobe_rgb", "dci_p3", "srgb", "hdr", "cinema", "reader", "bw", "dicom", "eyecare", "cal1", "cal2", "cal3"}),
	new setting_t(ALL,                     "00130", "serial"), // returns 13 blanks
	new setting_t(ALL,                     "00170", "frequency"),   // returns 060
	new setting_t(PS341WU, READ,           "00180", "quick_charge", {"off", "on"}),  // returns 56006 on MAG, 000 on PS341WU

	new setting_t(MAG | MPG | MAG321QR,               "00200", "game_mode", {"user", "fps", "racing", "rts", "rpg"}),

	new setting_t(MAG271CQR | MAG241GRP | MPG27CQ,
		                                   "00210", "black_tuner", 0, 20, -100),
	new setting_t(ALL,                     "00220", "response_time", {"normal", "fast", "fastest"}),  // returns 000 0:normal, 1:fast, 2:fastest
	// FIXME: anti-motion blur? -- MAG272QP MAG271 MAG241GRP MPG27CQ
	// FIXME: MAG321CQR manual says only supported for Optix MAG322CQRV
	new setting_t(MAG | MPG341CQR,         "00230", "enable_dynamic", {"on", "off"}),  // returns 000 - on/off only ==> on disables ZL and HDCR in OSD
	new setting_t(MAG | MPG | MAG321QR,               "00240", "hdcr", {"off", "on"}),
	new setting_t(MAG | MPG | MAG321QR,               "00250", "refresh_display", {"off", "on"}),
	new setting_t(MAG | MPG | MAG321QR,               "00251", "refresh_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),

	// MPG341: Alarm settings seem to be broken.
	//         alarm_clock_time returns an invalid response.
	new setting_t(MAG | PS341WU | MPG273CQR | MPG27CQ | MAG321QR,
		                                   "00260", "alarm_clock", {"off", "1", "2", "3", "4"}),

	// The following have been verified only on MAG321CURV, not used in Gaming OSD
	new setting_t(MAG321CURV,              "00261", "alarm_clock_index", 1, 4),  // FIXME: returns timeout on PS341WU
	new setting_t(MAG321CURV | MPG273CQR | MAG321QR,  "00262", "alarm_clock_time", 0, cMAX_ALARM, -60),  // FIXME: returns timeout on PS341WU
	new setting_t(MAG | MPG27CQ,           "00263", "alarm_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),
	new setting_t(PS341WU | MPG273CQR | MAG321QR,     "00263", "alarm_position", {"left_top", "right_top", "left_bottom", "right_bottom", "custom"}),

	// alarm4x is only verified on MAG321CURV, used in Gaming OSD
	new alarm4x_t(MAG321CURV,              "001f",  "alarm4x"),

	// MPG341: screen_assistance returns invalid results
	// FIXME: Check on MPG27CQ
	new setting_t(MAG,                     "00270", "screen_assistance", 100, {"off", "red1", "red2", "red3", "red4", "red5", "red6",
		"white1", "white2", "white3", "white4", "white5", "white6"}),
	new setting_t(PS341WU,                 "00270", "screen_assistance", {"off", "center", "edge",
		"scale_v", "scale_h", "line_v", "line_h", "grid", "thirds", "3D_assistance"}),
	new setting_t(MPG273CQR | MAG321QR,               "00270", "smart_crosshair_icon", {"off", "icon1", "icon2", "icon3", "icon4", "icon5", "icon6"}),
	new setting_t(MPG273CQR | MAG321QR,               "00271", "smart_crosshair_color", {"white", "red", "auto"}),
	new setting_t(UNKNOWN,                 "00271", "unknown271", 0, 100),  // returns 000, read only?

	// FIXME: This is working in game mode only - adaptive sync
	// Disabled for security reasons
	new setting_t(UNKNOWN /*MAG321CURV*/,  "00280", "unknown280"),  // returns 000, read only, write fails and monitor needs off/on cycle

	// FIXME: free_sync also on MPG27CQ
	new setting_t(MAG321CQR | MAG272GRP | MAG271CQR | MAG241GRP | MAG274GRP | MPG341CQR | MPG273CQR | MPG27CQ | MAG321QR,
		                                   "00280", "free_sync", {"off", "on"}),
	new setting_t(MAG321CURV | MAG321CQR | MAG272GRP | MAG271CQR | MPG341CQR | MPG27CQ,
		                                   "00290", "zero_latency", {"off", "on"}),  // returns 001

	new setting_t(MAG272GRP | MPG273CQR | MAG321QR,   "002:0", "screen_size", {"auto", "4:3", "16:9"}),
	// FIXME: also on MPG27CQ
	new setting_t(MAG321CURV | MAG321CQR | MAG271CQR | MPG27CQ,
		                                   "002:0", "screen_size", {"19", "24", "4:3", "16:9"}),
	new setting_t(PS341WU | MPG341CQR,     "002:0", "screen_size", {"auto", "4:3", "16:9", "21:9", "1:1"}),
	new setting_t(MAG321CURV | MAG272GRP | MAG274GRP | MPG341CQR | MPG273CQR | MAG321QR,
										   "002;0", "night_vision", {"off", "normal", "strong", "strongest", "ai"}),
	new setting_t(MAG272GRP,               "00300", "pro_mode", {"user", "reader", "cinema", "designer", "HDR"}),
	new setting_t(MAG274QRFQD,             "00300", "pro_mode", {"user", "reader", "cinema", "office"}),
	new setting_t(MAG274QRFQDNEW,          "00300", "pro_mode", {"user", "reader", "cinema", "office", "srgb", "adobe_rgb", "dci_p3"}),
	new setting_t(MAG321CURV | MAG321CQR | MAG271CQR | MAG241GRP | MPG341CQR | MPG27CQ,
		                                   "00300", "pro_mode", {"user", "reader", "cinema", "designer"}),
	new setting_t(PS341WU,                 "00300", "pro_mode", {"user", "adobe_rgb", "dci_p3", "srgb", "hdr", "cinema", "reader", "bw", "dicom", "eyecare", "cal1", "cal2", "cal3"}),
	new setting_t(MPG273CQR | MAG321QR,               "00300", "pro_mode", {"user", "anti_blue", "movie", "office", "srgb", "eco"}),
	// low blue light on MPG273
	new setting_t(ALL,                     "00310", "eye_saver", {"off", "on"}),  // returns 000
	new setting_t(ALL,                     "00340", "image_enhancement", {"off","weak","medium","strong","strongest"}),

	new setting_t(ALL,                     "00400", "brightness", 0, 100),  // returns 048
	new setting_t(ALL,                     "00410", "contrast", 0, 100),  // returns 050
	new setting_t(ALL,                     "00420", "sharpness", 0, 5),  // returns 000
	new setting_t(MAG | MPG | MAG321QR,               "00430", "color_preset", {"cool", "normal", "warm", "custom"}),
	new setting_t(PS341WU,                 "00430", "color_preset", {"5000K", "5500K", "6500K", "7500K", "9300K", "10000K", "custom"}),
	new setting_t(MAG | MPG | MAG321QR,               "00431", "color_red", 0, 100),
	new setting_t(MAG | MPG | MAG321QR,               "00432", "color_green", 0, 100),
	new setting_t(MAG | MPG | MAG321QR,               "00433", "color_blue", 0, 100),
	new tripple_t(ALL,                     "00434", "color_rgb"),  // returns bbb  -> value = 'b' - '0' = 98-48=50

	new setting_t(PS341WU,                 "00460", "gray_level", 0, 20),
	new setting_t(PS341WU,                 "00480", "low_blue_light", {"off", "on"}),
	new setting_t(PS341WU,                 "00490", "local_dimming", {"off", "on"}),
	new tripple_t(PS341WU,                 "004<0", "hue_rgb"),
	new tripple_t(PS341WU,                 "004<1", "hue_cmy"),
	new setting_t(PS341WU,                 "004=0", "zoom", {"off", "on"}),
	new setting_t(PS341WU,                 "004=1", "zoom_location", {"center", "left_top", "right_top", "left_bottom", "right_bottom"}),
	new tripple_t(PS341WU,                 "004;0", "saturation_rgb"),
	new tripple_t(PS341WU,                 "004;1", "saturation_cmy"),
	new setting_t(PS341WU,                 "004:0", "gamma", {"1.8", "2", "2.2", "2.4", "2.6"}),
	new setting_t(MAG321CURV | MAG272GRP | MAG274GRP | PS341WU | MPG341CQR | MPG273CQR | MAG321QR,
		                                   "00500", "input",  {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 002  -> 0=hdmi1, 1=hdmi2, 2=dp, 3=usbc
	new setting_t(MAG321CQR| MAG271CQR | MAG241GRP | MPG27CQ,
		                                   "00500", "input",  {"hdmi1", "hdmi2", "dp"}),
	new setting_t(MPG273CQR | MAG321QR,               "00510", "auto_scan", {"off", "on"}),
	new setting_t(MAG321CURV | MAG321CQR | MAG271CQR | MPG27CQ,
		                                   "00600", "pip", {"off", "pip", "pbp"}),  // returns 000 0:off, 1:pip, 2:pbp
	new setting_t(PS341WU | MPG341CQR,     "00600", "pip", {"off", "pip", "pbp_x2", "pbp_x3", "pbp_x4"}),  // returns 000 0:off, 1:pip, 2:pbp
	new setting_t(MAG321CURV,              "00610", "pip_input", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(MAG321CURV,              "00620", "pbp_input", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(MAG271CQR | MAG321CQR | MPG27CQ,
		                                   "00610", "pip_input", {"hdmi1", "hdmi2", "dp"}),
	new setting_t(MAG271CQR | MAG321CQR | MPG27CQ,
		                                   "00620", "pbp_input", {"hdmi1", "hdmi2", "dp"}),
	new setting_t(PS341WU | MPG341CQR,     "00620", "pip_input", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS341WU | MAG321CURV | MAG321CQR | MAG271CQR | MPG341CQR | MPG27CQ,
		                                   "00630", "pip_size", {"small", "medium", "large"}),
	new setting_t(PS341WU | MAG321CURV | MAG321CQR | MAG271CQR | MPG341CQR | MPG27CQ,
		                                   "00640", "pip_position", {"left_top", "right_top", "left_bottom", "right_bottom"}),
	new setting_t(PS341WU | MAG321CURV | MAG321CQR | MAG271CQR | MPG341CQR | MPG27CQ, WRITE,
		                                   "00650", "toggle_display", {"-off", "on"}),  // returns 56006
	new setting_t(MAG321CURV | MAG321CQR | MAG271CQR | MPG27CQ, WRITE,
		                                   "00660", "toggle_sound", {"-off", "on"}),  // returns 56006, but used to toggle audio in app, no response packet - only works with "1"
	new setting_t(PS341WU | MPG341CQR,     "00660", "pip_sound_source", {"hdmi1", "hdmi2", "dp", "usbc"}),  // returns 56006, but used to toggle audio in app, no response packet - only works with "1"
	new setting_t(PS341WU | MPG341CQR,     "00670", "pbp_input1", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS341WU | MPG341CQR,     "00680", "pbp_input2", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS341WU | MPG341CQR,     "00690", "pbp_input3", {"hdmi1", "hdmi2", "dp", "usbc"}),
	new setting_t(PS341WU,                 "006:0", "pbp_input4", {"hdmi1", "hdmi2", "dp", "usbc"}), // see issue #22
	new setting_t(PS341WU | MPG341CQR,     "006;0", "pbp_sound_source", {"hdmi1", "hdmi2", "dp", "usbc"}),

	// OSD Language is dangerous and at least on the MPG341 it is broken and
	// writing a wrong value can harm the monitor. Therefore it is completely disabled for the MPG341 series
	// and read-only on the MAG and PS341WU.
	// It is also not supported by the GamingOSD app as of version 2.49
	// On MAG Series:
	// returns 001 -> value = '0' + language, 0 chinese, 1 English, 2 French, 3 German, ... maximum value "C"
	// On PS341WU Series:
	// returns 001 -> value = '0' + language, 0 chinese, 1 English, 2 French, 3 German, ... maximum value "K"
	(new setting_t(MAG,                    "00800", "osd_language", 0, 19, -100))->set_access(READ),
	(new setting_t(PS341WU,                "00800", "osd_language", 0, 28, -100))->set_access(READ),
	new setting_t(ALL,                     "00810", "osd_transparency", 0, 5),  // returns 000
	new setting_t(ALL,                     "00820", "osd_timeout",0, 30),  // returns 020
	new setting_t(PS341WU | MAG274GRP | MPG273CQR | MAG321QR,
		                                   "00830", "screen_info", {"off", "on"}),
	// Reset is considered dangerous as well
	// Completely disable
	// new setting_t(ALL, WRITE,              "00840", "reset", {"-off", "on"}),  // returns 56006 - reset monitors

	new setting_t(MAG,                     "00850", "sound_enable", {"off", "on"}),  // returns 001 - digital/anlog as on some screenshots?
	new setting_t(PS341WU | MPG341CQR | MPG27CQ,
		                                   "00850", "audio_source", {"analog", "digital"}),  // returns 001 - digital/anlog as on some screenshots?
	new setting_t(MAG | MPG | MAG321QR,               "00860", "rgb_led", {"off", "on"}),

	new setting_t(MPG273CQR | MAG321QR,               "00880", "power_button", {"off", "standby"}),
	new setting_t(MPG273CQR | MAG321QR,               "008:0", "hdmi_cec", {"off", "on"}),
	new setting_t(MPG273CQR,               "008<0", "ambient_brightness", {"off", "auto", "custom"}),
	//new setting_t(MPG273CQR,                  "008<1", "test1"), // auto-brightness copy?
	new setting_t(MPG273CQR,               "008<2", "ambient_rgb", {"off", "on"}),
	new setting_t(MPG273CQR,               "008<3", "ambient_brightness_custom", 0, 100),
	new setting_t(MPG273CQR | MAG321QR,               "008>0", "kvm", {"auto", "upstream", "type_c"}),
	new setting_t(MPG273CQR,               "008=0", "sound_tune", {"off", "on"}),

	new setting_t(MPG273CQR,               "00900", "navi_up",    {"off", "brightness", "game_mode", "smart_crosshair", "alarm_clock", "input", "refresh_rate" , "info", "night_vision", "kvm"}),
	new setting_t(MPG273CQR,               "00910", "navi_down",  {"off", "brightness", "game_mode", "smart_crosshair", "alarm_clock", "input", "refresh_rate" , "info", "night_vision", "kvm"}),
	new setting_t(MPG273CQR,               "00920", "navi_left",  {"off", "brightness", "game_mode", "smart_crosshair", "alarm_clock", "input", "refresh_rate" , "info", "night_vision", "kvm"}),
	new setting_t(MPG273CQR,               "00930", "navi_right", {"off", "brightness", "game_mode", "smart_crosshair", "alarm_clock", "input", "refresh_rate" , "info", "night_vision", "kvm"}),

	new setting_t(MAG272GRP,               "00900", "navi_up",    {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "refresh_rate" , "info"}),
	new setting_t(MAG272GRP,               "00910", "navi_down",  {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "refresh_rate" , "info"}),
	new setting_t(MAG272GRP,               "00920", "navi_left",  {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "refresh_rate" , "info"}),
	new setting_t(MAG272GRP,               "00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "refresh_rate" , "info"}),

	new setting_t(MAG321QR,                "00900", "navi_up",    {"off", "brightness", "game_mode", "alarm_clock", "smart_crosshair", "input", "refresh_rate" , "info", "night_vision", "kvm"}),
	new setting_t(MAG321QR,                "00910", "navi_down",  {"off", "brightness", "game_mode", "alarm_clock", "smart_crosshair", "input", "refresh_rate" , "info", "night_vision", "kvm"}),
	new setting_t(MAG321QR,                "00920", "navi_left",  {"off", "brightness", "game_mode", "alarm_clock", "smart_crosshair", "input", "refresh_rate" , "info", "night_vision", "kvm"}),
	new setting_t(MAG321QR,                "00930", "navi_right", {"off", "brightness", "game_mode", "alarm_clock", "smart_crosshair", "input", "refresh_rate" , "info", "night_vision", "kvm"}),

	new setting_t(MAG241GRP,               "00900", "navi_up",    {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate"}),
	new setting_t(MAG241GRP,               "00910", "navi_down",  {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate"}),
	new setting_t(MAG241GRP,               "00920", "navi_left",  {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate"}),
	new setting_t(MAG241GRP,               "00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate"}),

	new setting_t(MPG341CQR,               "00900", "navi_up",    {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate", "audio_volume"}),
	new setting_t(MPG341CQR,               "00910", "navi_down",  {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate", "audio_volume"}),
	new setting_t(MPG341CQR,               "00920", "navi_left",  {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate", "audio_volume"}),
	new setting_t(MPG341CQR,               "00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate", "audio_volume"}),

	new setting_t(MAG274GRP,               "00900", "navi_up", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate", "info"}),
	new setting_t(MAG274GRP,               "00910", "navi_down", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate", "info"}),
	new setting_t(MAG274GRP,               "00920", "navi_left", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate", "info"}),
	new setting_t(MAG274GRP,               "00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "refresh_rate", "info"}),

	new setting_t(MAG321CURV | MAG321CQR | MAG271CQR | MPG27CQ, "00900", "navi_up", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),
	new setting_t(MAG321CURV | MAG321CQR | MAG271CQR | MPG27CQ, "00910", "navi_down", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),
	new setting_t(MAG321CURV | MAG321CQR | MAG271CQR | MPG27CQ, "00920", "navi_left", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),
	new setting_t(MAG321CURV | MAG321CQR | MAG271CQR | MPG27CQ, "00930", "navi_right", {"off", "brightness", "game_mode", "screen_assistance", "alarm_clock", "input", "pip", "refresh_rate"}),

	new setting_t(PS341WU,                 "00900", "navi_up",   {"off", "brightness", "pro_mode", "screen_assistance", "alarm_clock", "input", "pip", "zoom_in", "info"}),
	new setting_t(PS341WU,                 "00910", "navi_down", {"off", "brightness", "pro_mode", "screen_assistance", "alarm_clock", "input", "pip", "zoom_in", "info"}),
	new setting_t(PS341WU,                 "00920", "navi_left", {"off", "brightness", "pro_mode", "screen_assistance", "alarm_clock", "input", "pip", "zoom_in", "info"}),
	new setting_t(PS341WU,                 "00930", "navi_right",{"off", "brightness", "pro_mode", "screen_assistance", "alarm_clock", "input", "pip", "zoom_in", "info"}),
});

static setting_t *get_read_setting(series_t series, std::string opt)
{
	for (auto &s : settings)
		if (s->m_opt == opt && (s->m_access==READWRITE || s->m_access==READ)
				&& (s->m_series & series))
			return s;
	return nullptr;
}

struct led_group_data
{
	uint8_t mode       = 0x00;    // off
	uint8_t r          = 0xff;
	uint8_t g          = 0xff;
	uint8_t b          = 0xff;
	uint8_t speed      = 0x01;
	uint8_t brightness = 0x64;
	uint8_t c6         = 0x00;
	uint8_t c7         = 0xff;
	uint8_t c8         = 0x00;
	uint8_t mode_flag  = 0x80; // seems to depend on mode : https://gitlab.com/CalcProgrammer1/OpenRGB/-/blob/master/Controllers/MSIOptixController/MSIOptixController.cpp
	uint8_t c10        = 0x00;
};

template <std::uint8_t REQUEST, std::size_t GROUPS, std::size_t MAX_LEDS>
struct led_data_generic
{
	led_data_generic()
	{
		for (std::size_t i=0; i<MAX_LEDS; i++)
		{
			rgb[0][i*3 + 0] = 0xff;
			rgb[0][i*3 + 1] = 0x00;
			rgb[0][i*3 + 2] = 0x00;

			rgb[1][i*3 + 0] = 0xff;  // red
			rgb[1][i*3 + 1] = 0xff;  // green
			rgb[1][i*3 + 2] = 0xff;  // blue
		}
	}

	void set_rgb(uint8_t group, uint8_t r, uint8_t g, uint8_t b)
	{
		for (int i=0; i<MAX_LEDS; i++)
		{
			rgb[group][i*3 + 0] = r;
			rgb[group][i*3 + 1] = g;
			rgb[group][i*3 + 2] = b;
		}
	}

	void set_rgb(uint8_t group, std::vector<unsigned> colors)
	{
		std::size_t i = 0;
		std::size_t m = colors.size();
		unsigned v = 0;
		if (m > MAX_LEDS)
			m = MAX_LEDS;
			
		while (i < m)
		{
			v = colors[i];
			rgb[group][i*3 + 0] = (v >> 16) & 0xff;
			rgb[group][i*3 + 1] = (v >>  8) & 0xff;
			rgb[group][i*3 + 2] = (v >>  0) & 0xff;
			i++;
		}
		while (i < MAX_LEDS)
		{
			rgb[group][i*3 + 0] = (v >> 16) & 0xff;
			rgb[group][i*3 + 1] = (v >>  8) & 0xff;
			rgb[group][i*3 + 2] = (v >>  0) & 0xff;
			i++;
		}
	}

	void set_mode(uint8_t group, uint8_t m)
	{
		h[group].mode = m;
	}

	uint8_t request_id = REQUEST;
	led_group_data h[GROUPS];
	uint8_t rgb[GROUPS][MAX_LEDS * 3];
	uint8_t fill = 0x00;
};

// total size should be 78
//0040   71 01 00 00 00 01 64 00 00 00 00 00 01 00 00 00   q.....d.........
//0050   01 64 00 00 00 00 00 ff 00 00 ff 00 00 ff 00 00   .d.....ÿ..ÿ..ÿ..
static_assert(sizeof(led_data_generic<0x71, 2, 9>) == 78, "size mismatch 2!");

static_assert(sizeof(led_data_generic<0x72, 2, 24>) == 168, "size mismatch 1!");

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
// FIXME: The following are modes from issue #33 (MAG321QR) dump
// Unknown1: 0x19
// Unknown2: 0x21


static int str_to_mode(std::string opt, uint8_t &mode)
{
	if (opt == "off")
		{ mode = 0; return 0; }
	else if (opt == "static")
		{ mode = 1; return 0; }
	else if (opt == "breathing")
		{ mode = 2; return 0; }
	else if (opt == "blinking")
		{ mode = 3; return 0; }
	else if (opt == "flashing")
		{ mode = 5; return 0; }
	else if (opt == "blinds")
		{ mode = 6; return 0; }
	else if (opt == "meteor")
		{ mode = 8; return 0; }
	else if (opt == "rainbow")
		{ mode = 0x1a; return 0; }
	else if (opt == "random")
		{ mode = 0x1f; return 0; }
	
	return error(E_SYNTAX, "Error decoding mystic mode: %s", opt);;
}

template <std::uint8_t REQUEST, std::size_t GROUPS, std::size_t MAX_LEDS>
static int mystic_opt(std::string opt, std::size_t supported_group_mask, led_data_generic<REQUEST, GROUPS, MAX_LEDS> &leds)
{
	uint8_t group = 0;
	uint8_t mode = 0;

	auto p = splitstr(opt, ':');
	if (p.size() < 2 || p.size() > 3)
		return error(E_SYNTAX, "Error decoding mystic options: %s", opt);
	if (auto e = string_to_unsigned(p[0], group, static_cast<uint8_t>(0), static_cast<uint8_t>(1)) > 0)
		return e;
	if (auto e = str_to_mode(p[1], mode) > 0)
		return e;

	if (((1 << group) & supported_group_mask) == 0)
		return error(E_SYNTAX, "Error mystic group not supported: %d", group);

	leds.set_mode(group, mode);
	
	if (p.size() == 3)
	{
		std::vector<unsigned> colors;
		if (auto e = number_list(splitstr(p[2], ','), colors) > 0)
			return e;
		leds.set_rgb(group, colors);
	}
	return 0;
}

class mondev_t : public usbdev_t
{
public:
	mondev_t(logger_t &logger, const device_info &info,
		const std::string &sProduct, const std::string &sSerial)
	: usbdev_t(logger, info, sProduct, sSerial)
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

	template <typename T>
	int write_led(uint16_t val, T &data)
	{
		return control_msg_write(0x21, 0x09, val, 0,
			&data, static_cast<int>(sizeof(T)), 1000);
	}

	template <typename T>
	int read_led(uint16_t val, T &data, int &retsize)
	{
		return control_msg_read(0xa1, 0x01, val, 0,
			&data, static_cast<int>(sizeof(T)), 1000, retsize);
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
			//    Game mode not support on PS341 - thus not covered here
			// For MPG341CQR, game mode (00200) always returns 5b00000
			//    language (00800) pos 5,6 are 0 bytes
			// FIXME: looks like the return starts at pos 8
			if (ret.size() > cmd.size()
				&& (ret.substr(0, cmd.size()) == std::string("5b") + setting.m_cmd
					|| cmd == "5800260"                           // alarm clock
					|| ret.substr(0, cmd.size()) == "5b00000"     //  game_mode
				))
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
	}

};

static void help_set(series_t series, series_t exclude1, series_t exclude2, series_t exclude3 = UNKNOWN)
{
	static std::array<const char *, 3> access_str = { "R", "W", "RW" };
	for (auto &s : settings)
		if (/*(s->m_access == WRITE || s->m_access == READWRITE)
			&& */((s->m_series & series) == series)
			&& (((s->m_series & exclude1) != exclude1) && ((s->m_series & exclude2) != exclude2)
				&& ((s->m_series & exclude3) != exclude3)))
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
		"the device. Once completed query settings are queried.\n"
		"The --wait option which will be executed last.\n"
		"\n"
		"All device settings provide which operations are possible:\n"
		"R: Read, W: Write, RW: Read/Write\n"
		"\n"
		"\nWarning:\n"
		"    Use msigd only if you are sure what you are doing.\n"
		"    The monitor firmware seems to have no protection against unsupported accesses.\n"
		"    Using msigd may make your monitor permantently unusable.\n"
		"\nOptions:\n\n"
		"  -q, --query                display all monitor settings. This will also\n"
		"                               list readonly settings.\n"
		"  -l, --list                 list all available monitors.\n"
		"                               Obtains a comma separated list of all\n"
		"                               MSI monitors connected. The first element\n"
		"                               in the list is the monitor number to be used\n"
		"                               as the argument to the --monitor option\n"
		"  -m, --monitor              logical monitor number.\n"
		"                               The argument to this option is the monitor\n"
		"                               number as provided by the --list option\n"
		"                               If omitted, the first monitor found is used\n"
		"  -s, --serial               serial number of the monitor to control.\n"
		"                               Use the serial number to identify the target\n"
		"                               monitor in a multi-monitor environment\n"
		"                               If omitted and --monitor is omitted as well\n"
		"                               the first monitor found is used\n"
		"      --info                 display device information. This can be used\n"
		"                               with --query\n"
		"  -f, --filter               limits query result to comma separated list\n"
		"                               of settings, e.g. -f contrast,gamma\n"
		"  -w, --wait                 SETTING=VALUE. Wait for SETTING to become\n"
		"                               VALUE, e.g. macro_key=pressed\n"
		"  -n, --numeric              monitor settings are displayed as numeric\n"
		"                               settings\n"
		, appname);
	pprintf("%s",
		"       --mystic              Expects ledgroup:mode:colors\n"
		"                               ledgroup is 0 or 1, colors is a comma\n"
		"                               separated list of colors to be written to\n"
		"                               leds in ledgroup. The last element is used\n"
		"                               to set remaining leds. mode is one of:\n"
		"                               off, static, breathing, blinking, flashing,\n"
		"                               blinds, meteor, rainbow, random,\n"
		"                               Color format is 0xRRGGBB\n"
		"                               Only available on supported monitors.\n");
	pprintf("%s", "\nMulti monitor support:\n");
	pprintf("%s", "    Use --list to get a list of all attached monitors.\n"
		    "    If the serial numbers provided by this list are unique,\n"
		    "    you can use the serial numbers to identify monitors using\n"
		    "    the --serial option. If you have multiple monitors of the\n"
		    "    same type this is most likely not the case. This is an MSI\n"
		    "    issue. In this case use the --monitor option to specify the\n"
		    "    logical monitor number provided by the --list option\n\n");
	pprintf("%s", "\nAll monitors:\n");
	pprintf("%s", "    These options apply to all monitors:\n\n");
	help_set(ALL, UNKNOWN, UNKNOWN);
	pprintf("%s", "\nMAG series monitors:\n");
	pprintf("%s", "    These options apply to all MAG monitors:\n\n");
	help_set(MAG, ALL, UNKNOWN);
	pprintf("%s", "\nMPG series monitors:\n");
	pprintf("%s", "    These options apply to all MPG monitors:\n\n");
	help_set(MPG, ALL, UNKNOWN);
	for (std::size_t i=1; i<known_models.size(); i++)
	{
		pprintf("\n%s:\n", known_models[i].name);
		pprintf("    These options apply to the %s:\n\n", known_models[i].name);
		help_set(known_models[i].series, ALL, MAG, MPG);
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

static int test_steel_device(steeldev_t &steeldev, std_logger_t &logger)
{
	logger(DEBUG, "Testing group color");
	static const auto cSTEEL_DELAY = std::chrono::milliseconds(1000);
	steeldev.global_illumination(0xff);
	steeldev.flush();
	steeldev.write_all_leds(0xff, 0xff, 0xff);
	steeldev.flush();
	std::this_thread::sleep_for(cSTEEL_DELAY);
	steeldev.write_all_leds(0xff, 0x00, 0x00);
	steeldev.flush();
	std::this_thread::sleep_for(cSTEEL_DELAY);
	steeldev.write_all_leds(0x00, 0xff, 0x00);
	steeldev.flush();
	std::this_thread::sleep_for(cSTEEL_DELAY);
	steeldev.write_all_leds(0x00, 0x00, 0xff);
	steeldev.flush();
	logger(DEBUG, "Testing single color");
	for (uint8_t i = 0; i<0x28; i++)
	{
		std::this_thread::sleep_for(cSTEEL_DELAY / 10);
		steeldev.write_led(i, 0xff, 0xff, 0x00);
		steeldev.flush();
	}
	logger(DEBUG, "Testing global illumination 0 to 255, step 8");
	for (uint16_t i = 0; i<0xff; i+=0x08)
	{
		std::this_thread::sleep_for(cSTEEL_DELAY / 10);
		steeldev.global_illumination(i);
		steeldev.flush();
	}
	logger(DEBUG, "Enabling colorshift for five seconds");
	steeldev.colorshift_all_leds(0x01);
	steeldev.flush();
	std::this_thread::sleep_for(cSTEEL_DELAY * 5);
	logger(DEBUG, "Sending b record ... Waiting 5 seconds");
	steel_data_0b data_0b(true);
	steeldev.write_0b(data_0b);
	steeldev.flush();
	std::this_thread::sleep_for(cSTEEL_DELAY * 5);
	logger(DEBUG, "Disabling colorshift for two seconds");
	steeldev.colorshift_all_leds(0x00);
	steeldev.flush();
	std::this_thread::sleep_for(cSTEEL_DELAY * 2);
	return E_OK;
}

static int arg_to_u(unsigned &v, const std::string &s, const std::string &name, unsigned max)
{
	std::size_t idx(std::string::npos);
	try
	{
		v = std::stoul(s, &idx, 10);
	}
	catch (...)
	{
	}
	if (s.size() != idx)
		return error(E_SYNTAX, "Error decoding %s: %s", name, s);
	if (v > max)
		return error(E_SYNTAX, "Error %s max value is %d: %s", name, max, s);
	return E_OK;
}

static std::unordered_map<std::string, std::pair<unsigned, unsigned>>
steel_groups =
{
	{ "G1", {0x00,0x07} },
	{ "G2", {0x08,0x0f} },
	{ "G3", {0x10,0x17} },
	{ "G4", {0x18,0x1f} },
	{ "G5", {0x20,0x27} },
	{ "G6", {0x28,0x3e} },
	{ "G7", {0x3f,0x66} },
};

static int steel_main(std_logger_t &logger, int argc, char **argv)
{
	logger.set_level(DEBUG, true);
	//steeldev_t steeldev(logger, 0x1462, 0x3fa4, "MSI Gaming Controller");
	steeldev_t steeldev(logger, 0x1038, 0x1126, "SteelSeries MLC", "");
	//part of the code may as well work on keyboards with per key led.
	// Examples are GE63, GE73 with usb ids 1038:1122

	if (!steeldev)
		return error(E_IDENTIFY, "No steel series usb device found", 0);

	int argp = 0;
	int ret = 0;
	unsigned profile = 0;
	std::array<steel_data_0b, 16> profile_data;
	std::array<std::vector<unsigned>, 16> profile_cols;
	std::array<std::vector<unsigned>, 16> profile_dur;
	std::array<int, 16> profile_speed;

	// First entry has profile id.
	for (std::size_t i = 0; i < profile_data.size(); i++)
	{
		profile_data[i].col[0].e[0] = i;
		profile_speed[i] = 5;
	}

	while (argp < argc)
	{
		std::string cur_opt(argv[argp]);

		if (cur_opt == "--test")
		{
			if ((ret = test_steel_device(steeldev, logger)) > 0)
				return ret;
		}
		else if (cur_opt == "--persist")
		{
			steeldev.persist();
		}
		else if (cur_opt == "--flush")
		{
			steeldev.flush();
		}
		else if (cur_opt == "--color" && argp + 1 < argc)
		{
			unsigned start(0);
			unsigned end(0);

			auto p = splitstr(argv[++argp], ':');
			if (p.size() != 2)
				return error(E_SYNTAX, "Error decoding range and color: %s", argv[argp]);
			printf("%s\n", p[0].c_str());
			auto git = steel_groups.find(p[0].c_str());
			if (git != steel_groups.end())
			{
				start = git->second.first;
				end = git->second.second;
			}
			else
			{
				auto r = splitstr(p[0], '-');
				if (r.size() > 2)
					return error(E_SYNTAX, "Error decoding range and color: %s", argv[argp]);

				if (arg_to_u(start, r[0], "range start", 40*2+23-1) != 0
					|| arg_to_u(end, r[r.size() == 2 ? 1 : 0], "range end", 40*2+23-1) != 0 || start > end)
					return error(E_SYNTAX, "max led num is %d - parameter error: %s", 40*2 + 23 - 1, argv[argp]);
			}
			std::size_t idx(0);
			if (!p[1].empty() && p[1][0] == 'P')
			{
				unsigned v(0);
				if (arg_to_u(v, p[1].substr(1), "led profile", 15) !=0 )
					return error(E_SYNTAX, "Error decoding color: %s", argv[argp]);
				auto i=start;
				for (; i+40 <= end; i+=40)
				{
					logger(DEBUG, "setting profile %d-%d:P%d", i, 40, v);
					steeldev.write_led_profile(i, i + 40 - 1, v);
				}
				if (i <= end)
				{
					logger(DEBUG, "setting profile %d-%d:P%d", i, 40, v);
					steeldev.write_led_profile(i, end, v);
				}
			}
			else
			{
				auto col = std::stoul(p[1], &idx, 16);
				if (idx != p[1].size())
					return error(E_SYNTAX, "Error decoding color: %s", argv[argp]);

				auto i=start;
				for (; i+40 <= end; i+=40)
				{
					logger(DEBUG, "setting color %d-%d:%06x", i, 40, col);
					steeldev.write_led_range(i, i + 40 - 1, (col >> 16) & 0xff, (col >> 8) & 0xff, col & 0xff);
				}
				if (i <= end)
				{
					logger(DEBUG, "setting color %d-%d:%06x", i, end - i + 1, col);
					steeldev.write_led_range(i, end, (col >> 16) & 0xff, (col >> 8) & 0xff, col & 0xff);
				}
			}
		}
		else if (cur_opt == "--illum" && argp + 1 < argc)
		{
			unsigned i = 0;
			int ret = 0;
			if ((ret = arg_to_u(i, argv[++argp], "illum", 255))>0)
				return ret;
			steeldev.global_illumination(i);
		}
		else if (cur_opt == "--delay" && argp + 1 < argc)
		{
			unsigned i = 0;
			int ret = 0;
			if ((ret = arg_to_u(i, argv[++argp], "delay", 10000))>0)
				return ret;
			std::this_thread::sleep_for(std::chrono::milliseconds(i));
		}
		else if (cur_opt == "--profile" && argp + 1 < argc)
		{
			int ret = 0;
			if ((ret = arg_to_u(profile, argv[++argp], "profile", 16))>0)
				return ret;
			logger(DEBUG, "setting current profile to %d", profile);
		}
		else if (cur_opt == "--wave_speed" && argp + 1 < argc)
		{
			int ret = 0;
			unsigned val = 0;
			if ((ret = arg_to_u(val, argv[++argp], "wave_speed", 100))>0)
				return ret;
			logger(DEBUG, "setting wave speed for profile %d to %d", profile, val);
			profile_data[profile].set_wave_speed(val);
			steeldev.write_0b(profile_data[profile]);
		}
		else if (cur_opt == "--wave_mode" && argp + 1 < argc)
		{
			int ret = 0;
			unsigned val = 0;
			if ((ret = arg_to_u(val, argv[++argp], "wave_mode", 5))>0)
				return ret;
			logger(DEBUG, "setting wave mode for profile %d to %d (status %s)", profile, val, val ? "enabled" : "disabled");
			profile_data[profile].set_wave_mode(val);
			steeldev.write_0b(profile_data[profile]);
		}
		else if (cur_opt == "--pcolors" && argp + 1 < argc)
		{
			auto &cols = profile_cols[profile];
			auto &dur = profile_dur[profile];
			cols.clear();
			dur.clear();
			auto entries = splitstr(argv[++argp], ',');

			for (auto &e : entries)
			{
				try
				{
					auto sp = splitstr(e, ':');
					if (sp.size() != 2)
						return error(E_SYNTAX, "Error decoding profile color <%s> from: %s", e, argv[argp]);
					std::size_t idx(0);
					auto d = std::stoul(sp[0], &idx, 10);
					if (idx != sp[0].size() || d > 10000)
						return error(E_SYNTAX, "Error decoding profile color <%s> from: %s", e, argv[argp]);
					auto col = std::stoul(sp[1], &idx, 16);
					if (idx != sp[1].size())
						return error(E_SYNTAX, "Error decoding profile color <%s> from: %s", e, argv[argp]);
					cols.push_back(col);
					dur.push_back(d);
				}
				catch (...)
				{
					return error(E_SYNTAX, "Error decoding profile color <%s> from: %s", e, argv[argp]);
				}
			}
			if (cols.size() > 16)
				return error(E_SYNTAX, "Error: too many profile colors (max 16): %s", argv[argp]);
			logger(DEBUG, "setting %d colors for profile %d", cols.size(), profile);
			profile_data[profile].set_colors(cols, dur, profile_speed[profile]);
			steeldev.write_0b(profile_data[profile]);
		}
		else if (cur_opt == "--pspeed" && argp + 1 < argc)
		{
			int ret = 0;
			unsigned val = 0;
			if ((ret = arg_to_u(val, argv[++argp], "pspeed", 30))>0)
				return ret;
			if (val < 1)
				return error(E_SYNTAX, "Error: pspeed (min 1): %s", argv[argp]);
			auto &cols = profile_cols[profile];

			logger(DEBUG, "setting speed %d %d", val, profile);
			profile_data[profile].set_colors(cols, profile_dur[profile], val);
			steeldev.write_0b(profile_data[profile]);
			profile_speed[profile] = val;
		}
		else
		{
			return error(E_SYNTAX, "Unknown option: %s", cur_opt);
		}
		argp++;
	}
	return E_OK;
}

int main (int argc, char **argv)
{
	int arg_pointer = 1;
	bool query = false;
	bool debug = false;
	bool dump_mystic = false;
	bool info = false;
	bool steel = false;
	bool list = false;
	unsigned long monitor = 0;
	string_list mystic_opts;
	bool numeric = false;
	string_list filters;
	std::string waitfor;
	std::string serial;

	std_logger_t logger;

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
		else if (cur_opt == "--steel")
		{
			// anything behind the --steel parameter is interpreted as steel commands
			steel = true;
			++arg_pointer;
			break;
		}
		else if ((cur_opt == "--serial" || cur_opt == "-s") && arg_pointer + 1 < argc)
		{
			serial = argv[++arg_pointer];
		}
		else if ((cur_opt == "--monitor" || cur_opt == "-m") && arg_pointer + 1 < argc)
		{
			++arg_pointer;
			std::size_t idx = 0;
			std::string arg = argv[arg_pointer];
			try {
				monitor = std::stoul(arg, &idx, 10);
			}
			catch (...)
			{
			}
			if (idx != arg.size())
				return error(E_SYNTAX, "Error decoding monitor number: %s", arg);
		}

		else if (cur_opt == "--list" || cur_opt == "-l")
			list = true;
		else if ((cur_opt == "--filter" || cur_opt == "-f") && arg_pointer + 1 < argc)
		{
			filters = splitstr(argv[++arg_pointer], ',');
			query = true;
		}
		else if ((cur_opt == "--wait" || cur_opt == "-w") && arg_pointer + 1 < argc)
		{
			waitfor = argv[++arg_pointer];
		}
		else if (cur_opt == "--dump_mystic")
		{
			dump_mystic = true;
		}
		else if (cur_opt == "--mystic" && arg_pointer + 1 < argc)
		{
			++arg_pointer;
			mystic_opts.push_back(argv[arg_pointer]);
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

	if (steel)
	{
		int ret = 0;
		if ((ret = steel_main(logger, argc - arg_pointer, argv + arg_pointer)) > 0)
			return ret;
	}


	logger.set_level(DEBUG, debug);

	device_info_list monitor_list;

	if (usbdev_t::get_device_list(logger, 0x1462, 0x3fa4, monitor_list) != 0)
		return error(E_SYNTAX, "No msigd target monitors found\n");

	if (monitor > monitor_list.size())
		return error(E_SYNTAX, "Invalid monitor number: %d", monitor);

	if (list)
	{
		int idx = 1;
		for (auto &e : monitor_list)
		{
			mondev_t mon(logger, e, "MSI Gaming Controller", "");
			pprintf("%d,%s,%s,%s,%s\n", idx, mon.serial(), mon.manufacturer(), mon.product(), e.path);
			idx ++;
		}
		//return E_OK;
	}

	mondev_t usb(logger, monitor_list[monitor ? monitor - 1 : 0], "MSI Gaming Controller", serial);

	if (usb)
	{
		bool notify(false);
		// Try to identify the monitor
		std::string s140;
		std::string s150;
		identity_t &series = known_models[0];

		setting_t sp140(ALL, "00140", "sp140");
		setting_t sp150(ALL, "00150", "sp150");

#if (!USE_HID)
		// improve stability with usb api and avoid errors by issuing this command here
		usb.debug_cmd("\x01\xb4");
#endif

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
			std::string leds = (series.leds == LT_NONE ? "None"
				: (series.leds == LT_MYSTIC ? "Mystic"
				: (series.leds == LT_MYSTIC_OPTIX ? "MysticOptix"
				: (series.leds == LT_STEEL ? "Steel"
				: "Error"))));


			pprintf("Vendor Id:      0x%04x\n", usb.vendor_id());
			pprintf("Product Id:     0x%04x\n", usb.product_id());
			pprintf("Product:        %s\n",     usb.product());
			pprintf("Serial:         %s\n",     usb.serial());
			pprintf("Monitor Series: %s\n",     series.name);
			pprintf("LED support:    %s\n",     leds);
			if (debug)
			{
				pprintf("s140:           <%s>\n", s140);
				pprintf("s150:           <%s>\n", s150);
				usb.debug_cmd("\x01\xb0");
				usb.debug_cmd("\x01\xb4");
				// set after MSI app 20191206 0.0.2.23
				if ((series.series & MAG241GRP) == 0) // times out on MAG241GRP
					usb.debug_cmd("\x01\xd0""b00100000\r");

				// queried but not supported on MAG321CURV (returns 56006)
				//usb.debug_cmd("\x01""5800190\r");
				//usb.debug_cmd("\x01""5800130\r");
			}
		}

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

		if (series.series == QUERYONLY && set_encoded.size() > 0)
			return error(E_SYNTAX, "Unknown monitor - write access disabled");

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

		if (dump_mystic)
		{
			unsigned char data[200] = {0x72, 0x00};
			int retsize = 0;
			if (!usb.read_led(0x0372, data, retsize))
			{
				pprintf("Mystic record size: %d\n", retsize);
				for (std::size_t i = 0; i < retsize; i++)
				{
					if (i % 8 == 0)
						pprintf("\n%04x: ", i);
					pprintf("%02x ", data[i]);
				}
				pprintf("\n");
			}


		}
		// Set mystic leds
		if (mystic_opts.size() > 0)
		{
			if (series.leds == led_type_t::LT_MYSTIC)
			{
				//led_data leds;
				led_data_generic<0x71, 2, 9> leds;
				for (auto &s : mystic_opts)
				{
					if (auto err = mystic_opt(s, 0x02, leds) != 0)
						return err;
				}
				usb.write_led(0x0371, leds);
			}
			else if (series.leds == led_type_t::LT_MYSTIC_OPTIX)
			{
				led_data_generic<0x72, 2, 24> leds_optix;
				for (auto &s : mystic_opts)
				{
					if (auto err = mystic_opt(s, 0x03, leds_optix) != 0)
						return err;
				}
				usb.write_led(0x0372, leds_optix);
			}
			else
				return error(E_SYNTAX, "--mystic not supported");
		}

		// set values
		if (cWRITE_ENABLED)
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

