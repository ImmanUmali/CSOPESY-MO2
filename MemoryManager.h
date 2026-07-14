#pragma once
#include "MemoryBlock.h"
#include <vector>
#include <string>
#include <cstdint>

class MemoryManager {
private:
    uint32_t m_maxOverallMem;
    uint32_t m_memPerProc;
    std::vector<MemoryBlock> m_blocks;

public:
    MemoryManager(uint32_t maxOverallMem, uint32_t memPerProc);
    ~MemoryManager() = default;

    // Core first-fit logic
    bool allocateFirstFit(const std::string& processName);
    void freeMemory(const std::string& processName);

    // Helpers for reporting metrics in Phase 4
    size_t getNumProcessesInMemory() const;
    uint32_t calculateExternalFragmentation() const;
    const std::vector<MemoryBlock>& getBlocks() const { return m_blocks; }
};