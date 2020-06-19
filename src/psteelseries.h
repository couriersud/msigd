/*
 * psteelseries.h
 *
 */

#ifndef PSTEELSERIES_H_
#define PSTEELSERIES_H_

#if USE_HID
#include "phid.h"
#else
#include "pusb.h"
#endif

#include <array>

struct steel_rgb_entry
{

	steel_rgb_entry() {}
	steel_rgb_entry(uint8_t n, uint8_t vr, uint8_t vg, uint8_t vb)
	{
		num = n;
		set_col = 1;
		r = vr; g = vg; b = vb;
	}

	void set_rgb(uint8_t n, uint8_t vr, uint8_t vg, uint8_t vb)
	{
		num = n;
		set_col = 0x01;
		r = vr; g = vg; b = vb;
	}

	void set_mode(uint8_t n, uint8_t val)
	{
		num = n;
		set_col = 0x00;
		mode = val;
		r = 0; g = 0; b = 0;
	}

	uint8_t r       = 0x00; // ??
	uint8_t g       = 0x00; // ??
	uint8_t b       = 0x00; // ??
	uint8_t f03     = 0x00; // ??
	uint8_t f04     = 0x00; // ??
	uint8_t f05     = 0x00; // ??
	uint8_t f06     = 0x00; // ??
	uint8_t f07     = 0x00; // ??
	uint8_t mode    = 0x00; // ??
	uint8_t set_col = 0x00; // ??
	uint8_t f10     = 0x00; // ??
	uint8_t num     = 0x00; // ??

};

struct steel_data_0e
{
	// Set color constructor
	steel_data_0e(uint8_t start, uint8_t end, uint8_t r, uint8_t g, uint8_t b)
	{
		num_rec = end - start + 1;
		for (int i=start; i<=end; i++)
		{
			entries[i-start].set_rgb(i, r, g, b);
		}
	}

	// set mode constructor
	steel_data_0e(uint8_t start, uint8_t end, uint8_t mode)
	{
		num_rec = end - start + 1;
		for (int i=start; i<=end; i++)
		{
			entries[i-start].set_mode(i, mode);
		}
	}

	// total size should be 525
	uint8_t report_id = 0x00;
	uint8_t command   = 0x0e; // also seen 0x0b, 0x0c and 0x0d
	uint8_t f00       = 0x00; // ??
	uint8_t num_rec   = 0x08;
	// number of records
	uint8_t f01       = 0x00; // ??
	std::array<steel_rgb_entry, 43> entries;
	uint8_t f02       = 0x00; // ??
	uint8_t f03       = 0x00; // ??
	uint8_t f04       = 0x00; // ??
	uint8_t f05       = 0x00; // ??
};

struct steel_command
{
	explicit steel_command(uint8_t com, uint8_t val = 0x00)
	: command(com), subcmd(val)
	{
		std::fill(f02.begin(), f02.end(), 0);
	}

	// total size should be 65
	uint8_t report_id = 0x00;
	uint8_t command;          // 0x09, 0x0c and 0x0d
	uint8_t f00       = 0x00; // ??
	uint8_t subcmd;
	uint8_t f01       = 0x00; // ??
	std::array<uint8_t, 60> f02;
};

struct steel_data_0b
{
	struct sub
	{
		sub() { std::fill(e.begin(), e.end(), 0); }
		sub(uint8_t e0)
		: sub()
		{
			e[0] = e0;
		}
		sub(uint8_t e0, uint8_t e1, uint8_t e2, uint8_t e3,
			uint8_t e4, uint8_t e5, uint8_t e6, uint8_t e7)
		{
			e[0] = e0; e[1] = e1; e[2] = e2; e[3] = e3;
			e[4] = e4; e[5] = e5; e[6] = e6; e[7] = e7;
		}
		std::array<uint8_t, 8> e;
	};
	steel_data_0b(uint8_t led)
	: e00(led)
	{
	}

