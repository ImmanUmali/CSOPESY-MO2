#include "CPUCore.h"
#include "Process.h"
#include "PagedMemoryManager.h"

CPUCore::CPUCore(int id) : m_id(id), m_cyclesExecuted(0) {
}

bool CPUCore::isIdle() const {
    return m_currentProcess == nullptr;
}

void CPUCore::assignProcess(std::shared_ptr<Process> process) {
    m_currentProcess = process;
}

void CPUCore::executeCycle(IMemoryAllocator* globalAllocator) {
    if (isIdle()) return;

    m_cyclesExecuted++;

    // Phase 5: Simulate memory access before executing the instruction
    void* procMem = m_currentProcess->getMemoryPtr();

    // Attempt to cast the interface to our concrete PagedMemoryManager
    PagedMemoryManager* pagedManager = dynamic_cast<PagedMemoryManager*>(globalAllocator);

    if (pagedManager && procMem) {
        bool pageFault = pagedManager->performMemoryAccess(procMem);

        if (pageFault) {
            // Memory was on disk. The MMU just handled it and swapped it into RAM.
            // This cycle was consumed handling the page fault, so we skip execution.
            return;
        }
    }

    // If no page fault (or using flat memory), execute the next line normally
    m_currentProcess->executeNextLine(m_id);
}

std::shared_ptr<Process> CPUCore::getCurrentProcess() const {
    return m_currentProcess;
}

void CPUCore::resetCyclesExecuted() {
    m_cyclesExecuted = 0;
}

unsigned int CPUCore::getCyclesExecuted() const {
    return m_cyclesExecuted;
}

int CPUCore::getId() const {
    return m_id;
}