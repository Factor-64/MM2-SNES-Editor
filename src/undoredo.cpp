#include "undoredo.h"
#include <print>

void revertDelta(std::vector<uint8_t>& rom, const MemoryDelta& d)
{
	std::copy(d.oldData.begin(), d.oldData.end(), rom.begin() + d.address);
}

void applyDelta(std::vector<uint8_t>& rom, const MemoryDelta& d)
{
    std::copy(d.newData.begin(), d.newData.end(), rom.begin() + d.address);
}

MemoryDelta makeMemoryDelta(std::vector<uint8_t>& rom, uint32_t address, const std::vector<uint8_t>& newData)
{
    MemoryDelta delta;
    delta.address = address;

    delta.oldData.assign(rom.begin() + address, rom.begin() + address + newData.size());

    delta.newData = newData;

    return delta;
}

void saveDataToROM(UndoStack& undoStack, RedoStack& redoStack, std::vector<uint8_t>& rom, DataChanged& change)
{
    std::println("Saving data {}", change.deltas.size());

    if (change.deltas.empty()) return;

    undoStack.push_back(change);

    if (undoStack.size() > MAX_UNDO)
        undoStack.erase(undoStack.begin());

    for (const auto& d : change.deltas)
        applyDelta(rom, d);

    redoStack.clear();
}

bool undo(UndoStack& undoStack, RedoStack& redoStack, std::vector<uint8_t>& rom)
{
    std::println("Undoing {}", undoStack.size());
    if (undoStack.empty())
        return false;

    DataChanged change = undoStack.back();
    undoStack.pop_back();

    for (const auto& d : change.deltas)
        revertDelta(rom, d);

    redoStack.push_back(change);
    return true;
}

bool redo(UndoStack& undoStack, RedoStack& redoStack, std::vector<uint8_t>& rom)
{
    std::println("Redoing {}", redoStack.size());
    if (redoStack.empty())
        return false;

    DataChanged change = redoStack.back();
    redoStack.pop_back();

    for (const auto& d : change.deltas)
        applyDelta(rom, d);

    undoStack.push_back(change);

    if (undoStack.size() > MAX_UNDO)
        undoStack.erase(undoStack.begin());
    return true;
}
