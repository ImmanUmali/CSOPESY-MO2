#include "MemoryManager.h"
#include <algorithm>

MemoryManager::MemoryManager(uint32_t maxOverallMem, uint32_t memPerProc)
    : m_maxOverallMem(maxOverallMem), m_memPerProc(memPerProc) {
    // Initially, the system starts with one giant free block spanning all memory
    m_blocks.push_back({ 0, m_maxOverallMem, false, "" });
}

bool MemoryManager::allocateFirstFit(const std::string& processName) {
    // 1. Search sequentially from the beginning for the first free block that fits
    for (size_t i = 0; i < m_blocks.size(); ++i) {
        if (!m_blocks[i].isAllocated && m_blocks[i].size >= m_memPerProc) {

            // 2. If the block is exactly the right size, just allocate it
            if (m_blocks[i].size == m_memPerProc) {
                m_blocks[i].isAllocated = true;
                m_blocks[i].assignedProcessName = processName;
            }
            // 3. If it's larger, split the block into an allocated part and a free remainder
            else {
                uint32_t originalSize = m_blocks[i].size;
                uint32_t originalStart = m_blocks[i].startAddress;

                // Adjust current block to become the allocated partition
                m_blocks[i].size = m_memPerProc;
                m_blocks[i].isAllocated = true;
                m_blocks[i].assignedProcessName = processName;

                // Insert a new unallocated trailing block representing the leftover gap
                MemoryBlock leftoverBlock;
                leftoverBlock.startAddress = originalStart + m_memPerProc;
                leftoverBlock.size = originalSize - m_memPerProc;
                leftoverBlock.isAllocated = false;
                leftoverBlock.assignedProcessName = "";

                m_blocks.insert(m_blocks.begin() + i + 1, leftoverBlock);
            }
            return true; // Successfully placed in memory!
        }
    }
    return false; // Insufficient continuous space available (Memory Full)
}

void MemoryManager::freeMemory(const std::string& processName) {
    // 1. Locate the process block and mark it as free
    for (auto& block : m_blocks) {
        if (block.isAllocated && block.assignedProcessName == processName) {
            block.isAllocated = false;
            block.assignedProcessName = "";
            break;
        }
    }

    // 2. Coalescing step: Merge adjacent unallocated blocks to eliminate fake fragmentation
    for (size_t i = 0; i < m_blocks.size() - 1; ) {
        if (!m_blocks[i].isAllocated && !m_blocks[i + 1].isAllocated) {
            m_blocks[i].size += m_blocks[i + 1].size; // Absorbs trailing space
            m_blocks.erase(m_blocks.begin() + i + 1); // Delete the redundant block entry
            // Do not increment 'i' so we can check if the newly expanded block can merge further
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
    // External fragmentation is defined as the sum of all free blocks 
    // that are too small to satisfy the allocation requirements (less than 4,096 bytes)
    uint32_t fragSum = 0;
    for (const auto& block : m_blocks) {
        if (!block.isAllocated && block.size < m_memPerProc) {
            fragSum += block.size;
        }
    }
    return fragSum;
}