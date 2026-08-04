#include "Process.h"
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

static std::random_device rd;
static std::mt19937 gen(rd());

std::string getCurrentTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;

    struct tm buf;
#if defined(_MSC_VER)
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif

    ss << std::put_time(&buf, "%m/%d/%Y %I:%M:%S %p");
    return ss.str();
}

Instruction generateRandomInstruction(int currentDepth) {
    std::uniform_int_distribution<int> opDist(0, 5);
    OpCode randomOp = static_cast<OpCode>(opDist(gen));

    if (randomOp == OpCode::FOR && currentDepth >= 3) {
        std::uniform_int_distribution<int> linearOpDist(0, 4);
        randomOp = static_cast<OpCode>(linearOpDist(gen));
    }

    Instruction ins;
    ins.op = randomOp;

    // Generate dummy token arguments for simulated expressions
    if (ins.op == OpCode::DECLARE) {
        ins.args = { "x", "0" };
    }
    else if (ins.op == OpCode::ADD || ins.op == OpCode::SUBTRACT) {
        ins.args = { "x", "1" };
    }

    if (ins.op == OpCode::FOR) {
        std::uniform_int_distribution<uint32_t> loopDist(2, 5);
        ins.repeatCount = loopDist(gen);

        std::uniform_int_distribution<size_t> childLinesDist(1, 3);
        size_t linesInBlock = childLinesDist(gen);

        for (size_t i = 0; i < linesInBlock; ++i) {
            ins.childInstructions.push_back(generateRandomInstruction(currentDepth + 1));
        }
    }
    else {
        ins.repeatCount = 0;
    }

    return ins;
}

Process::Process(int pid, const std::string& name, uint32_t minIns, uint32_t maxIns, IMemoryAllocator* allocator, size_t memRequired)
    : m_pid(pid), m_name(name), m_state(ProcessState::READY), m_commandCounter(0), m_allocator(allocator), m_memRequired(memRequired) {

    if (m_allocator) {
        m_memoryPtr = m_allocator->allocate(memRequired);
    }
    else {
        m_memoryPtr = nullptr;
    }

    m_timestamp = getCurrentTimestampString();

    std::uniform_int_distribution<size_t> dist(minIns, maxIns);
    m_linesOfCode = dist(gen);

    for (size_t i = 0; i < m_linesOfCode; ++i) {
        m_instructions.push_back(generateRandomInstruction(1));
    }

    // Default configuration specification baseline log matching exact spacing requirement
    std::stringstream formattedLog;
    formattedLog << "(" << m_timestamp << ") Core:0 \"Hello world from " << m_name << "!\"";
    m_logs.push_back(formattedLog.str());
}

void Process::setState(ProcessState state) {
    m_state = state;
}

void Process::addLog(const std::string& message) {
    m_logs.push_back(message);
    // Keep sliding-window constraints maxed at 5 rows per image specifications
    if (m_logs.size() > 5) {
        m_logs.erase(m_logs.begin());
    }
}

