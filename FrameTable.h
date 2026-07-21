#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

class FrameTable {
private:
    std::vector<bool> m_frames;
    size_t m_freeFrameCount;

public:
    explicit FrameTable(size_t totalFrames);
    ~FrameTable() = default;

    // Returns the index of a free frame, or SIZE_MAX if full
    size_t allocateFreeFrame();

    // Frees a specific frame index
    void freeFrame(size_t frameIndex);

    // Getters
    size_t getFreeFrameCount() const;
    size_t getTotalFrames() const;
    bool isFrameFree(size_t frameIndex) const;
};