#include "PagedMemoryManager.h"
#include <sstream>

PagedMemoryManager::PagedMemoryManager(size_t maxMem, size_t frameSize)
    : m_frameTable(maxMem / frameSize) {
    this->maximumSize = maxMem;
    this->currentAllocatedSize = 0;
    this->memoryAllocatorType = PAGING;
    this->m_frameSize = frameSize;
    this->m_virtualAddressCounter = 4096;
}

void* PagedMemoryManager::allocate(size_t size) {
    if (size == 0) return nullptr;

    size_t pagesNeeded = (size + m_frameSize - 1) / m_frameSize;

    // Pure Demand Paging: Reserve virtual address space and initialize page table.
    // Do NOT allocate physical frames here.
    PageTable newPageTable;
    newPageTable.initialize(pagesNeeded);

    void* virtualAddress = reinterpret_cast<void*>(m_virtualAddressCounter);
    m_virtualAddressCounter += (pagesNeeded * m_frameSize);

    if (m_virtualAddressCounter > 0x7FFFFFFF) {
        m_virtualAddressCounter = 4096;
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

        // Only free physical frames if the page is currently loaded in RAM
        if (pte.isValid) {
            m_frameTable.freeFrame(pte.frameIndex);
            m_fifoQueue.remove(pte.frameIndex);
        }
    }

    this->currentAllocatedSize -= (numPages * m_frameSize);
    m_pageDirectory.erase(it);
}

std::string PagedMemoryManager::visualizeMemory() {
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

    for (size_t logicalPage = 0; logicalPage < pageTable.getNumPages(); ++logicalPage) {
        PageTableEntry pte = pageTable.getEntry(logicalPage);

        if (!pte.isValid) {
            size_t physicalFrame = m_frameTable.allocateFreeFrame();

            // FIFO Page Replacement when RAM is full
            if (physicalFrame == SIZE_MAX) {
                physicalFrame = m_fifoQueue.front();
                m_fifoQueue.pop_front();

                void* victimAddress = nullptr;
                size_t victimLogicalPage = 0;
                m_frameTable.getFrameOwner(physicalFrame, victimAddress, victimLogicalPage);

                // Evict victim page and persist data to store
                m_pageDirectory[victimAddress].unmapPage(victimLogicalPage);

                std::string victimKey = std::to_string(reinterpret_cast<uintptr_t>(victimAddress)) + "_" + std::to_string(victimLogicalPage);
                m_backingStore.writePageToFile(victimKey, "[PAGE_DATA_DUMP]");

                m_pagedOutCount++;
            }

            // Page in faulting page from store
            std::string pageKey = std::to_string(reinterpret_cast<uintptr_t>(ptr)) + "_" + std::to_string(logicalPage);
            std::string loadedData;
            m_backingStore.readPageFromFile(pageKey, loadedData);

            m_frameTable.setFrameOwner(physicalFrame, ptr, logicalPage);
            pageTable.mapPage(logicalPage, physicalFrame);
            m_fifoQueue.push_back(physicalFrame);

            m_pagedInCount++;

            return true; // Page fault occurred and was resolved
        }
    }

    return false; // All required pages are already in RAM
}