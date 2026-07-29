#include "objects.h"

Objects loadObjectData(const std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr, uint32_t typeAddr, int count)
{
	std::vector<Object> out;
	out.resize(count);
	for (int i = 0; i < count; ++i)
	{
		Object o{};
		o.screen = rom[screenAddr + i];
		o.x = rom[xAddr + i];
		o.y = rom[yAddr + i];
		o.type = rom[typeAddr + i];

		out[i] = o;
	}
	return out;
}

void saveObjectData(std::vector<uint8_t>& rom, uint32_t screenAddr,	uint32_t xAddr, uint32_t yAddr,	uint32_t typeAddr, const Objects& objects) 
{
	for (size_t i = 0; i < objects.size(); ++i) 
	{
		const auto& o = objects[i];
		rom[screenAddr + i] = o.screen;
		rom[xAddr + i] = o.x;
		rom[yAddr + i] = o.y;
		rom[typeAddr + i] = o.type;
	}
}

