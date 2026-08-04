#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

struct FrameData {
    bool isAllocated;
    void* ownerVirtualAddress;
    size_t ownerLogicalPage;

    FrameData() : isAllocated(false), ownerVirtualAddress(nullptr), ownerLogicalPage(0) {}
};

class FrameTable {
private:
    std::vector<FrameData> m_frames;
    size_t m_freeFrameCount;

public:
    explicit FrameTable(size_t totalFrames);
    ~FrameTable() = default;

    // Returns the index of a free frame, or SIZE_MAX if full
    size_t allocateFreeFrame();

    // Frees a specific frame index
    void freeFrame(size_t frameIndex);

    size_t getFreeFrameCount() const;
    size_t getTotalFrames() const;
    bool isFrameFree(size_t frameIndex) const;


    void setFrameOwner(size_t frameIndex, void* virtualAddress, size_t logicalPage);
    void getFrameOwner(size_t frameIndex, void*& outVirtualAddress, size_t& outLogicalPage) const;
};