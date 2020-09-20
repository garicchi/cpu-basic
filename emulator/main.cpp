#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <memory>
#include <unistd.h>
#include "memory.hpp"
#include "cpu.hpp"

using namespace std;

void load_program(shared_ptr<Memory>memory, string file_path) {
    ifstream ifs(file_path, ios::in | ios::binary);
    uint16_t buff;
    vector<uint16_t> program;
    while(!ifs.eof()) {
        ifs.read((char*)&buff, sizeof(uint16_t));
        program.push_back(buff);
    }
    for (int i = 0; i< program.size(); i++) {
        memory->memory[i] = program.at(i);
    }
}

int main(int argc, char *argv[]) {
    char *program_file = argv[1];

    shared_ptr<CpuArch> arch(new CpuArch());
    shared_ptr<Memory> memory(new Memory());
    shared_ptr<Cpu> cpu(new Cpu(memory, arch));
    load_program(memory, string(program_file));

    while(true) {
        if (cpu->clock()) {
            break;
        }
        usleep(100000);
    }

    cout << "RESULT is [" << memory->memory[0x64] << "]" << endl;

    return 0;
}
