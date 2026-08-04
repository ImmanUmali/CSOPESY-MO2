#pragma once

#include "ICommand.h"
#include "ISystemContext.h"
#include "ConfigLoader.h"
#include "ConsoleShell.h"
#include "PagedMemoryManager.h"
#include <iostream>

class ExitCommand : public ICommand {
public:
    std::string getName() const override { return "exit"; }
    bool isBypassingInitialization() const override { return true; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        std::cout << "Terminating CSOPESY Emulator console..." << std::endl;
        context.flagExit();
    }
};

class InitializeCommand : public ICommand {
public:
    std::string getName() const override { return "initialize"; }
    bool isBypassingInitialization() const override { return true; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        if (context.isInitialized()) {
            std::cout << "Error: Processor configuration has already been initialized." << std::endl;
            return;
        }

        SystemConfig parsedConfig;
        if (!ConfigLoader::loadAndValidate("config.txt", parsedConfig)) {
            std::cout << "Initialization Failed. System remains locked down.\n" << std::endl;
            return;
        }

        context.setConfig(parsedConfig);
        context.setInitialized(true);

        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        auto memManager = std::make_unique<PagedMemoryManager>(parsedConfig.maxOverallMem, parsedConfig.memPerFrame);
        IMemoryAllocator* allocatorPtr = memManager.get();
        shell.setMemoryManager(std::move(memManager));


        std::cout << "System initialized successfully via 'config.txt'!\n";
        std::cout << "-------------------------------------------\n";
        std::cout << " Cores Available     : " << parsedConfig.numCpu << "\n";
        std::cout << " Selected Scheduler  : " << parsedConfig.scheduler << "\n";
        std::cout << " Quantum Cycles      : " << parsedConfig.quantumCycles << "\n";
        std::cout << " Batch Process Freq  : " << parsedConfig.batchProcessFreq << "\n";
        std::cout << " Instruction Ranges  : [" << parsedConfig.minIns << ", " << parsedConfig.maxIns << "]\n";
        std::cout << " Execution Delay     : " << parsedConfig.delayPerExec << "\n";
        std::cout << " Max Overall Memory  : " << parsedConfig.maxOverallMem << " bytes\n";
        std::cout << " Memory Per Frame    : " << parsedConfig.memPerFrame << " bytes\n";
        std::cout << " Memory Per Proc     : [" << parsedConfig.minMemPerProc << ", " << parsedConfig.maxMemPerProc << "] bytes\n";
        std::cout << "-------------------------------------------\n";
        std::cout << "Paged Memory Partitioning initialized.\n" << std::endl;

        auto scheduler = std::make_shared<Scheduler>(
            parsedConfig.scheduler,
            parsedConfig.numCpu,
            parsedConfig.quantumCycles,
            parsedConfig.delayPerExec,
            allocatorPtr,
            parsedConfig.minMemPerProc,
            parsedConfig.maxMemPerProc
        );

        context.setScheduler(scheduler);
        scheduler->start();
        std::cout << "Background scheduler thread spawned successfully!\n" << std::endl;
    }
};

class VmStatCommand : public ICommand {
public:
    std::string getName() const override { return "vmstat"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        auto memMgr = dynamic_cast<PagedMemoryManager*>(shell.getMemoryManager());
        auto sched = shell.getScheduler();

        if (!memMgr || !sched) {
            std::cout << "Error: Memory Manager or Scheduler not fully initialized.\n\n";
            return;
        }

        size_t totalMem = memMgr->getMaxMemory();
        size_t usedMem = memMgr->getUsedMemory();
        size_t freeMem = memMgr->getFreeMemory();

        // Calculate CPU core ticks across all cores
        uint64_t totalTicks = sched->getCpuCycles();
        size_t cores = sched->getCores().size();

        size_t activeCores = 0;
        for (const auto& core : sched->getCores()) {
            if (!core.isIdle()) activeCores++;
        }

        uint64_t activeTicks = totalTicks * activeCores;
        uint64_t idleTicks = (totalTicks * cores) - activeTicks;

        std::cout << totalMem << " K total memory\n";
        std::cout << usedMem << " K used memory\n";
        std::cout << freeMem << " K free memory\n";
        std::cout << idleTicks << " idle cpu ticks\n";
        std::cout << activeTicks << " active cpu ticks\n";
        std::cout << (totalTicks * cores) << " total cpu ticks\n";
        std::cout << memMgr->getPagedInCount() << " pages paged in\n";
        std::cout << memMgr->getPagedOutCount() << " pages paged out\n\n";
    }
};