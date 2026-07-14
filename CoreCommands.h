#pragma once

#include "ICommand.h"
#include "ISystemContext.h"
#include "ConfigLoader.h"
#include "ConsoleShell.h"
#include "MemoryManager.h"
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
        auto memManager = std::make_unique<MemoryManager>(parsedConfig.maxOverallMem, parsedConfig.memPerProc);
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
        std::cout << " Fixed Memory / Proc : " << parsedConfig.memPerProc << " bytes\n";
        std::cout << "-------------------------------------------\n";
        std::cout << "First-Fit Memory Partitioning initialized.\n" << std::endl;

        auto scheduler = std::make_shared<Scheduler>(
            parsedConfig.scheduler,
            parsedConfig.numCpu,
            parsedConfig.quantumCycles,
            parsedConfig.delayPerExec
        );

        context.setScheduler(scheduler);
        scheduler->start();
        std::cout << "Background scheduler thread spawned successfully!\n" << std::endl;
    }
};