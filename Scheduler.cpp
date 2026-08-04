#include "Scheduler.h"
#include "Process.h"
#include <chrono>
#include <cmath>
#include <cstdlib>

Scheduler::Scheduler(const std::string& type, int numCpu, unsigned int quantum, unsigned int delayPerExec, IMemoryAllocator* allocator, size_t minMemPerProc, size_t maxMemPerProc)
    : m_schedulerType(type),
    m_rrScheduler(quantum),
    m_delayPerExec(delayPerExec),
    m_cpuCycles(0),
    m_running(false),
    m_allocator(allocator),
    m_minMemPerProc(minMemPerProc),
    m_maxMemPerProc(maxMemPerProc)
{
    for (int i = 0; i < numCpu; ++i) {
        m_cpuCores.emplace_back(i);
    }
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (!m_running) {
        m_running = true;
        m_schedulerThread = std::thread(&Scheduler::threadLoop, this);
    }
}

void Scheduler::stop() {
    if (m_running) {
        m_running = false;
        if (m_schedulerThread.joinable()) {
            m_schedulerThread.join();
        }
    }
}

void Scheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(m_schedulerMutex);

    m_allTrackedProcesses.push_back(process);

    // If no memory was allocated, send to Waiting Queue instead of Ready Queue
    if (process->getMemoryPtr() == nullptr) {
        process->setState(ProcessState::WAITING);
        m_waitingProcesses.push_back(process);
    } 
    else {
        if (m_schedulerType == "fcfs") {
            m_fcfsScheduler.addProcess(process);
        }
        else if (m_schedulerType == "rr") {
            m_rrScheduler.addProcess(process);
        }
    }
}

void Scheduler::threadLoop() {
    while (m_running) {
        m_cpuCycles++;

        if (m_generationEnabled.load()) {
            if (m_cpuCycles.load() % m_batchProcessFreq == 0) {
                int pid = ++m_generatedPidCounter;
                std::string processName = "p" + std::to_string(pid);

                int minPower = static_cast<int>(std::log2(m_minMemPerProc));
                int maxPower = static_cast<int>(std::log2(m_maxMemPerProc));

                // Roll a random power between min and max
                int randomPower = minPower + (std::rand() % (maxPower - minPower + 1));
                size_t randomMemSize = static_cast<size_t>(1) << randomPower; // Shift bit to get the actual byte size

                auto batchProc = std::make_shared<Process>(
                    pid,
                    processName,
                    m_minIns,
                    m_maxIns,
                    m_allocator,
                    randomMemSize
                );

                // Keep the state explicit
                batchProc->setState(ProcessState::READY);

                {
                    std::lock_guard<std::mutex> lock(m_schedulerMutex);
                    m_allTrackedProcesses.push_back(batchProc);

                    if (batchProc->getMemoryPtr() == nullptr) {
                        batchProc->setState(ProcessState::WAITING);
                        m_waitingProcesses.push_back(batchProc);
                    }
                    else {
                        batchProc->setState(ProcessState::READY);
                        if (m_schedulerType == "fcfs") {
                            m_fcfsScheduler.addProcess(batchProc);
                        }
                        else if (m_schedulerType == "rr") {
                            m_rrScheduler.addProcess(batchProc);
                        }
                    }
                }
            }
        }

        // Variable tracking if work was actually managed this cycle
        bool activeWorkDone = false;

        {
            std::lock_guard<std::mutex> lock(m_schedulerMutex);

            if (m_schedulerType == "fcfs") {

                for (auto& cpu : m_cpuCores) {
                    if (!cpu.isIdle()) {
                        activeWorkDone = true;
                        cpu.executeCycle(m_allocator);

                        auto process = cpu.getCurrentProcess();
                        if (process && process->isFinished()) {
                            process->setState(ProcessState::FINISHED);
                            process->reclaimMemory();
                            auto it = m_waitingProcesses.begin();
                            while (it != m_waitingProcesses.end()) {
                                auto waitProc = *it;
                                void* newMem = m_allocator->allocate(waitProc->getMemRequired());

                                if (newMem != nullptr) {
                                    waitProc->setMemoryPtr(newMem);
                                    waitProc->setState(ProcessState::READY);

                                    if (m_schedulerType == "fcfs") {
                                        m_fcfsScheduler.addProcess(waitProc);
                                    }
                                    else if (m_schedulerType == "rr") {
                                        m_rrScheduler.addProcess(waitProc);
                                    }
                                    // Remove from waiting queue as it successfully transitioned
                                    it = m_waitingProcesses.erase(it);
                                }
                                else {
                                    // Still no memory available, check the next one
                                    ++it;
                                }
                            }
                            cpu.assignProcess(nullptr);
                            cpu.resetCyclesExecuted();
                        }
                    }
                }

                for (auto& cpu : m_cpuCores) {
                    if (cpu.isIdle()) {
                        auto proc = m_fcfsScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                        }
                    }
                }
            }

            else if (m_schedulerType == "rr") {

                for (auto& cpu : m_cpuCores) {
                    if (!cpu.isIdle()) {
                        activeWorkDone = true;
                        cpu.executeCycle(m_allocator);
                    }
                }

                for (auto& cpu : m_cpuCores) {
                    auto process = cpu.getCurrentProcess();

                    if (!process)
                        continue;

                    // Finished processes have priority
                    if (process->isFinished()) {
                        process->setState(ProcessState::FINISHED);
                        process->reclaimMemory();
                        auto it = m_waitingProcesses.begin();
                        while (it != m_waitingProcesses.end()) {
                            auto waitProc = *it;
                            void* newMem = m_allocator->allocate(waitProc->getMemRequired());

                            if (newMem != nullptr) {
                                waitProc->setMemoryPtr(newMem);
                                waitProc->setState(ProcessState::READY);

                                if (m_schedulerType == "fcfs") {
                                    m_fcfsScheduler.addProcess(waitProc);
                                }
                                else if (m_schedulerType == "rr") {
                                    m_rrScheduler.addProcess(waitProc);
                                }
                                // Remove from waiting queue as it successfully transitioned
                                it = m_waitingProcesses.erase(it);
                            }
                            else {
                                // Still no memory available, check the next one
                                ++it;
                            }
                        }
                        cpu.assignProcess(nullptr);
                        cpu.resetCyclesExecuted();
                    }
                    // Otherwise, check quantum expiry
                    else if (cpu.getCyclesExecuted() >= m_rrScheduler.getQuantum()) {
                        process->setState(ProcessState::READY);
                        m_rrScheduler.addProcess(process);

                        cpu.assignProcess(nullptr);
                        cpu.resetCyclesExecuted();
                    }
                }

                for (auto& cpu : m_cpuCores) {
                    if (cpu.isIdle()) {
                        auto proc = m_rrScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                            cpu.resetCyclesExecuted();
                        }
                    }
                }
            }
        }


        if (m_delayPerExec == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delayPerExec));
        }
    }
}