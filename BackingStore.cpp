#include "BackingStore.h"
#include <sstream>

BackingStore::BackingStore() : m_numPagedIn(0), m_numPagedOut(0) {}

void BackingStore::pageOut() {
    // Tallying num-paged-out: This happens exactly here during eviction.
    m_numPagedOut++;
}

void BackingStore::pageIn() {
    // Tallying num-paged-in: This will be called during Phase 5 (Page Faults).
    m_numPagedIn++;
}

size_t BackingStore::getNumPagedIn() const { return m_numPagedIn; }
size_t BackingStore::getNumPagedOut() const { return m_numPagedOut; }

std::string BackingStore::getStats() const {
    std::stringstream ss;
    ss << "Paged In: " << m_numPagedIn << " | Paged Out: " << m_numPagedOut;
    return ss.str();
}