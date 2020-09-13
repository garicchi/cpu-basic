#include <memory>

//
// Created by garicchi on 2020/09/12.
//

#ifndef CPU_BASIC_ARCH_HPP
#define CPU_BASIC_ARCH_HPP

using namespace std;

enum class InstructionType {
    MOV,
    ADD,
    SUB,
    AND,
    OR,
    SL,
    SR,
    SRA,
    LDL,
    LDH,
    CMP,
    JE,
    JMP,
    LD,
    ST,
    HLT
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
    uint16_t opcode;
    string mnemonic;
    Instruction() {

    }
    Instruction(InstructionType type, string mnemonic, uint16_t opcode, OperandType operand_type) {
        this->type = type;
        this->operand_type = operand_type;
        this->mnemonic = mnemonic;
        this->opcode = opcode;
    }
};

class Register {
public:
    string name;
    uint16_t code;
    Register(string name, uint16_t code) {
        this->name = name;
        this->code = code;
    }
};


vector<shared_ptr<Instruction>> INSTRUCTIONS = {
        make_shared<Instruction>(InstructionType::MOV, "mov", 0b0000, OperandType::DOUBLE_OPERAND),
        make_shared<Instruction>(InstructionType::ADD, "add", 0b0001, OperandType::DOUBLE_OPERAND),
        make_shared<Instruction>(InstructionType::LDL, "ldl", 0b1000, OperandType::DOUBLE_OPERAND),
        make_shared<Instruction>(InstructionType::LDH, "ldh", 0b1001, OperandType::DOUBLE_OPERAND),
        make_shared<Instruction>(InstructionType::CMP, "cmp", 0b1010, OperandType::DOUBLE_OPERAND),
        make_shared<Instruction>(InstructionType::JE, "je", 0b1011, OperandType::SINGLE_OPERAND),
        make_shared<Instruction>(InstructionType::JMP, "jmp", 0b1100, OperandType::SINGLE_OPERAND),
        make_shared<Instruction>(InstructionType::LD, "ld", 0b1101, OperandType::DOUBLE_OPERAND),
        make_shared<Instruction>(InstructionType::ST, "st", 0b1110, OperandType::DOUBLE_OPERAND),
        make_shared<Instruction>(InstructionType::HLT, "hlt", 0b1111, OperandType::NO_OPERAND)
};

shared_ptr<Instruction> get_inst_by_mnemonic(string mnemonic) {
    int find_index = -1;
    for (int i = 0; i < INSTRUCTIONS.size(); i++) {
        if (INSTRUCTIONS[i]->mnemonic == mnemonic) {
            find_index = i;
            break;
        }
    }
    if (find_index >= 0) {
        return INSTRUCTIONS[find_index];
    } else {
        return nullptr;
    }
}

shared_ptr<Instruction> get_inst_by_opcode(uint16_t opcode) {
    int find_index = -1;
    for (int i = 0; i < INSTRUCTIONS.size(); i++) {
        if (INSTRUCTIONS[i]->opcode == opcode) {
            find_index = i;
            break;
        }
    }
    if (find_index >= 0) {
        return INSTRUCTIONS[find_index];
    } else {
        return nullptr;
    }
}

vector<shared_ptr<Register>> REGISTERS = {
        make_shared<Register>("r0", 0b000),
        make_shared<Register>("r1", 0b001),
        make_shared<Register>("r2", 0b010),
        make_shared<Register>("r3", 0b011)
};

shared_ptr<Register> get_register_by_name(string name) {
    int find_index = -1;
    for (int i = 0; i < REGISTERS.size(); i++) {
        if (REGISTERS[i]->name == name) {
            find_index = i;
            break;
        }
    }
    if (find_index >= 0) {
        return REGISTERS[find_index];
    } else {
        return nullptr;
    }
}

class Program {
public:
    shared_ptr<Instruction> inst;
    uint16_t first_operand;
    uint16_t second_operand;
    Program() {

    }
    Program(shared_ptr<Instruction> inst, uint16_t first_operand, uint16_t second_operand) {
        this->inst = inst;
        this->first_operand = first_operand;
        this->second_operand = second_operand;
    }

    static uint16_t assemble(Program p) {
        uint16_t code = 0;

        code |= p.inst->opcode << 11;
        if (p.inst->operand_type == OperandType::SINGLE_OPERAND) {
            code |= p.first_operand;
        } else if (p.inst->operand_type == OperandType::DOUBLE_OPERAND) {
            code |= p.first_operand << 8;
            code |= p.second_operand;
        }
        return code;
    }

    static shared_ptr<Program> deassemble(uint16_t code) {
        uint16_t mask_opcode = 0b0111100000000000;
        uint16_t mask_first_operand = 0b0000011100000000;
        uint16_t mask_second_operand = 0b0000000011111111;

        uint16_t opcode = (code & mask_opcode) >> 11;
        shared_ptr<Instruction> inst = nullptr;
        for(auto i: INSTRUCTIONS) {
            if (i->opcode == opcode) {
                inst = i;
                break;
            }
        }
        if (inst == nullptr) {
            cerr << "invalid opcode " << opcode << endl;
            exit(1);
        }
        uint16_t first_operand = (code & mask_first_operand) >> 8;
        uint16_t second_operand = (code & mask_second_operand);
        return make_shared<Program>(inst, first_operand, second_operand);
    }

};



#endif //CPU_BASIC_ARCH_HPP
