#pragma once
#include <array>
#include <vector>
#include "../undoredo.h"

struct Object {
	uint8_t screen = 255;
	uint8_t x = 255;
	uint8_t y = 255;
	uint8_t type = 255;
};

using Objects = std::vector<Object>;

Objects loadObjectData(const std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr, uint32_t typeAddr, int count);
DataChanged saveObjectData(std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr, uint32_t typeAddr, const Object& obj);
DataChanged saveAllObjectData(std::vector<uint8_t>& rom, uint32_t screenAddr, uint32_t xAddr, uint32_t yAddr, uint32_t typeAddr, const Objects& objs);
