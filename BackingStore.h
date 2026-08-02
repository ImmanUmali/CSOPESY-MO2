#pragma once

#include <string>
#include <fstream>
#include <unordered_map>
#include <cstddef>

class BackingStore {
private:
    std::string m_filename;
    size_t m_numPagedIn;
    size_t m_numPagedOut;
    std::unordered_map<std::string, std::streampos> m_pageLocations;

public:
    BackingStore();
    ~BackingStore() = default;

    void writePageToFile(const std::string& pageKey, const std::string& data);
    bool readPageFromFile(const std::string& pageKey, std::string& outData);

    void pageOut();
    void pageIn();

    size_t getNumPagedIn() const;
    size_t getNumPagedOut() const;
    std::string getStats() const;
};