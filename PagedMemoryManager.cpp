#include "PagedMemoryManager.h"
#include <sstream>

PagedMemoryManager::PagedMemoryManager(size_t maxMem, size_t frameSize)
    : m_frameTable(maxMem / frameSize) { // Initialize FrameTable with total frames

    // Initialize protected variables from IMemoryAllocator
    this->maximumSize = maxMem;
    this->currentAllocatedSize = 0;
    this->memoryAllocatorType = PAGING;

    this->m_frameSize = frameSize;

    // Start virtual addresses at a simulated offset (e.g., 0x1000)
    this->m_virtualAddressCounter = 4096;
}

void* PagedMemoryManager::allocate(size_t size) {
    if (size == 0) return nullptr;

    // 1. Calculate required pages (ceiling division)
    size_t pagesNeeded = (size + m_frameSize - 1) / m_frameSize;

    // 2. Check if enough physical frames are available
    if (m_frameTable.getFreeFrameCount() < pagesNeeded) {
        return nullptr; // Out of physical memory (Phase 4 will handle swapping here)
    }

    // 3. Create and initialize a PageTable for this allocation
    PageTable newPageTable;
    newPageTable.initialize(pagesNeeded);

    // 4. Allocate frames and map them in the PageTable
    for (size_t logicalPage = 0; logicalPage < pagesNeeded; ++logicalPage) {
        size_t physicalFrame = m_frameTable.allocateFreeFrame();
        newPageTable.mapPage(logicalPage, physicalFrame);
    }

    // 5. Generate a simulated virtual address pointer
    void* virtualAddress = (void*)(uintptr_t)m_virtualAddressCounter;
    m_virtualAddressCounter += (pagesNeeded * m_frameSize); // Increment by virtual space used

    // 6. Store the PageTable and update metrics
    m_pageDirectory[virtualAddress] = newPageTable;
    this->currentAllocatedSize += (pagesNeeded * m_frameSize);

    return virtualAddress;
}

void PagedMemoryManager::deallocate(void* ptr) {
    if (!ptr) return;

    // 1. Locate the PageTable associated with this virtual address
    auto it = m_pageDirectory.find(ptr);
    if (it == m_pageDirectory.end()) {
        return; // Invalid pointer or already freed
    }

    PageTable& pageTable = it->second;
    size_t numPages = pageTable.getNumPages();

    // 2. Iterate through the PageTable and free the physical frames
    for (size_t logicalPage = 0; logicalPage < numPages; ++logicalPage) {
        const PageTableEntry& pte = pageTable.getEntry(logicalPage);
        if (pte.isValid) {
            m_frameTable.freeFrame(pte.frameIndex);
        }
    }

    // 3. Update metrics and remove the PageTable
    this->currentAllocatedSize -= (numPages * m_frameSize);
    m_pageDirectory.erase(it);
}

std::string PagedMemoryManager::visualizeMemory() {
    std::stringstream ss;
    ss << "--- Memory Visualization (Paging Phase 3) ---\n";
    ss << "Total Memory: " << maximumSize << " | Allocated: " << currentAllocatedSize << "\n";
    ss << "Frame Size: " << m_frameSize << " | Total Frames: " << m_frameTable.getTotalFrames() << "\n";
    ss << "Free Frames: " << m_frameTable.getFreeFrameCount() << "\n\n";

    ss << "--- Frame Table Status ---\n";
    size_t totalFrames = m_frameTable.getTotalFrames();
    for (size_t i = 0; i < totalFrames; ++i) {
        ss << "Frame " << i << ": [" << (m_frameTable.isFrameFree(i) ? "FREE" : "USED") << "]  ";
        if ((i + 1) % 5 == 0) ss << "\n"; // Newline every 5 frames for readability
    }
    ss << "\n";

    return ss.str();
}