//
// Created by garicchi on 2020/08/02.
//

#ifndef EMULATOR_INST_HPP
#define EMULATOR_INST_HPP

#include <memory>

using namespace std;

enum class InstructionType {
    MOV = 0b0000,
    ADD = 0b0001,
    SUB = 0b0010,
    AND = 0b0011,
    OR = 0b0100,
    SL = 0b0101,
    SR = 0b0110,
    SRA = 0b0111,
    LDL = 0b1000,
    LDH = 0b1001,
    CMP = 0b1010,
    JE = 0b1011,
    JMP = 0b1100,
    LD = 0b1101,
    ST = 0b1110,
    HLT = 0b1111
};

enum class OperandType {
    SINGLE_OPERAND,
    DOUBLE_OPERAND,
    NO_OPERAND
};

class Instruction {
public:
    InstructionType type;
    OperandType operand_type;
    string mnemonic;
    uint16_t first_operand;
    uint16_t second_operand;
    Instruction(InstructionType type, string mnemonic, OperandType operand_type) {
        this->type = type;
        this->operand_type = operand_type;
        this->mnemonic = mnemonic;
        this->first_operand = NULL;
        this->second_operand = NULL;
    }
};

#endif //EMULATOR_INST_HPP
