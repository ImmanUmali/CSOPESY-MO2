#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include "CPUCore.h"
#include "FCFS.h"
#include "RR.h"
#include "IMemoryAllocator.h"

class Scheduler {
private:
    std::string m_schedulerType;
    std::vector<CPUCore> m_cpuCores; 
    FCFSScheduler m_fcfsScheduler;
    RoundRobinScheduler m_rrScheduler;
    unsigned int m_delayPerExec;

    // Threading & Counters
    std::atomic<unsigned int> m_cpuCycles;
    std::atomic<bool> m_running;
    std::thread m_schedulerThread;

    std::atomic<bool> m_generationEnabled{false};
    unsigned int m_batchProcessFreq{1};
    uint32_t m_minIns{0};
    uint32_t m_maxIns{0};
    std::atomic<int> m_generatedPidCounter{0};
    mutable std::mutex m_schedulerMutex; 
    std::vector<std::shared_ptr<Process>> m_allTrackedProcesses;
    std::vector<std::shared_ptr<Process>> m_waitingProcesses;

    IMemoryAllocator* m_allocator;
    size_t m_minMemPerProc;
    size_t m_maxMemPerProc;

    void threadLoop(); 

public:
    Scheduler(const std::string& type, int numCpu, unsigned int quantum, unsigned int delayPerExec, IMemoryAllocator* allocator, size_t minMemPerProc, size_t maxMemPerProc);
    ~Scheduler();

    void start();
    void stop();
    void addProcess(std::shared_ptr<Process> process);

    unsigned int getCpuCycles() const { return m_cpuCycles.load(); }
    const std::vector<CPUCore>& getCores() const { return m_cpuCores; }

    void setGenerationParameters(unsigned int freq, uint32_t minI, uint32_t maxI, int initialPidOffset) {
        m_batchProcessFreq = freq;
        m_minIns = minI;
        m_maxIns = maxI;
        m_generatedPidCounter = initialPidOffset;
    }

    void startGeneration() { m_generationEnabled = true; }
    void stopGeneration() { m_generationEnabled = false; }
    bool isGenerationEnabled() const { return m_generationEnabled.load(); }
    
    // Thread-safe accessors for Dashboard Metrics
    std::vector<std::shared_ptr<Process>> getAllTrackedProcesses() const {
        std::lock_guard<std::mutex> lock(m_schedulerMutex);
        return m_allTrackedProcesses;
    }
};