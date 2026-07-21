#pragma once
#include <cstddef>
#include <string>

class BackingStore {
private:
    size_t m_numPagedIn;
    size_t m_numPagedOut;

public:
    BackingStore();
    ~BackingStore() = default;

    // Simulates writing a victim page to disk
    void pageOut();

    // Simulates reading a required page from disk back into RAM
    void pageIn();

    size_t getNumPagedIn() const;
    size_t getNumPagedOut() const;
    std::string getStats() const;
};