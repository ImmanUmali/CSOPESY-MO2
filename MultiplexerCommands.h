#pragma once
#include "ICommand.h"
#include "ISystemContext.h"
#include "ConsoleShell.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

class ConsoleShell;

void ClearTerminal() {
#if defined(_WIN32) || defined(_WIN64)
    std::system("cls");
#else
    std::system("clear");
#endif
}

// Helper structure to print log
inline void GenerateReportStream(std::ostream& out, ConsoleShell& shell) {
    auto sched = shell.getScheduler();
    if (!sched) return;

    auto cores = sched->getCores();
    auto trackingList = sched->getAllTrackedProcesses();

    size_t totalCores = cores.size();
    size_t coresUsed = 0;
    for (const auto& core : cores) {
        if (!core.isIdle()) coresUsed++;
    }

    double utilization = (totalCores > 0) ? ((double)coresUsed / totalCores) * 100.0 : 0.0;

    out << "---------------------------------------------------------\n";
    out << "CSOPESY Emulator Log\n";
    out << "---------------------------------------------------------\n";
    out << "CPU utilization: " << std::fixed << std::setprecision(0) << utilization << "%\n";
    out << "Cores used: " << coresUsed << "\n";
    out << "Cores available: " << (totalCores - coresUsed) << "\n";
    out << "---------------------------------------------------------\n\n";

    // Display process status if running
    out << "Running processes:\n";
    for (const auto& core : cores) {
        if (core.isIdle())
            continue;

        auto proc = core.getCurrentProcess();
        if (!proc)
            continue;

        out << std::left << std::setw(12) << proc->getName()
            << " (" << proc->getTimestamp() << ")    "
            << "Core # " << core.getId() << "    "
            << proc->getCommandCounter() << " / "
            << proc->getLinesOfCode() << "\n";
    }

    // Display process status if finished
    out << "\nFinished processes:\n";
    for (const auto& proc : trackingList) {
        if (proc->isFinished()) {
            out << std::left << std::setw(12) << proc->getName()
                << " (" << proc->getTimestamp() << ")    "
                << "Finished    " << proc->getLinesOfCode() << " / " << proc->getLinesOfCode() << "\n";
        }
    }
    out << "---------------------------------------------------------\n";
}

inline std::vector<Instruction> ParseCustomInstructions(const std::string& instString) {
    std::vector<Instruction> instrs;
    std::stringstream ss(instString);
    std::string token;

    while (std::getline(ss, token, ';')) {
        // Trim leading/trailing whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        if (token.empty()) continue;

        std::stringstream cmdSS(token);
        std::string opStr;
        cmdSS >> opStr;

        Instruction ins;
        if (opStr == "DECLARE") ins.op = OpCode::DECLARE;
        else if (opStr == "ADD") ins.op = OpCode::ADD;
        else if (opStr == "SUBTRACT") ins.op = OpCode::SUBTRACT;
        else if (opStr == "PRINT") ins.op = OpCode::PRINT;
        else if (opStr == "READ") ins.op = OpCode::READ;
        else if (opStr == "WRITE") ins.op = OpCode::WRITE;
        else if (opStr == "SLEEP") ins.op = OpCode::SLEEP;
        else continue;

        std::string arg;
        while (cmdSS >> arg) {
            // Strip quotes from args if they exist (for PRINT statements)
            if (arg.front() == '"') arg.erase(0, 1);
            if (arg.back() == '"') arg.pop_back();
            ins.args.push_back(arg);
        }
        instrs.push_back(ins);
    }
    return instrs;
}

class ScreenCommand : public ICommand {
public:
    std::string getName() const override { return "screen"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        if (args.empty()) {
            std::cout << "Usage: screen -s <process_name> | screen -r <process_name> | screen -ls\n" << std::endl;
            return;
        }

        std::string flag = args[0];
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);

