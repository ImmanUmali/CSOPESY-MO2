#pragma once
#include <string>
#include <cstdint>

struct MemoryBlock {
    uint32_t startAddress;
    uint32_t size;
    bool isAllocated;
    std::string assignedProcessName;
};