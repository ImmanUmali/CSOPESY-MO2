#include "MemoryManager.h"
#include <algorithm>
#include <sstream>

MemoryManager::MemoryManager(uint32_t maxOverallMem, uint32_t memPerProc)
    : m_maxOverallMem(maxOverallMem), m_memPerProc(memPerProc) {

    // Initialize IMemoryAllocator protected variables
    this->maximumSize = maxOverallMem;
    this->currentAllocatedSize = 0;
    this->memoryAllocatorType = FLAT_MEMORY_ALLOCATOR;

    PartitionBlock initialBlock;
    initialBlock.start = 0;
    initialBlock.size = m_maxOverallMem;
    initialBlock.isAllocated = false;
    initialBlock.assignedProcessName = "";

    m_blocks.push_back(initialBlock);
}

bool MemoryManager::allocateFirstFit(const std::string& processName) {
    for (size_t i = 0; i < m_blocks.size(); ++i) {
        if (!m_blocks[i].isAllocated && m_blocks[i].size >= m_memPerProc) {

            if (m_blocks[i].size == m_memPerProc) {
                m_blocks[i].isAllocated = true;
                m_blocks[i].assignedProcessName = processName;
            }
            else {
                uint32_t originalSize = m_blocks[i].size;
                uint32_t originalStart = m_blocks[i].start;

                m_blocks[i].size = m_memPerProc;
                m_blocks[i].isAllocated = true;
                m_blocks[i].assignedProcessName = processName;

                PartitionBlock leftoverBlock;
                leftoverBlock.start = originalStart + m_memPerProc;
                leftoverBlock.size = originalSize - m_memPerProc;
                leftoverBlock.isAllocated = false;
                leftoverBlock.assignedProcessName = "";

                m_blocks.insert(m_blocks.begin() + i + 1, leftoverBlock);
            }

            // Track allocated size for the interface
            this->currentAllocatedSize += m_memPerProc;
            return true;
        }
    }
    return false;
}

void MemoryManager::freeMemory(const std::string& processName) {
    for (auto& block : m_blocks) {
        if (block.isAllocated && block.assignedProcessName == processName) {
            block.isAllocated = false;
            block.assignedProcessName = "";

            // Track deallocated size for the interface
            this->currentAllocatedSize -= m_memPerProc;
            break;
        }
    }

    for (size_t i = 0; i < m_blocks.size() - 1; ) {
        if (!m_blocks[i].isAllocated && !m_blocks[i + 1].isAllocated) {
            m_blocks[i].size += m_blocks[i + 1].size;
            m_blocks.erase(m_blocks.begin() + i + 1);
        }
        else {
            ++i;
        }
    }
}

size_t MemoryManager::getNumProcessesInMemory() const {
    size_t count = 0;
    for (const auto& block : m_blocks) {
        if (block.isAllocated) count++;
    }
    return count;
}

uint32_t MemoryManager::calculateExternalFragmentation() const {
    uint32_t fragSum = 0;
    for (const auto& block : m_blocks) {
        if (!block.isAllocated && block.size < m_memPerProc) {
            fragSum += static_cast<uint32_t>(block.size);
        }
    }
    return fragSum;
}

// --- IMemoryAllocator Implementations ---

void* MemoryManager::allocate(size_t size) {
    static int allocCounter = 0;
    std::string tempName = "DynamicAlloc_" + std::to_string(allocCounter++);

    if (allocateFirstFit(tempName)) {
        for (const auto& block : m_blocks) {
            if (block.isAllocated && block.assignedProcessName == tempName) {
                return (void*)(uintptr_t)block.start;
            }
        }
    }
    return nullptr;
}

void MemoryManager::deallocate(void* ptr) {
    if (!ptr) return;

    uint32_t targetAddress = (uint32_t)(uintptr_t)ptr;
    std::string targetProcess = "";

    for (const auto& block : m_blocks) {
        if (block.isAllocated && block.start == targetAddress) {
            targetProcess = block.assignedProcessName;
            break;
        }
    }

    if (!targetProcess.empty()) {
        freeMemory(targetProcess);
    }
}

std::string MemoryManager::visualizeMemory() {
    std::stringstream ss;
    ss << "--- Memory Visualization (Flat First-Fit) ---\n";
    ss << "Total Memory: " << maximumSize << " | Allocated: " << currentAllocatedSize << "\n";

    for (const auto& block : m_blocks) {
        ss << "[Start: " << block.start << " | Size: " << block.size << "] - ";
        if (block.isAllocated) {
            ss << "Allocated to: " << block.assignedProcessName;
        }
        else {
            ss << "FREE";
        }
        ss << "\n";
    }
    return ss.str();
}