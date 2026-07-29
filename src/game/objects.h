#pragma once
#include <vector>

struct Object {
	uint8_t screen = 0;
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t type = 0;
};

using Objects = std::vector<Object>;

Objects loadObjectData(const std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr, uint32_t typeAddr, int count);
void saveObjectData(std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr, uint32_t typeAddr, const std::vector<Object>& objects);
