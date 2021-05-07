/*
 * pcommon.h
 *
 */

#ifndef PCOMMON_H_
#define PCOMMON_H_

#include <string>
#include <array>
#include <vector>

struct device_info
{
	device_info() = default;

	device_info(unsigned vendor, unsigned product)
	: idVendor(vendor)
	, idProduct(product)
	, release_number(0)
	, dev(nullptr)
	{
	}

	unsigned idVendor;
	unsigned idProduct;
	unsigned short release_number;
	std::string path;
	void *dev;
};

using device_info_list = std::vector<device_info>;

#endif /* PCOMMON_H_ */
