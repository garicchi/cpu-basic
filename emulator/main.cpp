#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <map>
#include <memory>
#include <unistd.h>

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

enum class CpuStatus {
    LOAD_INST_0,
    LOAD_INST_1,
    LOAD_OPERAND_0,
    LOAD_OPERAND_1,
    EXE_INST,
    STORE,
};

enum class AluMode {
    ADD,
    SUB,
    CMP,
    AND,
    OR,
    XOR,
    NOT,
    INC,
    DEC,
    NOP
};



map<InstructionType , shared_ptr<Instruction>> instructions;


const int PSW_REG_NUMBER = 5;
const int SP_REG_NUMBER = 6;
const int PC_REG_NUMBER = 7;

uint16_t registers[8] = {0};
uint16_t reg_b = 0;
uint16_t ir = 0;
uint16_t mar = 0;
uint16_t mdr = 0;
uint16_t memory[256] = {0};

uint16_t mem_bus_addr = 0;
uint16_t mem_bus_data = 0;
uint16_t s_bus = 0;
uint16_t a_bus = 0;
uint16_t b_bus = 0;

class Alu {
public:
    AluMode mode;

    Alu() {
        this->mode = AluMode::NOP;
    }

    uint16_t calc(uint16_t a_bus, uint16_t b_bus) {
        uint16_t result = 0;
        switch (this->mode) {
            case AluMode::ADD:
                result = a_bus + b_bus;
                break;
            case AluMode::SUB:
                result = a_bus - b_bus;
                break;
            case AluMode::CMP:
                result = a_bus - b_bus;
                if (result < 0) {
                    set_negative_flag(1);
                } else if (result > 0) {
                    set_negative_flag(0);
                }
                if (result == 0) {
                    set_zero_flag(1);
                } else {
                    set_zero_flag(0);
                }
                break;
            case AluMode::INC:
                result = a_bus + 1;
                break;
            case AluMode::DEC:
                result = a_bus - 1;
                break;
            case AluMode::NOP:
                result = a_bus;
                break;
        }
        return result;
    }

    void set_negative_flag(bool is_on) {
        uint16_t mask = 0b1000000000000000;
        if (is_on) {
            registers[PSW_REG_NUMBER] |= mask;
        } else {
            registers[PSW_REG_NUMBER] &= ~mask;
        }
    }

    void set_zero_flag(bool is_on) {
        uint16_t mask = 0b0100000000000000;
        if (is_on) {
            registers[PSW_REG_NUMBER] |= mask;
        } else {
            registers[PSW_REG_NUMBER] &= ~mask;
        }
    }

    uint16_t get_zero_flag() {
        uint16_t mask = 0b0100000000000000;
        return (registers[PSW_REG_NUMBER] >> 14) & 0x1;
    }
};

shared_ptr<Alu> alu(new Alu());

CpuStatus current_status = CpuStatus::LOAD_INST_0;
shared_ptr<Instruction> current_instruction;


shared_ptr<Instruction> decode_instruction(uint16_t code) {
    uint16_t mask_opcode = 0b0111100000000000;
    uint16_t mask_first_operand = 0b0000011100000000;
    uint16_t mask_second_operand = 0b0000000011111111;

    uint16_t opcode = (code & mask_opcode) >> 11;
    auto inst = instructions[static_cast<InstructionType>(opcode)];
    cout << "[" << registers[PC_REG_NUMBER] << "] " << inst->mnemonic << " ";
    if (inst->operand_type == OperandType::SINGLE_OPERAND || inst->operand_type == OperandType::DOUBLE_OPERAND) {
        inst->first_operand = (code & mask_first_operand) >> 8;
        cout << bitset<3>(inst->first_operand) << " ";
        inst->second_operand = code & mask_second_operand;
        cout << bitset<8>(inst->second_operand) << " ";
    }

    cout << endl;
    return inst;
}

void load_memory() {
    mem_bus_addr = mar;
    mem_bus_data = memory[mem_bus_addr];
}

void store_memory() {
    mem_bus_data = mdr;
    mem_bus_addr = mar;
    memory[mem_bus_addr] = mem_bus_data;
}

