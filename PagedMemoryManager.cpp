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

    size_t pagesNeeded = (size + m_frameSize - 1) / m_frameSize;

    // We can no longer reject allocation if physical memory is full!
    // Instead, we proceed, relying on the backing store for space.

    PageTable newPageTable;
    newPageTable.initialize(pagesNeeded);

    // Pre-calculate the virtual address so we can assign it as the owner
    // void* virtualAddress = (void*)(uintptr_t)m_virtualAddressCounter;
    void* virtualAddress = reinterpret_cast<void*>(m_virtualAddressCounter);
    m_virtualAddressCounter += (pagesNeeded * m_frameSize);

    if (m_virtualAddressCounter > 0x7FFFFFFF) {
        m_virtualAddressCounter = 4096;
    }

    for (size_t logicalPage = 0; logicalPage < pagesNeeded; ++logicalPage) {
        size_t physicalFrame = m_frameTable.allocateFreeFrame();

        // --- PAGE REPLACEMENT LOGIC (FIFO) ---
        if (physicalFrame == SIZE_MAX) {
            // Memory is full. Evict the oldest frame.
            physicalFrame = m_fifoQueue.front();
            m_fifoQueue.pop_front();

            // Find out who owns this victim frame
            void* victimAddress = nullptr;
            size_t victimLogicalPage = 0;
            m_frameTable.getFrameOwner(physicalFrame, victimAddress, victimLogicalPage);

            // Invalidate the victim's Page Table Entry
            m_pageDirectory[victimAddress].unmapPage(victimLogicalPage);

            // Tally the swap to disk
            m_backingStore.pageOut();
        }

        // Assign the frame to the new logical page
        m_frameTable.setFrameOwner(physicalFrame, virtualAddress, logicalPage);
        newPageTable.mapPage(logicalPage, physicalFrame);

        // Add this frame to the back of the FIFO queue
        m_fifoQueue.push_back(physicalFrame);
    }

    m_pageDirectory[virtualAddress] = newPageTable;
    this->currentAllocatedSize += (pagesNeeded * m_frameSize);

    return virtualAddress;
}

void PagedMemoryManager::deallocate(void* ptr) {
    if (!ptr) return;

    auto it = m_pageDirectory.find(ptr);
    if (it == m_pageDirectory.end()) return;

    PageTable& pageTable = it->second;
    size_t numPages = pageTable.getNumPages();

    for (size_t logicalPage = 0; logicalPage < numPages; ++logicalPage) {
        const PageTableEntry& pte = pageTable.getEntry(logicalPage);

        // Only free physical frames if the page is currently in RAM
        if (pte.isValid) {
            m_frameTable.freeFrame(pte.frameIndex);

            // Remove it from the FIFO replacement queue
            m_fifoQueue.remove(pte.frameIndex);
        }
    }

    this->currentAllocatedSize -= (numPages * m_frameSize);
    m_pageDirectory.erase(it);
}

/* std::string PagedMemoryManager::visualizeMemory() {
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
*/

std::string PagedMemoryManager::visualizeMemory() {
    // 1. Calculate actual Physical RAM currently in use (0 to 128)
    size_t occupiedFrames = m_frameTable.getTotalFrames() - m_frameTable.getFreeFrameCount();
    size_t physicalAllocated = occupiedFrames * m_frameSize;

    std::stringstream ss;
    ss << "--- Memory Visualization (Paging Phase 3) ---\n";
    ss << "Total Physical Memory: " << maximumSize << " | Physical Allocated: " << physicalAllocated << "\n";
    ss << "Total Virtual Allocated: " << currentAllocatedSize << " bytes\n"; 
    ss << "Frame Size: " << m_frameSize << " | Total Frames: " << m_frameTable.getTotalFrames() << "\n";
    ss << "Free Frames: " << m_frameTable.getFreeFrameCount() << "\n\n";

    ss << "--- Frame Table Status ---\n";
    size_t totalFrames = m_frameTable.getTotalFrames();
    for (size_t i = 0; i < totalFrames; ++i) {
        ss << "Frame " << i << ": [" << (m_frameTable.isFrameFree(i) ? "FREE" : "USED") << "]  ";
        if ((i + 1) % 5 == 0) ss << "\n";
    }
    ss << "\n";

    return ss.str();
}

bool PagedMemoryManager::performMemoryAccess(void* ptr) {
    if (!ptr) return false;

    auto it = m_pageDirectory.find(ptr);
    if (it == m_pageDirectory.end()) return false;

    PageTable& pageTable = it->second;
    bool pageFaultOccurred = false;

    // Check all pages belonging to this process
    for (size_t logicalPage = 0; logicalPage < pageTable.getNumPages(); ++logicalPage) {
        PageTableEntry pte = pageTable.getEntry(logicalPage);

        // If the page is invalid (on disk), we have a page fault
        if (!pte.isValid) {
            pageFaultOccurred = true;
            m_backingStore.pageIn(); // Tally num-paged-in

            size_t physicalFrame = m_frameTable.allocateFreeFrame();

            // Apply FIFO Page Replacement if physical memory is full
            if (physicalFrame == SIZE_MAX) {
                physicalFrame = m_fifoQueue.front();
                m_fifoQueue.pop_front();

                void* victimAddress = nullptr;
                size_t victimLogicalPage = 0;
                m_frameTable.getFrameOwner(physicalFrame, victimAddress, victimLogicalPage);

                m_pageDirectory[victimAddress].unmapPage(victimLogicalPage);
                m_backingStore.pageOut(); // Evict victim to disk
            }

            // Bring the faulting page into the frame
            m_frameTable.setFrameOwner(physicalFrame, ptr, logicalPage);
            pageTable.mapPage(logicalPage, physicalFrame);
            m_fifoQueue.push_back(physicalFrame);
        }
    }

    return pageFaultOccurred;
}