void Process::evaluateInstruction(const Instruction& ins, int coreId) {
    switch (ins.op) {
    case OpCode::PRINT: {
        std::string printOutput = "";

        if (ins.args.empty()) {
            printOutput = "Hello world from " + m_name + "!";
        } else {
            for (size_t i = 0; i < ins.args.size(); ++i) {
                std::string arg = ins.args[i];
                
                // Skip '+' concatenation operators
                if (arg == "+") continue;

                // Strip any remaining backslashes or quotation marks
                arg.erase(std::remove(arg.begin(), arg.end(), '\\'), arg.end());
                arg.erase(std::remove(arg.begin(), arg.end(), '\"'), arg.end());

                if (arg.empty()) continue;

                // Check if argument is in symbol table
                if (m_symbolTable.find(arg) != m_symbolTable.end()) {
                    // If there's already text in the buffer and it doesn't end with a space, add one!
                    if (!printOutput.empty() && printOutput.back() != ' ') {
                        printOutput += " ";
                    }
                    printOutput += std::to_string(m_symbolTable[arg]);
                } else {
                    printOutput += arg;
                }
            }
        }

        addLog(printOutput);
        break;
    }
    case OpCode::DECLARE: {
        std::string varName = ins.args.empty() ? "var" : ins.args[0];

        // Enforce 32 variable limit (64-byte symbol table / 2 bytes per uint16)
        if (m_symbolTable.size() >= 32 && m_symbolTable.find(varName) == m_symbolTable.end()) {
            break; // Limit reached, ignore succeeding declarations
        }

        int initialVal = ins.args.size() < 2 ? 0 : std::stoi(ins.args[1]);
        m_symbolTable[varName] = initialVal;
        break;
    }
    case OpCode::READ: {
        if (ins.args.size() < 2) break;
        std::string varName = ins.args[0];
        std::string hexAddrStr = ins.args[1];
        unsigned long addr = std::stoul(hexAddrStr, nullptr, 16);

        if (addr >= m_memRequired) {
            m_hasCrashed = true;
            m_crashTimestamp = getCurrentTimestampString();
            m_invalidAddress = hexAddrStr;
            m_state = ProcessState::FINISHED;
            return;
        }
        m_symbolTable[varName] = m_simulatedMemory[addr];
        break;
    }
    case OpCode::WRITE: {
        if (ins.args.size() < 2) break;
        std::string hexAddrStr = ins.args[0];
        unsigned long addr = std::stoul(hexAddrStr, nullptr, 16);

        std::string valStr = ins.args[1];
        uint16_t valToWrite = 0;

        if (isdigit(valStr[0])) {
            valToWrite = static_cast<uint16_t>(std::stoi(valStr));
        }
        else {
            valToWrite = m_symbolTable[valStr];
        }

        if (addr >= m_memRequired) {
            m_hasCrashed = true;
            m_crashTimestamp = getCurrentTimestampString();
            m_invalidAddress = hexAddrStr;
            m_state = ProcessState::FINISHED;
            return;
        }
        m_simulatedMemory[addr] = valToWrite;
        break;
    }
    case OpCode::ADD: {
        // Handles MCO2 custom string syntax: ADD dest op1 op2 (e.g., ADD varA varA varB)
        if (ins.args.size() >= 3) {
            std::string dest = ins.args[0];

            // Check if argument is a number or a variable name
            int val1 = (isdigit(ins.args[1][0]) || ins.args[1][0] == '-') ? std::stoi(ins.args[1]) : m_symbolTable[ins.args[1]];
            int val2 = (isdigit(ins.args[2][0]) || ins.args[2][0] == '-') ? std::stoi(ins.args[2]) : m_symbolTable[ins.args[2]];

            m_symbolTable[dest] = val1 + val2;
        }
        // Handles original MO1 batch syntax: ADD var val (e.g., ADD x 1)
        else {
            std::string varName = ins.args.empty() ? "var" : ins.args[0];
            int val = 1;
            if (ins.args.size() >= 2) {
                val = (isdigit(ins.args[1][0]) || ins.args[1][0] == '-') ? std::stoi(ins.args[1]) : m_symbolTable[ins.args[1]];
            }
            m_symbolTable[varName] += val;
        }
        break;
    }
    case OpCode::SUBTRACT: {
        // Handles MCO2 custom string syntax: SUBTRACT dest op1 op2
        if (ins.args.size() >= 3) {
            std::string dest = ins.args[0];
            int val1 = (isdigit(ins.args[1][0]) || ins.args[1][0] == '-') ? std::stoi(ins.args[1]) : m_symbolTable[ins.args[1]];
            int val2 = (isdigit(ins.args[2][0]) || ins.args[2][0] == '-') ? std::stoi(ins.args[2]) : m_symbolTable[ins.args[2]];
            m_symbolTable[dest] = val1 - val2;
        }
        // Handles original MO1 batch syntax: SUBTRACT var val
        else {
            std::string varName = ins.args.empty() ? "var" : ins.args[0];
            int val = 1;
            if (ins.args.size() >= 2) {
                val = (isdigit(ins.args[1][0]) || ins.args[1][0] == '-') ? std::stoi(ins.args[1]) : m_symbolTable[ins.args[1]];
            }
            m_symbolTable[varName] -= val;
        }
        break;
    }
    case OpCode::SLEEP: {
        // Read arguments or default to a randomized sleep tick constraint (e.g., 5 to 15 ticks)
        unsigned int ticksToSleep = ins.args.empty() ? 10 : std::stoul(ins.args[0]);

        m_state = ProcessState::WAITING;
        m_remainingSleepTicks = ticksToSleep;

        
        break;
    }
    case OpCode::FOR: {
        // A loop container doesn't log a message itself; it recursively executes its children
        for (uint32_t r = 0; r < ins.repeatCount; ++r) {
            for (const auto& child : ins.childInstructions) {
                evaluateInstruction(child, coreId);
            }
        }
        break;
    }
    }
}

void Process::executeNextLine(int coreId) {
    if (isFinished() || m_hasCrashed) return;

    m_state = ProcessState::RUNNING;

    // Fetch the active structural instructions node
    const Instruction& activeIns = m_instructions[m_commandCounter];
    evaluateInstruction(activeIns, coreId);

    if (!m_hasCrashed) {
        m_commandCounter++;
    }
}

void Process::reclaimMemory() {
    if (m_allocator && m_memoryPtr) {
        m_allocator->deallocate(m_memoryPtr);
        m_memoryPtr = nullptr; // Ensure we don't double-free
    }
}

Process::~Process() {
    // Fallback in case the process is destroyed before finishing naturally
    reclaimMemory();
}