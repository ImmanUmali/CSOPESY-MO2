#include "Scheduler.h"
#include "Process.h"
#include <chrono>

Scheduler::Scheduler(const std::string& type, int numCpu, unsigned int quantum,
    unsigned int delayPerExec, uint32_t maxMem, uint32_t memPerProc)
    : m_schedulerType(type),
    m_rrScheduler(quantum),
    m_delayPerExec(delayPerExec),
    m_memoryManager(maxMem, memPerProc)
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

    // Track globally for reporting metrics
    m_allTrackedProcesses.push_back(process);

    if (m_schedulerType == "fcfs") {
        m_fcfsScheduler.addProcess(process);
    }
    else if (m_schedulerType == "rr") {
        m_rrScheduler.addProcess(process);
    }
}

void Scheduler::threadLoop() {
    while (m_running) {
        m_cpuCycles++;

        // Automated Process Generation
        if (m_generationEnabled.load()) {
            if (m_cpuCycles.load() % m_batchProcessFreq == 0) {
                int pid = ++m_generatedPidCounter;
                std::string processName = "p" + std::to_string(pid);

                auto batchProc = std::make_shared<Process>(pid, processName, m_minIns, m_maxIns);

                // Keep the state explicit
                batchProc->setState(ProcessState::READY);

                {
                    std::lock_guard<std::mutex> lock(m_schedulerMutex);
                    m_allTrackedProcesses.push_back(batchProc);

                    if (m_schedulerType == "fcfs") {
                        m_fcfsScheduler.addProcess(batchProc);
                    }
                    else if (m_schedulerType == "rr") {
                        m_rrScheduler.addProcess(batchProc);
                    }
                }
            }
        }

        // Variable tracking if work was actually managed this cycle
        bool activeWorkDone = false;

        // Scope lock for core pipeline operations
        {
            std::lock_guard<std::mutex> lock(m_schedulerMutex);

            if (m_schedulerType == "fcfs") {

                for (auto& cpu : m_cpuCores) {
                    if (!cpu.isIdle()) {
                        activeWorkDone = true;
                        cpu.executeCycle();

                        auto process = cpu.getCurrentProcess();
                        if (process && process->isFinished()) {
                            process->setState(ProcessState::FINISHED);
                            m_memoryManager.freeMemory(process->getName());
                            m_allocatedProcesses.erase(process->getName());
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
                        cpu.executeCycle();
                    }
                }

                for (auto& cpu : m_cpuCores) {
                    auto process = cpu.getCurrentProcess();

                    if (!process)
                        continue;

                    // Finished processes have priority
                    if (process->isFinished()) {
                        process->setState(ProcessState::FINISHED);
                        m_memoryManager.freeMemory(process->getName());
                        m_allocatedProcesses.erase(process->getName());
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
                            bool hasMemory = (m_allocatedProcesses.find(proc->getName()) != m_allocatedProcesses.end());
                            
                            if (!hasMemory) {
                                // Attempt First-Fit Allocation
                                if (m_memoryManager.allocateFirstFit(proc->getName())) {
                                    m_allocatedProcesses.insert(proc->getName());
                                    hasMemory = true;
                                }
                            }
                            
                            if (hasMemory) {
                                proc->setState(ProcessState::RUNNING);
                                cpu.assignProcess(proc);
                                cpu.resetCyclesExecuted();
                            } else {
                                // Memory Full: Revert process to tail of Ready Queue
                                m_rrScheduler.addProcess(proc);
                            }
                        }
                    }
                }
            }
        }

        // If the system has 0 delay and nothing is runnable, yield CPU slice 
        // to let dashboard rendering commands print cleanly without thread choking
        if (m_delayPerExec == 0) {
            if (!activeWorkDone) {
                std::this_thread::yield();
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delayPerExec));
        }
    }
}