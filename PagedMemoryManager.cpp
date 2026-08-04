#include "PagedMemoryManager.h"
#include <sstream>
#include <cstdlib>

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

    
    if (m_pageDirectory.size() >= m_frameTable.getTotalFrames()) {
        return nullptr; // Forces the process into the Waiting Queue
    }

    size_t pagesNeeded = (size + m_frameSize - 1) / m_frameSize;

    // Reserve virtual address space and initialize page table
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

    // Lock memory operations across multi-threaded CPU cores
    std::lock_guard<std::mutex> lock(m_memoryMutex);

    auto it = m_pageDirectory.find(ptr);
    if (it == m_pageDirectory.end()) return false;

    PageTable& pageTable = it->second;
    size_t numPages = pageTable.getNumPages();

    if (numPages == 0) return false;

    // Check only ONE random page per CPU cycle.
    size_t logicalPage = std::rand() % numPages;
    PageTableEntry pte = pageTable.getEntry(logicalPage);

    if (!pte.isValid) {
        size_t physicalFrame = m_frameTable.allocateFreeFrame();

        // FIFO Page Replacement when RAM is full
        if (physicalFrame == SIZE_MAX) {
            if (numPages > m_frameTable.getTotalFrames()) {
                m_deadlockedProcesses.insert(ptr);
                return true; // Cycle consumed, but process is now deadlocked
            }
            physicalFrame = m_fifoQueue.front();
            m_fifoQueue.pop_front();

            void* victimAddress = nullptr;
            size_t victimLogicalPage = 0;
            m_frameTable.getFrameOwner(physicalFrame, victimAddress, victimLogicalPage);

            auto victimIt = m_pageDirectory.find(victimAddress);
            if (victimIt != m_pageDirectory.end()) {
                victimIt->second.unmapPage(victimLogicalPage);
            }

            // Extract victim PID from victim address or pointer
            int victimPid = static_cast<int>(reinterpret_cast<uintptr_t>(victimAddress) & 0xFFFF);
            size_t vpn = victimLogicalPage;

            // Generate page buffer bytes corresponding to frame size
            std::vector<uint8_t> dummyPageData(m_frameSize);
            for (size_t b = 0; b < m_frameSize; ++b) {
                dummyPageData[b] = static_cast<uint8_t>(std::rand() % 256);
            }

            // Write the formatted hex dump to the backing store file
            m_backingStore.writePageToFile(victimPid, vpn, dummyPageData);

            m_pagedOutCount++;
        }

        std::string pageKey = std::to_string(reinterpret_cast<uintptr_t>(ptr)) + "_" + std::to_string(logicalPage);
        std::string loadedData;
        m_backingStore.readPageFromFile(pageKey, loadedData);

        m_frameTable.setFrameOwner(physicalFrame, ptr, logicalPage);
        pageTable.mapPage(logicalPage, physicalFrame);
        m_fifoQueue.push_back(physicalFrame);

        m_pagedInCount++;

        return true; 
    }

    return false; 
}