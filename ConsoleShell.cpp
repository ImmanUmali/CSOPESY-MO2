#include "ConsoleShell.h"
#include "CoreCommands.h"
#include "MultiplexerCommands.h"
#include <iostream>
#include <sstream>

ConsoleShell::ConsoleShell()
    : m_initialized(false),
    m_exitFlag(false),
    m_currentView(TerminalView::MAIN_MENU),
    m_attachedProcessName(""),
    m_pidCounter(0),
    m_scheduler(nullptr) {
    setupCommands();
}

void ConsoleShell::printMainMenu() const {
    std::cout << "  /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$  /$$$$$$$$  /$$$$$$  /$$     /$$\n";
    std::cout << " /$$__  $$ /$$__  $$ /$$__  $$| $$__  $$| $$_____/ /$$__  $$|  $$   /$$/\n";
    std::cout << "| $$  \\__/| $$  \\__/| $$  \\ $$| $$  \\ $$| $$      | $$  \\__/ \\  $$ /$$/ \n";
    std::cout << "| $$      |  $$$$$$ | $$  | $$| $$$$$$$/| $$$$$   |  $$$$$$   \\  $$$$/  \n";
    std::cout << "| $$       \\____  $$| $$  | $$| $$____/ | $$__/    \\____  $$   \\  $$/   \n";
    std::cout << "| $$    $$ /$$  \\ $$| $$  | $$| $$      | $$       /$$  \\ $$    | $$    \n";
    std::cout << "|  $$$$$$/|  $$$$$$/|  $$$$$$/| $$      | $$$$$$$$|  $$$$$$/    | $$    \n";
    std::cout << " \\______/  \\______/  \\______/ |__/      |________/ \\______/     |__/    \n";
                                                                        
                                                                        
                                                                        
    std::cout << "Welcome to CSOPESY Emulator!\n";
    std::cout << "Developers:\n     Lazaro, Heisel Janine C. \n     Tria, Chynna Mae Z. \n     Umali, Immanuel Z. \n";
    std::cout << "Last updated: 06-27-2026\n";
    std::cout << "___________________________________________\n\n";
}

void ConsoleShell::registerCommand(CommandPtr command) {
    if (command) {
        m_commandRegistry[command->getName()] = std::move(command);
    }
}

void ConsoleShell::setupCommands() {
    registerCommand(std::make_unique<ExitCommand>());
    registerCommand(std::make_unique<InitializeCommand>());
    registerCommand(std::make_unique<ScreenCommand>());
    registerCommand(std::make_unique<ProcessSmiCommand>());
    registerCommand(std::make_unique<SchedulerStartCommand>());
    registerCommand(std::make_unique<SchedulerStopCommand>());
    registerCommand(std::make_unique<ReportUtilCommand>());
    registerCommand(std::make_unique<VmStatCommand>());
}

Process* ConsoleShell::findProcess(const std::string& name) {
    // 1. Check local manual process tracker first
    for (auto& proc : m_processList) {
        if (proc->getName() == name) {
            return proc.get();
        }
    }

    if (m_scheduler) {
        auto tracked = m_scheduler->getAllTrackedProcesses();
        for (auto& proc : tracked) {
            if (proc->getName() == name) {
                return proc.get();
            }
        }
    }
    
    return nullptr;
}

std::vector<std::string> ConsoleShell::tokenizeInput(const std::string& rawInput) {
    std::vector<std::string> tokens;
    std::stringstream ss(rawInput);
    std::string token;

    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void ConsoleShell::setConfig(const SystemConfig& config) {
    m_systemConfig = config;
}

SystemConfig ConsoleShell::getConfig() const {
    return m_systemConfig;
}

void ConsoleShell::setAttachedProcess(const std::string& processName) {
    m_attachedProcessName = processName;
}

std::string ConsoleShell::getAttachedProcess() const {
    return m_attachedProcessName;
}

void ConsoleShell::run() {
    printMainMenu();

    std::string rawInputLine;

    while (!shouldExit()) {
        // Adjust the console interface prompt dynamically based on view state
        if (m_currentView == TerminalView::MAIN_MENU) {
            std::cout << "root:\\> ";
        } else {
            std::cout << "root:\\" << m_attachedProcessName << "\\> ";
        }
        
        if (!std::getline(std::cin, rawInputLine)) {
            break;
        }

        std::vector<std::string> tokens = tokenizeInput(rawInputLine);
        if (tokens.empty()) continue;

        std::string commandToken = tokens[0];
        std::vector<std::string> commandArgs(tokens.begin() + 1, tokens.end());

        // View Context Rules Engine Intervention
        if (m_currentView == TerminalView::SCREEN_MULTIPLEXER) {
            if (commandToken == "exit") {

                ClearTerminal();
                printMainMenu();

                std::cout << "Welcome back to the main menu.\n" << std::endl;
                m_currentView = TerminalView::MAIN_MENU;
                m_attachedProcessName = "";
                continue;
            }
            if (commandToken != "process-smi") {
                std::cout << "Error: Unrecognized command. Sub-screens only support 'process-smi' and 'exit'.\n" << std::endl;
                continue;
            }
        }

        auto it = m_commandRegistry.find(commandToken);
        if (it != m_commandRegistry.end()) {
            ICommand* commandToExecute = it->second.get();

            if (!isInitialized() && !commandToExecute->isBypassingInitialization()) {
                std::cout << "Error: You must run the \"initialize\" command before performing this action.\n" << std::endl;
            } else {
                commandToExecute->execute(*this, commandArgs);
            }
        } else {
            std::cout << "Error: Command \"" << commandToken << "\" not found.\n" << std::endl;
        }
    }
}