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
		mode = 1;
		r = vr; g = vg; b = vb;
	}

	void set_rgb(uint8_t n, uint8_t vr, uint8_t vg, uint8_t vb)
	{
		num = n;
		mode = 0x01;
		r = vr; g = vg; b = vb;
		// FIXME: hardcoded
		if (n < 0x28)
			f08 = 0x01;
	}

	uint8_t r       = 0x00; // ??
	uint8_t g       = 0x00; // ??
	uint8_t b       = 0x00; // ??
	uint8_t f03     = 0x00; // ??
	uint8_t f04     = 0x00; // ??
	uint8_t f05     = 0x00; // ??
	uint8_t f06     = 0x00; // ??
	uint8_t f07     = 0x00; // ??
	uint8_t f08     = 0x00; // ??
	uint8_t mode    = 0x00; // ??
	uint8_t f10     = 0x00; // ??
	uint8_t num     = 0x00; // ??

};

struct steel_data_0e
{
	steel_data_0e(uint8_t start, uint8_t end, uint8_t r, uint8_t g, uint8_t b)
	{
		num_rec = end - start + 1;
		for (int i=start; i<=end; i++)
		{
			entries[i-start].set_rgb(i, r, g, b);
		}
	}

#if 0
	void set_rgb(uint8_t r, uint8_t g, uint8_t b)
	{
		for (int i=0; i<9; i++)
		{
			rgb[i*3 + 0] = r;
			rgb[i*3 + 1] = g;
			rgb[i*3 + 2] = b;
		}
	}
#endif

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
#if 0
	ff 00 00 00 00 00 00 00 00 01 00 00
	ff 00 00 00 00 00 00 00 00 01 00 01
	ff 00 00 00 00 00 00 00 00 01 00 02
	ff 00 00 00 00 00 00 00 00 01 00 03
	ff 00 00 00 00 00 00 00 00 01 00 04
	ff 00 00 00 00 00 00 00 00 01 00 05
	ff 00 00 00 00 00 00 00 00 01 00 06
	ff 00 00 00 00 00 00 00 00 01 00 07

	00 00 00 00 00 00 00 00 00 00 00 00  // 08
	00 00 00 00 00 00 00 00 00 00 00 00  // 09
	00 00 00 00 00 00 00 00 00 00 00 00  // 10
	00 00 00 00 00 00 00 00 00 00 00 00  // 11
	00 00 00 00 00 00 00 00 00 00 00 00  // 12
	00 00 00 00 00 00 00 00 00 00 00 00  // 13
	00 00 00 00 00 00 00 00 00 00 00 00  // 14
	00 00 00 00 00 00 00 00 00 00 00 00  // 15
	00 00 00 00 00 00 00 00 00 00 00 00  // 16
	00 00 00 00 00 00 00 00 00 00 00 00  // 17
	00 00 00 00 00 00 00 00 00 00 00 00  // 18
	00 00 00 00 00 00 00 00 00 00 00 00  // 19
	00 00 00 00 00 00 00 00 00 00 00 00  // 20
	00 00 00 00 00 00 00 00 00 00 00 00  // 21
	00 00 00 00 00 00 00 00 00 00 00 00  // 22
	00 00 00 00 00 00 00 00 00 00 00 00  // 23
	00 00 00 00 00 00 00 00 00 00 00 00  // 24
	00 00 00 00 00 00 00 00 00 00 00 00  // 25
	00 00 00 00 00 00 00 00 00 00 00 00  // 26
	00 00 00 00 00 00 00 00 00 00 00 00  // 27
	00 00 00 00 00 00 00 00 00 00 00 00  // 28
	00 00 00 00 00 00 00 00 00 00 00 00  // 29
	00 00 00 00 00 00 00 00 00 00 00 00  // 30
	00 00 00 00 00 00 00 00 00 00 00 00  // 31
	00 00 00 00 00 00 00 00 00 00 00 00  // 32
	00 00 00 00 00 00 00 00 00 00 00 00  // 33
	00 00 00 00 00 00 00 00 00 00 00 00  // 34
	00 00 00 00 00 00 00 00 00 00 00 00  // 35
	00 00 00 00 00 00 00 00 00 00 00 00  // 36
	00 00 00 00 00 00 00 00 00 00 00 00  // 37
	00 00 00 00 00 00 00 00 00 00 00 00  // 38
	00 00 00 00 00 00 00 00 00 00 00 00  // 39
	00 00 00 00 00 00 00 00 00 00 00 00  // 40
	00 00 00 00 00 00 00 00 00 00 00 00  // 41
	00 00 00 00 00 00 00 00 00 00 00 00  // 42
	00 00 00 00
#endif
};

