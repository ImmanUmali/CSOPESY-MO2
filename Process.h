#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "Instruction.h"
#include "IMemoryAllocator.h"

enum class ProcessState { READY, RUNNING, WAITING, FINISHED };

class Process {
private:
    int m_pid;
    std::string m_name;
    ProcessState m_state;
    size_t m_commandCounter;
    size_t m_linesOfCode;
    std::vector<Instruction> m_instructions;
    std::vector<std::string> m_logs;
    std::string m_timestamp;
    std::unordered_map<std::string, int> m_symbolTable;
    unsigned int m_remainingSleepTicks = 0;
    void evaluateInstruction(const Instruction& ins, int coreId);
    void* m_memoryPtr;
    IMemoryAllocator* m_allocator;

public:
    Process(int pid, const std::string& name, uint32_t minIns, uint32_t maxIns, IMemoryAllocator* allocator, size_t memRequired);
    ~Process();
    int getPid() const { return m_pid; }
    std::string getName() const { return m_name; }
    ProcessState getState() const { return m_state; }
    size_t getCommandCounter() const { return m_commandCounter; }
    size_t getLinesOfCode() const { return m_linesOfCode; }
    const std::vector<std::string>& getLogs() const { return m_logs; } 
    std::string getTimestamp() const { return m_timestamp; }

    void addLog(const std::string& message);
    void executeNextLine(int coreId);
    bool isFinished() const { return m_commandCounter >= m_linesOfCode; }
    void setState(ProcessState state);

    void decrementSleep() { if (m_remainingSleepTicks > 0) m_remainingSleepTicks--; }
    unsigned int getRemainingSleep() const { return m_remainingSleepTicks; }
    void setSleepTicks(unsigned int ticks) { m_remainingSleepTicks = ticks; }

    void* getMemoryPtr() const { return m_memoryPtr; }
};