        // If input is screen -ls
        if (flag == "-ls") {
            std::stringstream ss;
            GenerateReportStream(ss, shell); 
            
            std::string reportText = ss.str();
            shell.setLastSnapshot(reportText); 

            std::cout << reportText; 
            return;
        }

        // 3. Guard for -s and -r which require a process name 
        if (args.size() < 2) {
            std::cout << "Usage: screen " << flag << " <process_name>\n" << std::endl;
            return;
        }

        std::string processName = args[1];

       // If input is screen -s
        // If input is screen -s
        if (flag == "-s") {
            if (args.size() < 3) {
                std::cout << "Usage: screen -s <process_name> <process_memory_size>\n" << std::endl;
                return;
            }

            long long requestedMemory;
            try {
                requestedMemory = std::stoll(args[2]);
            }
            catch (const std::exception&) {
                std::cout << "invalid memory allocation\n" << std::endl;
                return;
            }

            // Must be between 64 and 65536 and a power of 2
            if (requestedMemory < 64 || requestedMemory > 65536 || (requestedMemory & (requestedMemory - 1)) != 0) {
                std::cout << "invalid memory allocation\n" << std::endl;
                return;
            }

            if (shell.findProcess(processName) != nullptr) {
                std::cout << "Error: Process with name '" << processName << "' already exists.\n" << std::endl;
                return;
            }

            SystemConfig cfg = shell.getConfig();
            int newPid = shell.generateNextPid();

            auto newProc = std::make_shared<Process>(
                newPid,
                processName,
                cfg.minIns,
                cfg.maxIns,
                shell.getMemoryManager(),
                static_cast<uint32_t>(requestedMemory) // Uses the validated user input
            );

            shell.addProcess(newProc);

            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);

            ClearTerminal();

            auto sched = shell.getScheduler();
            if (sched) {
                sched->addProcess(newProc);
            }

            std::cout << "Attached to new process screen: " << processName << std::endl;
        }
        
        else if (flag == "-c") {
            if (args.size() < 4) {
                std::cout << "Usage: screen -c <process_name> <process_memory_size> \"<instructions>\"\n" << std::endl;
                return;
            }

            long long requestedMemory;
            try { requestedMemory = std::stoll(args[2]); }
            catch (...) { std::cout << "invalid memory allocation\n" << std::endl; return; }

            if (requestedMemory < 64 || requestedMemory > 65536 || (requestedMemory & (requestedMemory - 1)) != 0) {
                std::cout << "invalid memory allocation\n" << std::endl;
                return;
            }

            if (shell.findProcess(processName) != nullptr) {
                std::cout << "Error: Process with name '" << processName << "' already exists.\n" << std::endl;
                return;
            }

            // Reconstruct the instruction string in case it was split by spaces
            std::string fullInstStr = "";
            for (size_t i = 3; i < args.size(); ++i) {
                fullInstStr += args[i] + (i == args.size() - 1 ? "" : " ");
            }
            // Strip encapsulating quotes
            if (!fullInstStr.empty() && fullInstStr.front() == '"') fullInstStr.erase(0, 1);
            if (!fullInstStr.empty() && fullInstStr.back() == '"') fullInstStr.pop_back();

            std::vector<Instruction> customInstrs = ParseCustomInstructions(fullInstStr);
            if (customInstrs.empty() || customInstrs.size() > 50) {
                std::cout << "invalid command\n" << std::endl;
                return;
            }

            SystemConfig cfg = shell.getConfig();
            int newPid = shell.generateNextPid();
            auto newProc = std::make_shared<Process>(newPid, processName, cfg.minIns, cfg.maxIns, shell.getMemoryManager(), static_cast<uint32_t>(requestedMemory));

            newProc->setCustomInstructions(customInstrs);

            shell.addProcess(newProc);
            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);
            ClearTerminal();

            auto sched = shell.getScheduler();
            if (sched) sched->addProcess(newProc);

            std::cout << "Attached to new process screen (custom instructions): " << processName << std::endl;
        }

        // If input is screen -r
        else if (flag == "-r") {
            Process* existingProc = shell.findProcess(processName);

            if (existingProc == nullptr) {
                std::cout << "Error: Process '" << processName << "' not found.\n" << std::endl;
                return;
            }

            if (existingProc->hasCrashed()) {
                std::cout << "Process " << processName << " shut down due to memory access violation error that occurred at "
                    << existingProc->getCrashTimestamp() << ". " << existingProc->getInvalidAddress() << " invalid.\n" << std::endl;
                return;
            }

            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);
            ClearTerminal();

            std::cout << "Re-attached to process screen: " << processName << std::endl;
        }
        else {
            std::cout << "Invalid flag. Use -s to start, -r to re-attach, or -ls to list dashboard.\n" << std::endl;
        }
    }
};

