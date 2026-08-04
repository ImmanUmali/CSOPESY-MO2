#pragma once
#include <string>
#include <vector>
#include <memory>

enum class OpCode { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR, READ, WRITE };

struct Instruction {
    OpCode op;
    std::vector<std::string> args;
    std::vector<Instruction> childInstructions;
    uint32_t repeatCount = 0;
};