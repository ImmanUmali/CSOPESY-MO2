// BackingStore.cpp
#include "BackingStore.h"
#include <fstream>
#include <sstream>
#include <iomanip>

BackingStore::BackingStore() : m_numPagedIn(0), m_numPagedOut(0) {
    std::ofstream file("csopesy-backing-store.txt", std::ios::trunc);
    if (file.is_open()) {
        file << "# <Process Base Address> <VPN (Virtual Page Number)> : <Page Bytes Hex Dump>\n";
        file.close();
    }
}

void BackingStore::writePageToFile(int pid, size_t vpn, const std::vector<uint8_t>& pageData) {
    std::ofstream file("csopesy-backing-store.txt", std::ios::app);
    if (file.is_open()) {
        file << "# " << pid << " " << vpn << " : ";
        for (size_t i = 0; i < pageData.size(); ++i) {
            file << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (int)pageData[i];
            if (i + 1 < pageData.size()) {
                file << " ";
            }
        }
        file << "\n";
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