class ProcessSmiCommand : public ICommand {
public:
    std::string getName() const override { return "process-smi"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        if (context.getCurrentView() != TerminalView::SCREEN_MULTIPLEXER) {
            std::cout << "Error: 'process-smi' can only be executed inside an attached process screen.\n" << std::endl;
            return;
        }

        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        Process* proc = shell.findProcess(shell.getAttachedProcess());

        if (!proc) {
            std::cout << "Error: Attached process references a null context state.\n" << std::endl;
            return;
        }

        std::cout << "Process name: " << proc->getName() << "\n";
        std::cout << "ID: " << proc->getPid() << "\n";

        std::cout << "Logs:\n";
        for (const auto& log : proc->getLogs()) {
            std::cout << log << "\n";
        }

        std::cout << '\n';
        std::cout << "Current Line: " << proc->getCommandCounter() << "\n";
        std::cout << "Total Lines: " << proc->getLinesOfCode() << "\n";


        std::cout << "\n";

        if (proc->isFinished()) {
            std::cout << "Finished!\n\n";
        }
    }
};


class SchedulerStartCommand : public ICommand {
public:
    std::string getName() const override { return "scheduler-start"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        auto sched = shell.getScheduler();
        
        if (!sched) {
            std::cout << "Error: Engine Scheduler context layer uninstantiated.\n" << std::endl;
            return;
        }

        SystemConfig cfg = shell.getConfig();
        sched->setGenerationParameters(cfg.batchProcessFreq, cfg.minIns, cfg.maxIns, shell.generateNextPid() - 1);
        sched->startGeneration();
        
        std::cout << "Automated batch job scheduler engine successfully STARTED.\n" << std::endl;
    }
};

class SchedulerStopCommand : public ICommand {
public:
    std::string getName() const override { return "scheduler-stop"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        auto sched = shell.getScheduler();
        
        if (sched) {
            sched->stopGeneration();
            std::cout << "Automated batch job scheduler engine successfully STOPPED.\n" << std::endl;
        }
    }
};

class ScreenLsCommand : public ICommand {
public:
    std::string getName() const override { return "screen"; } 
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        
        if (!args.empty() && args[0] == "-ls") {
            GenerateReportStream(std::cout, shell);
            return;
        }
        
        std::cout << "Usage: screen -ls  (To display system performance dashboards)\n" << std::endl;
    }
};

class ReportUtilCommand : public ICommand {
public:
    std::string getName() const override { return "report-util"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        
        std::string cachedReport = shell.getLastSnapshot();
        
        if (cachedReport.empty()) {
            std::stringstream ss;
            GenerateReportStream(ss, shell);
            cachedReport = ss.str();
            shell.setLastSnapshot(cachedReport);
        }

        std::ofstream logFile("csopesy-log.txt", std::ios::trunc);
        if (!logFile.is_open()) {
            std::cout << "Error: Unresolved IO access exceptions generating 'csopesy-log.txt'.\n" << std::endl;
            return;
        }

        logFile << cachedReport;
        logFile.close();

        std::cout << "Report generated at csopesy-log.txt!\n" << std::endl;
    }
};