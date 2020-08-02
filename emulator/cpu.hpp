//
// Created by garicchi on 2020/08/03.
//

#ifndef EMULATOR_CPU_HPP
#define EMULATOR_CPU_HPP

#include <iostream>
#include <vector>
#include <bitset>
#include <map>
#include <memory>
#include <unistd.h>
#include <iomanip>
#include "alu.hpp"
#include "psw.hpp"
#include "memory.hpp"
#include "inst.hpp"

using namespace std;

enum class CpuStatus {
    FETCH_INST_0,
    FETCH_INST_1,
    FETCH_OPERAND_0,
    FETCH_OPERAND_1,
    EXEC_INST,
    WRITE_BACK,
};

class Cpu {
private:
    const int PSW_REG_NUMBER = 5;
    const int SP_REG_NUMBER = 6;
    const int PC_REG_NUMBER = 7;

    void register_instructions() {
        vector<shared_ptr<Instruction>> _instructions;
        _instructions.push_back(make_shared<Instruction>(InstructionType::MOV, "mov", OperandType::DOUBLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::ADD,"add", OperandType::DOUBLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::LDL,"ldl", OperandType::DOUBLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::LDH,"ldh", OperandType::DOUBLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::CMP,"cmp", OperandType::DOUBLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::JE,"je", OperandType::SINGLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::JMP,"jmp", OperandType::SINGLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::LD,"ld", OperandType::DOUBLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::ST,"st", OperandType::DOUBLE_OPERAND));
        _instructions.push_back(make_shared<Instruction>(InstructionType::HLT,"hlt", OperandType::NO_OPERAND));
        for (auto const& inst: _instructions) {
            this->instructions[inst->type] = inst;
        }
    }

    shared_ptr<Instruction> decode(uint16_t code) {
        uint16_t mask_opcode = 0b0111100000000000;
        uint16_t mask_first_operand = 0b0000011100000000;
        uint16_t mask_second_operand = 0b0000000011111111;

        uint16_t opcode = (code & mask_opcode) >> 11;
        auto inst = this->instructions[static_cast<InstructionType>(opcode)];
        if (inst->operand_type == OperandType::SINGLE_OPERAND || inst->operand_type == OperandType::DOUBLE_OPERAND) {
            inst->first_operand = (code & mask_first_operand) >> 8;
            inst->second_operand = code & mask_second_operand;
        }

        return inst;
    }

    void print_info() {
        int index = 0;
        cout << "\033[2J" <<"\033[0;0H";  // clear screen
        cout << endl << "----------CLOCK------------" << endl;
        cout << " " << "CLOCK [" << dec << clock_counter << "]" << endl;
        cout << endl << "-------INSTRUCTION---------" << endl;
        if (current_inst != nullptr) {
            cout << " " << "IR [ " << current_inst->mnemonic << " ";
            if (current_inst->operand_type == OperandType::SINGLE_OPERAND ||
                current_inst->operand_type == OperandType::DOUBLE_OPERAND) {
                cout << bitset<3>(current_inst->first_operand) << " ";
                if (current_inst->operand_type == OperandType::DOUBLE_OPERAND) {
                    cout << bitset<8>(current_inst->second_operand) << " ";
                }
            }
            cout << "]" << endl;
        }
        cout << endl << "------STATUS COUNTER--------" << endl;
        for (int i = 0; i < statuses.size(); i++) {
            cout << " " << statuses[static_cast<CpuStatus>(i)] << " ";
            if (current_status == static_cast<CpuStatus>(i)) {
                cout << " <";
            }
            cout << endl;
        }
        cout << endl << "---------REGISTER-----------" << endl;
        for(uint16_t reg: registers) {
            stringstream hex_str;
            cout << " " <<  "R" << index << " [" << bitset<16>(reg) << "] (0x" << hex << reg << ") ";
            if (index == PC_REG_NUMBER) {
                cout << " (PC) ";
            }
            if (index == SP_REG_NUMBER) {
                cout << " (SP) ";
            }
            if (index == PSW_REG_NUMBER) {
                cout << "N = "<< psw->get_negative_flag() << " ";
                cout << "Z = "<< psw->get_zero_flag() << " ";
                cout << " (PSW) ";
            }
            cout << endl;
            index++;
        }
        cout << endl << "----------MEMORY------------" << endl;
        int min_memory_index = (registers[PC_REG_NUMBER] - 2) >= 0 ? registers[PC_REG_NUMBER] - 2 : 0;
        for (int i = min_memory_index; i < (min_memory_index + 5); i++) {
            cout << " " << "[0x" << hex << i << "] [" << bitset<16>(memory->memory[i]) << "] ";
            if (i == registers[PC_REG_NUMBER]) {
                cout << "(PC)";
            }
            cout << endl;
        }
        cout << endl;
        cout << " " << "[0x64] [" << bitset<16>(memory->memory[0x64]) << "] (" << dec << memory->memory[0x64] << ") " << endl;
        cout << "--------------------------" << endl;
    }


public:
    shared_ptr<Alu> alu;
    shared_ptr<Psw> psw;
    shared_ptr<Memory> memory;

    uint16_t mar = 0;
    uint16_t mdr = 0;

    map<InstructionType , shared_ptr<Instruction>> instructions;
    map<CpuStatus, string> statuses;

    uint16_t registers[8] = {0};
    uint16_t reg_b = 0;
    uint16_t ir = 0;

