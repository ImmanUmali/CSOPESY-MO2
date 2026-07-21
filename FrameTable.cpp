#include "FrameTable.h"

FrameTable::FrameTable(size_t totalFrames) {
    m_frames.resize(totalFrames);
    m_freeFrameCount = totalFrames;
}

size_t FrameTable::allocateFreeFrame() {
    if (m_freeFrameCount == 0) return SIZE_MAX;
    for (size_t i = 0; i < m_frames.size(); ++i) {
        if (!m_frames[i].isAllocated) {
            m_frames[i].isAllocated = true;
            m_freeFrameCount--;
            return i;
        }
    }
    return SIZE_MAX;
}

void FrameTable::freeFrame(size_t frameIndex) {
    if (frameIndex < m_frames.size() && m_frames[frameIndex].isAllocated) {
        m_frames[frameIndex].isAllocated = false;
        m_frames[frameIndex].ownerVirtualAddress = nullptr;
        m_freeFrameCount++;
    }
}

size_t FrameTable::getFreeFrameCount() const {
    return m_freeFrameCount;
}

size_t FrameTable::getTotalFrames() const {
    return m_frames.size();
}

bool FrameTable::isFrameFree(size_t frameIndex) const {
    return !m_frames[frameIndex].isAllocated;
}

void FrameTable::setFrameOwner(size_t frameIndex, void* virtualAddress, size_t logicalPage) {
    if (frameIndex < m_frames.size()) {
        m_frames[frameIndex].ownerVirtualAddress = virtualAddress;
        m_frames[frameIndex].ownerLogicalPage = logicalPage;
    }
}

void FrameTable::getFrameOwner(size_t frameIndex, void*& outVirtualAddress, size_t& outLogicalPage) const {
    if (frameIndex < m_frames.size()) {
        outVirtualAddress = m_frames[frameIndex].ownerVirtualAddress;
        outLogicalPage = m_frames[frameIndex].ownerLogicalPage;
    }
}