#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

struct PageTableEntry {
    size_t frameIndex;
    bool isValid; // True if in RAM, False if on backing store

    PageTableEntry() : frameIndex(SIZE_MAX), isValid(false) {}
};

class PageTable {
private:
    std::vector<PageTableEntry> m_entries;

public:
    PageTable() = default;
    ~PageTable() = default;

    void initialize(size_t numPages) {
        m_entries.resize(numPages);
    }

    // Map a logical page to a physical frame
    void mapPage(size_t logicalPage, size_t physicalFrame) {
        if (logicalPage < m_entries.size()) {
            m_entries[logicalPage].frameIndex = physicalFrame;
            m_entries[logicalPage].isValid = true;
        }
    }

    // Unmap a logical page
    void unmapPage(size_t logicalPage) {
        if (logicalPage < m_entries.size()) {
            m_entries[logicalPage].frameIndex = SIZE_MAX;
            m_entries[logicalPage].isValid = false;
        }
    }

    // Get the page table entry
    const PageTableEntry& getEntry(size_t logicalPage) const {
        return m_entries[logicalPage];
    }

    size_t getNumPages() const {
        return m_entries.size();
    }
};