    uint16_t s_bus = 0;
    uint16_t a_bus = 0;
    uint16_t b_bus = 0;
    int clock_counter = 1;
    CpuStatus current_status = CpuStatus::FETCH_INST_0;
    shared_ptr<Instruction> current_inst;

    Cpu() {

    }
    Cpu(shared_ptr<Memory> memory) {
        clock_counter = 1;
        register_instructions();

        statuses[CpuStatus::FETCH_INST_0] = "FETCH_INST_0";
        statuses[CpuStatus::FETCH_INST_1] = "FETCH_INST_1";
        statuses[CpuStatus::FETCH_OPERAND_0] = "FETCH_OPERAND_0";
        statuses[CpuStatus::FETCH_OPERAND_1] = "FETCH_OPERAND_1";
        statuses[CpuStatus::EXEC_INST] = "EXEC_INST";
        statuses[CpuStatus::WRITE_BACK] = "WRITE_BACK";

        this->memory = memory;
        this->psw = shared_ptr<Psw>(new Psw(&this->registers[PSW_REG_NUMBER]));
        this->alu = shared_ptr<Alu>(new Alu(this->psw));
    }

    bool clock() {
        bool is_hlt = false;
        switch (current_status) {
            case CpuStatus::FETCH_INST_0:
                a_bus = registers[PC_REG_NUMBER];
                alu->mode = AluMode::NOP;
                s_bus = alu->calc(a_bus, b_bus);
                current_status = CpuStatus::FETCH_INST_1;
                break;
            case CpuStatus::FETCH_INST_1:
                mar = s_bus;
                memory->mode = MemoryMode::READ;
                memory->access(&mar, &mdr);
                alu->mode = AluMode::INC;
                s_bus = alu->calc(a_bus, b_bus);
                current_status = CpuStatus::FETCH_OPERAND_0;
                break;
            case CpuStatus::FETCH_OPERAND_0:
                ir = mdr;
                current_inst = decode(ir);
                registers[PC_REG_NUMBER] = s_bus;
                if (current_inst->type == InstructionType::MOV
                    || current_inst->type == InstructionType::ADD
                    || current_inst->type == InstructionType::SUB
                    || current_inst->type == InstructionType::AND
                    || current_inst->type == InstructionType::OR
                    || current_inst->type == InstructionType::CMP) {
                    a_bus = registers[current_inst->second_operand >> 5];
                    current_status = CpuStatus::EXEC_INST;
                } else if (current_inst->type == InstructionType::LD || current_inst->type == InstructionType::ST) {
                    a_bus = current_inst->second_operand;
                    current_status = CpuStatus::FETCH_OPERAND_1;
                } else {
                    current_status = CpuStatus::EXEC_INST;
                }
                alu->mode = AluMode::NOP;
                s_bus = alu->calc(a_bus, b_bus);

                break;
            case CpuStatus::FETCH_OPERAND_1:
                mar = s_bus;
                if (current_inst->type == InstructionType::LD) {
                    memory->mode = MemoryMode::READ;
                    memory->access(&mar, &mdr);
                }
                alu->mode = AluMode::NOP;
                s_bus = alu->calc(a_bus, b_bus);
                current_status = CpuStatus::EXEC_INST;
                break;
            case CpuStatus::EXEC_INST:
                switch (current_inst->type) {
                    case InstructionType::HLT:
                        is_hlt = true;
                        break;
                    case InstructionType::ADD:
                        reg_b = s_bus;
                        b_bus = reg_b;
                        a_bus = registers[current_inst->first_operand];
                        alu->mode = AluMode::ADD;
                        break;
                    case InstructionType::LDL:
                        a_bus = current_inst->second_operand;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::LDH:
                        a_bus = current_inst->second_operand << 8;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::CMP:
                        reg_b = s_bus;
                        b_bus = reg_b;
                        a_bus = registers[current_inst->first_operand];
                        alu->mode = AluMode::CMP;
                        break;
                    case InstructionType::JE:
                        a_bus = current_inst->second_operand;
                        current_inst->first_operand = PC_REG_NUMBER;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::JMP:
                        a_bus = current_inst->second_operand;
                        current_inst->first_operand = PC_REG_NUMBER;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::LD:
                        a_bus = mdr;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::ST:
                        a_bus = registers[current_inst->first_operand];
                        alu->mode = AluMode::NOP;
                        break;
                }
                s_bus = alu->calc(a_bus, b_bus);

                current_status = CpuStatus::WRITE_BACK;
                break;
            case CpuStatus::WRITE_BACK:
                if (current_inst->type == InstructionType::ST) {
                    mdr = s_bus;
                    memory->mode = MemoryMode::WRITE;
                    memory->access(&mar, &mdr);
                } else if (current_inst->type == InstructionType::LDL || current_inst->type == InstructionType::LDH) {
                    registers[current_inst->first_operand] |= s_bus;
                } else if(current_inst->type == InstructionType::JE) {
                    if (psw->get_zero_flag()) {
                        registers[current_inst->first_operand] = s_bus;
                    }
                } else if (current_inst->type == InstructionType::CMP) {
                    // pass
                } else {
                    registers[current_inst->first_operand] = s_bus;
                }
                current_status = CpuStatus::FETCH_INST_0;
                break;
        }
        print_info();
        clock_counter++;
        return is_hlt;
    }

};


#endif //EMULATOR_CPU_HPP
