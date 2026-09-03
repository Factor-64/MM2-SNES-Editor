#pragma once
#include <vector>

#define MAX_UNDO 512

struct MemoryDelta {
	uint32_t address = 0xFFFFFFFF;
	std::vector<uint8_t> oldData;
	std::vector<uint8_t> newData;
};

struct DataChanged {
	std::vector<MemoryDelta> deltas;
};

using UndoStack = std::vector<DataChanged>;
using RedoStack = std::vector<DataChanged>;

MemoryDelta makeMemoryDelta(std::vector<uint8_t>& rom, uint32_t address, const std::vector<uint8_t>& newData);
void saveDataToROM(UndoStack& undoStack, RedoStack& redoStack, std::vector<uint8_t>& rom, DataChanged& change);
bool undo(UndoStack& undoStack, RedoStack& redoStack, std::vector<uint8_t>& rom);
bool redo(UndoStack& undoStack, RedoStack& redoStack, std::vector<uint8_t>& rom);
