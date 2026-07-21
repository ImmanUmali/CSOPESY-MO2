#pragma once
//#include "MemoryBlock.h"
#include "IMemoryAllocator.h"
#include <vector>
#include <string>
#include <cstdint>

class MemoryManager : public IMemoryAllocator {
public:
    struct PartitionBlock : public MemoryBlock {
        bool isAllocated;
        std::string assignedProcessName;
    };

private:
    uint32_t m_maxOverallMem;
    uint32_t m_memPerProc;
    
    std::vector<PartitionBlock> m_blocks;

public:
    MemoryManager(uint32_t maxOverallMem, uint32_t memPerProc);
    ~MemoryManager() = default;

    // Core first-fit logic
    bool allocateFirstFit(const std::string& processName);
    void freeMemory(const std::string& processName);

    // Helpers for reporting metrics
    size_t getNumProcessesInMemory() const;
    uint32_t calculateExternalFragmentation() const;
    const std::vector<PartitionBlock>& getBlocks() const { return m_blocks; }

    // IMemoryAllocator Overrides
    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    std::string visualizeMemory() override;
};