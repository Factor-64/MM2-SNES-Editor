#include "objects.h"

Objects loadObjectData(const std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr, uint32_t typeAddr, int count)
{
	Objects out;
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

DataChanged saveAllObjectData(std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr,	uint32_t typeAddr, const Objects& objs)
{
	DataChanged data;
	for (size_t i = 0; i < objs.size(); ++i) 
	{
		const auto& o = objs[i];
		DataChanged d = saveObjectData(rom, screenAddr + i, xAddr + i, yAddr + i, typeAddr + i, o);
		data.deltas.insert(data.deltas.begin(), d.deltas.begin(), d.deltas.end());
	}
	return data;
}

DataChanged saveObjectData(std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr, uint32_t typeAddr, const Object& obj)
{
	DataChanged data;
	MemoryDelta mem;
	mem.address = screenAddr;
	mem.newData.push_back(obj.screen);
	mem.oldData.push_back(rom[screenAddr]);
	data.deltas.push_back(mem);

	mem.address = xAddr;
	mem.newData[0] = (obj.x);
	mem.oldData[0] = (obj.y);
	data.deltas.push_back(mem);

	mem.address = yAddr;
	mem.newData[0] = (obj.y);
	mem.oldData[0] = (rom[yAddr]);
	data.deltas.push_back(mem);

	mem.address = typeAddr;
	mem.newData[0] = (obj.type);
	mem.oldData[0] = (rom[typeAddr]);
	data.deltas.push_back(mem);
	return data;
}
