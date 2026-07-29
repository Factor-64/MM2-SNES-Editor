#pragma once
#include <cstdint>
uint32_t snesToPc(uint32_t snes, bool hiROM);

uint32_t snesBankAddrToPc(uint32_t bank, uint32_t addr, bool hiROM);