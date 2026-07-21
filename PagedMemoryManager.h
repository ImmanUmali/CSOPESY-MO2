#pragma once
#include "IMemoryAllocator.h"
#include "FrameTable.h"
#include "PageTable.h"
#include <unordered_map>
#include <string>
#include <cstdint>

class PagedMemoryManager : public IMemoryAllocator {
private:
    size_t m_frameSize;

    // The global physical memory manager
    FrameTable m_frameTable;

    // Maps a simulated virtual base address to its corresponding PageTable
    std::unordered_map<void*, PageTable> m_pageDirectory;

    // Counter to generate unique simulated virtual addresses
    size_t m_virtualAddressCounter;

public:
    PagedMemoryManager(size_t maxMem, size_t frameSize);
    ~PagedMemoryManager() = default;

    // --- IMemoryAllocator Overrides ---
    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    std::string visualizeMemory() override;
};