	steel_data_0b(uint8_t led, bool)
#if 1
	//                  r     g     b
	: e00(0x01, 0x00, 0xc5, 0x00, 0x41, 0x00, 0x21, 0x00)
	, e01(0x01, 0x00, 0xc0, 0x00, 0xbf, 0x00, 0x21, 0x00)
	, e02(0x02, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x21, 0x00)
#else
	: e00(0x01, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x40, 0x00)
	, e01(0x01, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x40, 0x00)
	, e02(0x02, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x40, 0x00)
	, e03(0x03, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x40, 0x00)
	, e04(0x04, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x40, 0x00)
	, e05(0x05, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x40, 0x00)
#endif
	, e16(0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00)  // 00 00 f0 0f 00 00 10 0e
	, e19(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) // 01 00 00 00 00 00 00 00 // 0x01 ????
	{
	}

	// total size should be 525
	uint8_t report_id = 0x00;
	uint8_t command   = 0x0b; // also seen 0x0b, 0x0c and 0x0d
	uint8_t f00       = 0x00; // ??

	sub e00;
	sub e01;
	sub e02;
	sub e03;
	sub e04;
	sub e05;
	std::array<sub, 10> f01;
	sub e16;
	uint8_t e17_0 = 0xff;
	uint8_t e17_1 = 0x00;
	uint8_t e17_2 = 0x00; // 0xeb;
	uint8_t e17_3 = 0x00;
	uint8_t e17_4 = 0x00; // 0x37;
	uint8_t e17_5 = 0x00;
	uint8_t wave_mode = 0x00;  // 0x01 if enable
	uint8_t e17_7 = 0x00;
	uint8_t e18_0 = 0x00;
	uint8_t e18_1 = 0x00;
	uint8_t wave_speed = 0x80; // min 0x1e, max 0xe9
	uint8_t e18_3 = 0x00;      // changes to 0x03 on setting wave speed to max ?
	uint8_t e18_4 = 0x03;
	uint8_t e18_5 = 0x00;
	uint8_t e18_6 = 0x63;  // Fd 01 - may be colorshift speed (min 0x0063, max 0x0bd9)
	uint8_t e18_7 = 0x00;
	sub e19;
	std::array<uint8_t, 22*16 + 10> f02;
};

#if 0
0b
00
01 00 00 3f 00 00 40 00
01 00 c0 00 00 00 40 00
02 00 00 00 3f 00 40 00
03 00 00 c0 00 00 40 00
04 00 3f 00 00 00 40 00
05 00 00 00 c0 00 40 00

00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00

00 00 0f 0f 00 00 00 00
ff 00 00 00 00 00 01 00
00 00 80 00 06 00 80 01
01 00 00 00 00 00 00 00

00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
#endif

// after startup
// b - 07
// c - ff
// e -     8 leds 18-1f
// b - 03
// e -     28 leds 3f-66
// e -      8 leds 20-27
// e -      8 leds 10-17
// b - 0b
// b - 08
// b - 10
// b - 09
// b - 0f
// b - 0d
// b - 0c
// b - 04
// b - 00  ---> other bytes set as well

// later
// 9 - 00 ----> send with value 0x200
// ....
// d - 02
//
// pcap v2:
// e 3f-66 (=0x28)
// e 00-07
// b 12
// b 10
// b 06
// e 10-17
// e 08-0f
// b 11
// b 0b
// b 09
// b 05
// b 0c
// e 18-1f
// b 07
// b 04
// b 03
// c ff
// e 20-27
// b 0d
// b 08
// b 01 --> special
// e 28-3e (=0x17) color not white
// b 0f
// b 0e
// b 0a
// b 02 --> special
// b 00 --> special
// 9 00 -> persist
// e 3f-66 (=0x28) white
// e 28-3e (=0x17) white
// 9 00 -> persist
// e 3f-66 (=0x28) red 0x00 0x01 0x00 num
// e 00-07 red
// e 10-17 red
// e 08-0f red
// e 18-1f red
// e 20-27 red         0x01 0x01 0x00 num
// e 28-3e (=0x17) red 0x00 0x01 0x00 num
// 9 00 -> persist


static_assert(sizeof(steel_data_0e) == 525, "steel_data size mismatch");
static_assert(sizeof(steel_command) == 65, "steel_data size mismatch");
static_assert(sizeof(steel_data_0b) == 525, "steel_data size mismatch");

