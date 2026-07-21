#include "FrameTable.h"

FrameTable::FrameTable(size_t totalFrames) {
    m_frames.resize(totalFrames, false); // false means free
    m_freeFrameCount = totalFrames;
}

size_t FrameTable::allocateFreeFrame() {
    if (m_freeFrameCount == 0) {
        return SIZE_MAX; // Use SIZE_MAX to indicate out of memory
    }

    for (size_t i = 0; i < m_frames.size(); ++i) {
        if (!m_frames[i]) {
            m_frames[i] = true; // Mark as occupied
            m_freeFrameCount--;
            return i;
        }
    }
    return SIZE_MAX;
}

void FrameTable::freeFrame(size_t frameIndex) {
    if (frameIndex < m_frames.size() && m_frames[frameIndex]) {
        m_frames[frameIndex] = false;
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
    if (frameIndex < m_frames.size()) {
        return !m_frames[frameIndex];
    }
    return false;
}