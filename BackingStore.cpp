// BackingStore.cpp
#include "BackingStore.h"
#include <fstream>

BackingStore::BackingStore() : m_numPagedIn(0), m_numPagedOut(0) {
    // Ensure file exists at startup
    std::ofstream file("csopesy-backing-store.txt", std::ios::app);
}

void BackingStore::writePageToFile(const std::string& pageKey, const std::string& data) {
    std::ofstream file("csopesy-backing-store.txt", std::ios::app);
    if (file.is_open()) {
        file << pageKey << ":" << data << "\n";
        m_numPagedOut++;
    }
}

bool BackingStore::readPageFromFile(const std::string& pageKey, std::string& outData) {
    std::ifstream file("csopesy-backing-store.txt");
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind(pageKey + ":", 0) == 0) {
            outData = line.substr(pageKey.length() + 1);
            m_numPagedIn++;
            return true;
        }
    }
    return false;
}

void BackingStore::pageOut() { m_numPagedOut++; }
void BackingStore::pageIn() { m_numPagedIn++; }
size_t BackingStore::getNumPagedIn() const { return m_numPagedIn; }
size_t BackingStore::getNumPagedOut() const { return m_numPagedOut; }

std::string BackingStore::getStats() const {
    return "Paged In: " + std::to_string(m_numPagedIn) + " | Paged Out: " + std::to_string(m_numPagedOut);
}