class steeldev_t : public usbdev_t
{
public:
	steeldev_t(logger_t &logger, unsigned idVendor, unsigned idProduct, const std::string &sProduct)
	: usbdev_t(logger, idVendor, idProduct, sProduct)
	{
		// no endpoints on steel device ?
	#if 0
	#if !(USE_HID)
			if (!checkep(1, false) && !checkep(2, true))
				return;
			else
				cleanup();
	#endif
	#endif
	}

	~steeldev_t()
	{
	}

	// e 3f-66 (=0x28) red 0x00 0x01 0x00 num
	// e 00-07 red
	// e 10-17 red
	// e 08-0f red
	// e 18-1f red
	// e 20-27 red         0x01 0x01 0x00 num
	// e 28-3e (=0x17) red 0x00 0x01 0x00 num
	// 9 00 -> persist

	// 10 -> device sends packet via interrupt (contains 1a 02 00 ...)

	int write_all_leds(uint8_t r, uint8_t g, uint8_t b)
	{
		// FIXME: effects not yet supported

		// We seem to have 7 groups - 0xb messages ?
		// Back led - left group of 10-10-10-10
		write_led_rec(steel_data_0e(0x3f, 0x66, r, g, b));
		// Front leds .... 8-8-8-8-8
		write_led_rec(steel_data_0e(0x00, 0x07, r, g, b));
		write_led_rec(steel_data_0e(0x10, 0x17, r, g, b));
		write_led_rec(steel_data_0e(0x08, 0x0f, r, g, b));
		write_led_rec(steel_data_0e(0x18, 0x1f, r, g, b));
		write_led_rec(steel_data_0e(0x20, 0x27, r, g, b));
		// Back led - right group of 23 (10-9-4)
		write_led_rec(steel_data_0e(0x28, 0x3e, r, g, b));
		return 0;
	}

	int write_0b(steel_data_0b &data)
	{
		return control_msg_write(0x21, 0x09, 0x300, 0,
			&data, static_cast<int>(sizeof(steel_data_0b)), 1000);
	}

	int write_led(uint8_t led, uint8_t r, uint8_t g, uint8_t b)
	{
		return write_led_rec(steel_data_0e(led, led, r, g, b));
	}

	int persist()
	{
		steel_command d(0x09);
		return write(&d, sizeof(d));
	}

	int flush()
	{
		steel_command d(0x0d, 0x02);
		return write(&d, sizeof(d));
	}

	int global_illumination(uint8_t val)
	{
		steel_command d(0x0c, val);
		return write(&d, sizeof(d));
	}

	int colorshift_all_leds(uint8_t v)
	{
		// FIXME: effects not yet supported

		// We seem to have 7 groups - 0xb messages ?
		// Back led - left group of 10-10-10-10
		write_led_rec(steel_data_0e(0x3f, 0x66, v));
		// Front leds .... 8-8-8-8-8
		write_led_rec(steel_data_0e(0x00, 0x07, v));
		write_led_rec(steel_data_0e(0x10, 0x17, v));
		write_led_rec(steel_data_0e(0x08, 0x0f, v));
		write_led_rec(steel_data_0e(0x18, 0x1f, v));
		write_led_rec(steel_data_0e(0x20, 0x27, v));
		// Back led - right group of 23 (10-9-4)
		write_led_rec(steel_data_0e(0x28, 0x3e, v));
		return 0;
	}

private:
	int write_led_rec(steel_data_0e &&data)
	{
		return control_msg_write(0x21, 0x09, 0x300, 0,
			&data, static_cast<int>(sizeof(steel_data_0e)), 1000);
	}

};

#endif /* PSTEELSERIES_H_ */

#if 0
0000   0b 00
00 00 00 16 eb 00 a8 00
01 00 e8 fe 18 00 a8 00
02 00 17 ee fe 00 ad 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00

00 00 f0 0f 00 00 10 0e
ff 00 eb 00 37 00 01 00
01 00 3c 00 03 00 fd 01

00 00 00 00 00 00
00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00c0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00d0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00f0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0100   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0110   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0120   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0130   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0140   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0150   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0160   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0170   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0180   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0190   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
01a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
01b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
01c0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
01d0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
01e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
01f0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0200   00 00 00 00 00 00 00 00 00 00 00 00
#endif
