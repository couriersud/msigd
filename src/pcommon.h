/*
 * pcommon.h
 *
 */

#ifndef PCOMMON_H_
#define PCOMMON_H_

#include <string>
#include <array>
#include <list>

struct device_info
{
	std::string path;
	std::string serial_number;
	unsigned short release_number;
	std::string manufacturer;
	std::string product;
};

using device_info_list = std::list<device_info>;

#endif /* PCOMMON_H_ */