void update_status() {
    switch (current_status) {
        case CpuStatus::LOAD_INST_0:
            a_bus = registers[PC_REG_NUMBER];
            alu->mode = AluMode::NOP;
            s_bus = alu->calc(a_bus, b_bus);
            current_status = CpuStatus::LOAD_INST_1;
            break;
        case CpuStatus::LOAD_INST_1:
            mar = s_bus;
            load_memory();
            alu->mode = AluMode::INC;
            s_bus = alu->calc(a_bus, b_bus);
            current_status = CpuStatus::LOAD_OPERAND_0;
            break;
        case CpuStatus::LOAD_OPERAND_0:
            ir = mem_bus_data;
            current_instruction = decode_instruction(ir);
            registers[PC_REG_NUMBER] = s_bus;
            if (current_instruction->type == InstructionType::MOV
                || current_instruction->type == InstructionType::ADD
                || current_instruction->type == InstructionType::SUB
                || current_instruction->type == InstructionType::AND
                || current_instruction->type == InstructionType::OR
                || current_instruction->type == InstructionType::CMP) {
                a_bus = registers[current_instruction->second_operand >> 5];
                current_status = CpuStatus::EXE_INST;
            } else if (current_instruction->type == InstructionType::LD || current_instruction->type == InstructionType::ST) {
                a_bus = current_instruction->second_operand;
                current_status = CpuStatus::LOAD_OPERAND_1;
            } else {
                current_status = CpuStatus::EXE_INST;
            }
            alu->mode = AluMode::NOP;
            s_bus = alu->calc(a_bus, b_bus);

            break;
        case CpuStatus::LOAD_OPERAND_1:
            mar = s_bus;
            if (current_instruction->type == InstructionType::LD) {
                load_memory();
            }
            alu->mode = AluMode::NOP;
            s_bus = alu->calc(a_bus, b_bus);
            current_status = CpuStatus::EXE_INST;
            break;
        case CpuStatus::EXE_INST:
            switch (current_instruction->type) {
                case InstructionType::HLT:
                    cout << memory[0x64] << endl;
                    exit(0);
                    break;
                case InstructionType::ADD:
                    reg_b = s_bus;
                    b_bus = reg_b;
                    a_bus = registers[current_instruction->first_operand];
                    alu->mode = AluMode::ADD;
                    break;
                case InstructionType::LDL:
                    a_bus = current_instruction->second_operand;
                    alu->mode = AluMode::NOP;
                    break;
                case InstructionType::LDH:
                    a_bus = current_instruction->second_operand << 8;
                    alu->mode = AluMode::NOP;
                    break;
                case InstructionType::CMP:
                    reg_b = s_bus;
                    b_bus = reg_b;
                    a_bus = registers[current_instruction->first_operand];
                    alu->mode = AluMode::CMP;
                    break;
                case InstructionType::JE:
                    a_bus = current_instruction->second_operand;
                    current_instruction->first_operand = PC_REG_NUMBER;
                    alu->mode = AluMode::NOP;
                    break;
                case InstructionType::JMP:
                    a_bus = current_instruction->second_operand;
                    current_instruction->first_operand = PC_REG_NUMBER;
                    alu->mode = AluMode::NOP;
                    break;
                case InstructionType::LD:
                    mdr = mem_bus_data;
                    a_bus = mdr;
                    alu->mode = AluMode::NOP;
                    break;
                case InstructionType::ST:
                    a_bus = registers[current_instruction->first_operand];
                    alu->mode = AluMode::NOP;
                    break;
            }
            s_bus = alu->calc(a_bus, b_bus);

            current_status = CpuStatus::STORE;
            break;
        case CpuStatus::STORE:
            if (current_instruction->type == InstructionType::ST) {
                mdr = s_bus;
                store_memory();
            } else if (current_instruction->type == InstructionType::LDL || current_instruction->type == InstructionType::LDH) {
                registers[current_instruction->first_operand] |= s_bus;
            } else if(current_instruction->type == InstructionType::JE) {
                if (alu->get_zero_flag()) {
                    registers[current_instruction->first_operand] = s_bus;
                }
            } else if (current_instruction->type == InstructionType::CMP) {
                // pass
            } else {
                registers[current_instruction->first_operand] = s_bus;
            }
            current_status = CpuStatus::LOAD_INST_0;
            break;
    }
}

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
        instructions[inst->type] = inst;
    }
}

int main(int argc, char *argv[]) {
    char *program_file = argv[1];
    ifstream ifs(program_file,ios::in | ios::binary);
    uint16_t buff;
    vector<uint16_t> program;
    while(!ifs.eof()) {
        ifs.read((char*)&buff, sizeof(uint16_t));
        program.push_back(buff);
    }
    for (int i = 0; i< program.size(); i++) {
        memory[i] = program.at(i);
    }

    register_instructions();

    char input;
    while(true) {
        update_status();
        //cout << "r0 " << bitset<16>(registers[0]) << endl;
        //cout << "r2 " << bitset<16>(registers[2]) << endl;
        usleep(100000);
    }

    return 0;
}
