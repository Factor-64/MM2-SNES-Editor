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
	mem.newData.clear();
	mem.oldData.clear();

	mem.address = xAddr;
	mem.newData.push_back(obj.x);
	mem.oldData.push_back(obj.y);
	data.deltas.push_back(mem);
	mem.newData.clear();
	mem.oldData.clear();

	mem.address = yAddr;
	mem.newData.push_back(obj.y);
	mem.oldData.push_back(rom[yAddr]);
	data.deltas.push_back(mem);
	mem.newData.clear();
	mem.oldData.clear();

	mem.address = typeAddr;
	mem.newData.push_back(obj.type);
	mem.oldData.push_back(rom[typeAddr]);
	data.deltas.push_back(mem);
	return data;
}
