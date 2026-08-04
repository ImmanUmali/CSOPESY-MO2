#pragma once

#include <string>
#include <fstream>
#include <unordered_map>
#include <cstddef>
#include <vector>
#include <cstdint>

class BackingStore {
private:
    std::string m_filename;
    size_t m_numPagedIn;
    size_t m_numPagedOut;
    std::unordered_map<std::string, std::streampos> m_pageLocations;

public:
    BackingStore();
    ~BackingStore() = default;

    void writePageToFile(int pid, size_t vpn, const std::vector<uint8_t>& pageData);
    bool readPageFromFile(const std::string& pageKey, std::string& outData);

    void pageOut();
    void pageIn();

    size_t getNumPagedIn() const;
    size_t getNumPagedOut() const;
    std::string getStats() const;
};