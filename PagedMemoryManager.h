#pragma once
#include "IMemoryAllocator.h"
#include "FrameTable.h"
#include "PageTable.h"
#include "BackingStore.h"
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <mutex>

class PagedMemoryManager : public IMemoryAllocator {
private:
    size_t m_frameSize;
    std::mutex m_memoryMutex;

    FrameTable m_frameTable;

    std::unordered_map<void*, PageTable> m_pageDirectory;
    std::unordered_set<void*> m_deadlockedProcesses;
    // Counter to generate unique simulated virtual addresses
    size_t m_virtualAddressCounter;
    BackingStore m_backingStore;
    std::list<size_t> m_fifoQueue;

    uint64_t m_pagedInCount{0};
    uint64_t m_pagedOutCount{0};

public:
    PagedMemoryManager(size_t maxMem, size_t frameSize);
    ~PagedMemoryManager() = default;

    // IMemoryAllocator Overrides 
    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    std::string visualizeMemory() override;
    bool performMemoryAccess(void* ptr);

    size_t getMaxMemory() const { return maximumSize; }
    size_t getUsedMemory() const { 
        size_t occupiedFrames = m_frameTable.getTotalFrames() - m_frameTable.getFreeFrameCount();
        return occupiedFrames * m_frameSize;
    }
    size_t getFreeMemory() const { return m_frameTable.getFreeFrameCount() * m_frameSize; }
    uint64_t getPagedInCount() const { return m_pagedInCount; }
    uint64_t getPagedOutCount() const { return m_pagedOutCount; }
    bool isProcessDeadlocked(void* ptr) const {
        return m_deadlockedProcesses.count(ptr) > 0;
    }
};