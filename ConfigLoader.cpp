#include "ConfigLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string stripQuotes(const std::string& str) {
    if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.size() - 2);
    }
    return str;
}

bool isPowerOfTwo(long long n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

bool isValidMemRange(long long val) {
    return (val >= 64 && val <= 65536) && isPowerOfTwo(val);
}

bool ConfigLoader::loadAndValidate(const std::string& filename, SystemConfig& outConfig) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "File Error: Could not open configuration file '" << filename << "'.\n";
        return false;
    }

    SystemConfig tempConfig;
    std::string key, valueStr;
    int itemsParsed = 0;

    while (file >> key >> valueStr) {
        if (key == "num-cpu") {
            long long val = std::stoll(valueStr);
            if (val < 1 || val > 128) {
                std::cerr << "Validation Error: num-cpu value " << val << " is outside [1, 128].\n";
                return false;
            }
            tempConfig.numCpu = static_cast<uint32_t>(val);
            itemsParsed++;
        }
        else if (key == "scheduler") {
            std::string sched = stripQuotes(valueStr);
            if (sched != "fcfs" && sched != "rr") {
                std::cerr << "Validation Error: scheduler must be 'fcfs' or 'rr'. Found: " << sched << "\n";
                return false;
            }
            tempConfig.scheduler = sched;
            itemsParsed++;
        }
        else if (key == "quantum-cycles") {
            long long val = std::stoll(valueStr);
            
            if (val < 0) {
                std::cerr << "Validation Error: quantum-cycles cannot be negative.\n";
                return false;
            }
            tempConfig.quantumCycles = static_cast<uint32_t>(val);
            itemsParsed++;
        }
        else if (key == "batch-process-freq") {
            long long val = std::stoll(valueStr);
            if (val < 1) {
                std::cerr << "Validation Error: batch-process-freq must be >= 1.\n";
                return false;
            }
            tempConfig.batchProcessFreq = static_cast<uint32_t>(val);
            itemsParsed++;
        }
        else if (key == "min-ins") {
            long long val = std::stoll(valueStr);
            if (val < 1) {
                std::cerr << "Validation Error: min-ins must be >= 1.\n";
                return false;
            }
            tempConfig.minIns = static_cast<uint32_t>(val);
            itemsParsed++;
        }
        else if (key == "max-ins") {
            long long val = std::stoll(valueStr);
            if (val < 1) {
                std::cerr << "Validation Error: max-ins must be >= 1.\n";
                return false;
            }
            tempConfig.maxIns = static_cast<uint32_t>(val);
            itemsParsed++;
        }
        else if (key == "delay-per-exec") {
            long long val = std::stoll(valueStr);
            if (val < 0) {
                std::cerr << "Validation Error: delay-per-exec cannot be negative.\n";
                return false;
            }
            tempConfig.delayPerExec = static_cast<uint32_t>(val);
            itemsParsed++;
        }


        else if (key == "max-overall-mem") {
            long long val = std::stoll(valueStr);
            if (!isValidMemRange(val)) {
                std::cerr << "Validation Error: max-overall-mem must be a power of 2 between 64 and 65536.\n";
                return false;
            }
            tempConfig.maxOverallMem = static_cast<uint32_t>(val);
            itemsParsed++;
        }
        else if (key == "mem-per-frame") {
            long long val = std::stoll(valueStr);
            if (!isPowerOfTwo(val)) {
                std::cerr << "Validation Error: mem-per-frame must be a power of 2.\n";
                return false;
            }
            tempConfig.memPerFrame = static_cast<uint32_t>(val);
            itemsParsed++;
        }
        else if (key == "min-mem-per-proc") {
            long long val = std::stoll(valueStr);
            if (!isValidMemRange(val)) {
                std::cerr << "Validation Error: min-mem-per-proc must be a power of 2 between 64 and 65536.\n";
                return false;
            }
            tempConfig.minMemPerProc = static_cast<uint32_t>(val);
            itemsParsed++;
        }
        else if (key == "max-mem-per-proc") {
            long long val = std::stoll(valueStr);
            if (!isValidMemRange(val)) {
                std::cerr << "Validation Error: max-mem-per-proc must be a power of 2 between 64 and 65536.\n";
                return false;
            }
            tempConfig.maxMemPerProc = static_cast<uint32_t>(val);
            itemsParsed++;
        }
    }

    if (itemsParsed < 11) {
        std::cerr << "Validation Error: Missing parameters in config.txt. Parse count: " << itemsParsed << "/11\n";
        return false;
    }

    if (tempConfig.minIns > tempConfig.maxIns) {
        std::cerr << "Validation Error: min-ins cannot be greater than max-ins.\n";
        return false;
    }
    if (tempConfig.minMemPerProc > tempConfig.maxMemPerProc) {
        std::cerr << "Validation Error: min-mem-per-proc cannot be greater than max-mem-per-proc.\n";
        return false;
    }
    if (tempConfig.maxMemPerProc > tempConfig.maxOverallMem) {
        std::cerr << "Validation Error: max-mem-per-proc cannot be larger than max-overall-mem.\n";
        return false;
    }
    if (tempConfig.scheduler == "rr" && tempConfig.quantumCycles == 0) {
        std::cerr << "Validation Error: quantum-cycles must be >= 1 when using the 'rr' (Round Robin) scheduler.\n";
        return false;
    }
    outConfig = tempConfig;
    return true;
}