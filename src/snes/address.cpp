#include "address.h"
uint32_t snesToPc(uint32_t snes, bool hiROM)
{
	uint32_t bank = (snes >> 16) & 0xFF;
	uint32_t addr = snes & 0xFFFF;
	if (hiROM)
		return (bank << 16) | addr;
	else
		return (bank * 0x8000) + (addr & 0x7FFF);
}

uint32_t snesBankAddrToPc(uint32_t bank, uint32_t addr, bool hiROM)
{
	if (hiROM)
		return (bank << 16) | addr;
	else
		return (bank * 0x8000) + (addr & 0x7FFF);
}