struct steel_data_0d
{
	steel_data_0d()
	{
		std::fill(f02.begin(), f02.end(), 0);
	}

	// total size should be 65
	uint8_t report_id = 0x00;
	uint8_t command   = 0x0d; // also seen 0x0b, 0x0c and 0x0d
	uint8_t f00       = 0x00; // ??
	uint8_t subcmd    = 0x02; // 02,
	uint8_t f01       = 0x00; // ??
	std::array<uint8_t, 60> f02;
};

struct steel_data_0c
{
	steel_data_0c()
	{
		std::fill(f02.begin(), f02.end(), 0);
	}

	// total size should be 65
	uint8_t report_id = 0x00;
	uint8_t command   = 0x0c; // also seen 0x0b, 0x0c and 0x0d
	uint8_t f00       = 0x00; // ??
	uint8_t subcmd    = 0xff; // ff
	uint8_t f01       = 0x00; // ??
	std::array<uint8_t, 60> f02;
};

struct steel_data_09
{
	steel_data_09()
	{
		std::fill(f02.begin(), f02.end(), 0);
	}

	// total size should be 65
	uint8_t report_id = 0x00;
	uint8_t command   = 0x09; // also seen 0x0b, 0x0c and 0x0d
	uint8_t f00       = 0x00; // ??
	std::array<uint8_t, 62> f02;
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
	: e00(0x01, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x40, 0x00)
	, e01(0x01, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x40, 0x00)
	, e02(0x02, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x40, 0x00)
	, e03(0x03, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x40, 0x00)
	, e04(0x04, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x40, 0x00)
	, e05(0x05, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x40, 0x00)

	, e16(0x00, 0x00, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00)
	, e17(0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00)
	, e18(0x00, 0x00, 0x80, 0x00, 0x06, 0x00, 0x80, 0x01)
	, e19(0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
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
	sub e17;
	sub e18;
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
static_assert(sizeof(steel_data_0d) == 65, "steel_data size mismatch");
static_assert(sizeof(steel_data_0c) == 65, "steel_data size mismatch");
static_assert(sizeof(steel_data_09) == 65, "steel_data size mismatch");
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

	int write_led(uint8_t r, uint8_t g, uint8_t b)
	{
		// FIXME
		write_led_rec(steel_data_0e(0x3f, 0x66, r, g, b));
		write_led_rec(steel_data_0e(0x00, 0x07, r, g, b));
		write_led_rec(steel_data_0e(0x10, 0x17, r, g, b));
		write_led_rec(steel_data_0e(0x08, 0x0f, r, g, b));
		write_led_rec(steel_data_0e(0x18, 0x1f, r, g, b));
		write_led_rec(steel_data_0e(0x20, 0x27, r, g, b));
		write_led_rec(steel_data_0e(0x28, 0x3e, r, g, b));
		return 0;
	}

	int persist()
	{
		steel_data_09 d;
		return write(&d, sizeof(d));
	}

	int c_ff()
	{
		steel_data_0c d;
		return write(&d, sizeof(d));
	}

private:
	int write_led_rec(steel_data_0e &&data)
	{
		return control_msg_write(0x21, 0x09, 0x300, 0,
			&data, static_cast<int>(sizeof(steel_data_0e)), 1000);
	}

};

#endif /* PSTEELSERIES_H_ */
