#pragma once
#include "IMemoryAllocator.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

class PagedMemoryManager : public IMemoryAllocator {
private:
    size_t m_frameSize;
    size_t m_numFrames;

    // Frame Table: true = allocated, false = free
    std::vector<bool> m_frameTable;

    std::unordered_map<void*, std::vector<size_t>> m_pageMap;

    // Counter to generate simulated virtual addresses
    size_t m_virtualAddressCounter;

public:
    PagedMemoryManager(size_t maxMem, size_t frameSize);
    ~PagedMemoryManager() = default;

    // --- IMemoryAllocator Overrides ---
    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    std::string visualizeMemory() override;
};