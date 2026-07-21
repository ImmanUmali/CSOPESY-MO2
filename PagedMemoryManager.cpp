#include "PagedMemoryManager.h"
#include <sstream>

PagedMemoryManager::PagedMemoryManager(size_t maxMem, size_t frameSize) {
    // Initialize protected variables from IMemoryAllocator
    this->maximumSize = maxMem;
    this->currentAllocatedSize = 0;
    this->memoryAllocatorType = PAGING;

    this->m_frameSize = frameSize;

    // Calculate total frames and initialize Frame Table
    this->m_numFrames = maxMem / frameSize;
    this->m_frameTable.resize(this->m_numFrames, false); // All frames initially free

    // Start virtual addresses at a simulated offset
    this->m_virtualAddressCounter = 0x1000;
}

void* PagedMemoryManager::allocate(size_t size) {
    if (size == 0) return nullptr;

    // 1. Calculate required frames (ceiling division)
    size_t framesNeeded = (size + m_frameSize - 1) / m_frameSize;

    // 2. Check if we have enough total free frames (Phase 4 backing store hook goes here later)
    size_t freeFramesAvailable = 0;
    for (bool isAllocated : m_frameTable) {
        if (!isAllocated) freeFramesAvailable++;
    }

    if (freeFramesAvailable < framesNeeded) {
        return nullptr; // Out of physical memory
    }

    // 3. Allocate the frames (non-contiguous is fine!)
    std::vector<size_t> allocatedFrames;
    for (size_t i = 0; i < m_numFrames && allocatedFrames.size() < framesNeeded; ++i) {
        if (!m_frameTable[i]) {
            m_frameTable[i] = true; // Mark frame as allocated
            allocatedFrames.push_back(i);
        }
    }

    // 4. Generate a simulated virtual address pointer
    void* virtualAddress = (void*)(uintptr_t)m_virtualAddressCounter;
    m_virtualAddressCounter += (framesNeeded * m_frameSize); // Increment by allocated virtual space

    // 5. Update interface tracker and store mapping
    this->currentAllocatedSize += (framesNeeded * m_frameSize);
    m_pageMap[virtualAddress] = allocatedFrames;

    return virtualAddress;
}

void PagedMemoryManager::deallocate(void* ptr) {
    if (!ptr) return;

    // 1. Look up the simulated virtual address in our Page Map
    auto it = m_pageMap.find(ptr);
    if (it == m_pageMap.end()) {
        return; // Invalid pointer or already freed
    }

    // 2. Free the associated physical frames
    const std::vector<size_t>& allocatedFrames = it->second;
    for (size_t frameIndex : allocatedFrames) {
        if (frameIndex < m_numFrames) {
            m_frameTable[frameIndex] = false; // Mark frame as free
        }
    }

    // 3. Update interface tracker and remove mapping
    this->currentAllocatedSize -= (allocatedFrames.size() * m_frameSize);
    m_pageMap.erase(it);
}

std::string PagedMemoryManager::visualizeMemory() {
    std::stringstream ss;
    ss << "--- Memory Visualization (Paging) ---\n";
    ss << "Total Memory: " << maximumSize << " | Allocated: " << currentAllocatedSize << "\n";
    ss << "Frame Size: " << m_frameSize << " | Total Frames: " << m_numFrames << "\n";

    size_t freeFrames = 0;
    for (bool isAllocated : m_frameTable) {
        if (!isAllocated) freeFrames++;
    }
    ss << "Free Frames: " << freeFrames << " | Used Frames: " << (m_numFrames - freeFrames) << "\n\n";

    ss << "--- Frame Table Status ---\n";
    for (size_t i = 0; i < m_numFrames; ++i) {
        ss << "Frame " << i << ": [" << (m_frameTable[i] ? "USED" : "FREE") << "]  ";
        // Newline every 5 frames for readability
        if ((i + 1) % 5 == 0) ss << "\n";
    }
    ss << "\n";

    return ss.str();
}