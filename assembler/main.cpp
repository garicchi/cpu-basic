#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <map>
#include <bitset>
#include "arch.hpp"

// 15  14 13 12 11   10 9 8   7 6 5   4 3 2 1 0
// -   命令           source   dest    ----

// 15  14 13 12 11   10 9 8   7 6 5 4 3 2 1 0
// -   命令           source   data

using namespace std;

enum class TokenType {
    RESERVED,
    HEX,
    IDENT,
    EOL,
    COMMA,
    COLON
};

struct Token {
    string str;
    TokenType type;
};

uint16_t resolve_operand(Token operand_token, shared_ptr<map<string, int>> label_table) {
    if (operand_token.type == TokenType::HEX) {
        return stoi(operand_token.str, 0, 16);
    }
    if (label_table->find(operand_token.str) != label_table->end()) {
        return label_table->at(operand_token.str);
    }
    auto reg = get_register_by_name(operand_token.str);
    if (reg != nullptr) {
        return reg->code;
    }
    cerr << "invalid operand " << operand_token.str << endl;
    exit(1);
    return NULL;
}

shared_ptr<vector<uint16_t>> generate(shared_ptr<vector<Token>> tokens) {
    shared_ptr<map<string, int>> label_table(new map<string, int>());
    int code_index = 0;
    for (int current_pos = 0; current_pos < tokens->size();) {
        Token *first_token = &tokens->at(current_pos);
        Token *second_token = &tokens->at(current_pos + 1);

        if (first_token->type == TokenType::EOL) {
            current_pos++;
            continue;
        }
        if (first_token->type == TokenType::IDENT && second_token->type == TokenType::COLON) {
            label_table->insert(make_pair(first_token->str, code_index));
            current_pos += 3;
            continue;
        }
        if (second_token->type == TokenType::EOL && first_token->type != TokenType::COLON) {
            code_index++;
            current_pos += 2;
            continue;
        }
        current_pos++;
    }
    vector<Program> programs;
    for (int current_pos = 0; current_pos < tokens->size();) {
        Token *first_token = &tokens->at(current_pos++);
        if (first_token->type != TokenType::IDENT && first_token->type != TokenType::EOL) {
            Token *first_operand_token = NULL;
            Token *second_operand_token = NULL;
            if (tokens->at(current_pos).type != TokenType::EOL && tokens->at(current_pos).type != TokenType::COMMA) {
                first_operand_token = &tokens->at(current_pos++);
            }
            if (tokens->at(current_pos).type == TokenType::COMMA) {
                tokens->at(current_pos++); // comma
                second_operand_token = &tokens->at(current_pos++);
            }

            shared_ptr<Instruction> inst = get_inst_by_mnemonic(first_token->str);
            if (inst == nullptr) {
                cout << "invalid inst [" << first_token->str << "]" << endl;
            }
            uint16_t first_operand = 0;
            uint16_t second_operand = 0;

            if (inst->operand_type == OperandType::SINGLE_OPERAND) {
                first_operand = resolve_operand(*first_operand_token, label_table);
            } else if (inst->operand_type == OperandType::DOUBLE_OPERAND) {
                first_operand = resolve_operand(*first_operand_token, label_table);
                if (second_operand_token->type == TokenType::RESERVED) {
                    second_operand = resolve_operand(*second_operand_token, label_table) << 5;
                } else {
                    second_operand = resolve_operand(*second_operand_token, label_table);
                }

            }
            programs.push_back(Program(inst, first_operand, second_operand));
        } else {
            if (first_token->type == TokenType::IDENT) {
                current_pos++;
            }
        }
    }
    shared_ptr<vector<uint16_t>> output_code(new vector<uint16_t>());
    uint16_t code;
    for (auto program: programs) {
        code = Program::assemble(program);
        output_code->push_back(code);
        string debug_asm = "[" + to_string(output_code->size()) + "] ";
        debug_asm += program.inst->mnemonic + " ";
        if (program.inst->operand_type == OperandType::SINGLE_OPERAND) {
            debug_asm += to_string(program.first_operand);
        } else if (program.inst->operand_type == OperandType::DOUBLE_OPERAND) {
            debug_asm += to_string(program.first_operand) + ", " + to_string(program.second_operand);
        }
        cout << debug_asm << "\t" << bitset<16>(code) << endl;
    }
    return output_code;
}

shared_ptr<vector<Token>> tokenize(shared_ptr<string> code_str) {
    shared_ptr<vector<Token>> tokens(new vector<Token>());
    string current_token_str;
    bool is_among_comment = false;
    for (int current_pos = 0; current_pos < code_str->length(); current_pos++) {
        char current_char = code_str->at(current_pos);
        char next_char = NULL;
        if ((current_pos + 1) < code_str->length()) {
            next_char = code_str->at(current_pos + 1);
        }
        if (current_char == '#') {
            is_among_comment = true;
        }
        if (is_among_comment && current_char == '\n') {
            is_among_comment = false;
        }
        if (is_among_comment) {
            continue;
        }
        if (current_char != ' ' && current_char != '\t') {
            current_token_str.push_back(current_char);
        }

        if (current_char == ',' || current_char == ':' || current_char == '\n') {
            TokenType token_type = TokenType::COMMA;
            if (current_char == ',') {
                token_type = TokenType::COMMA;
            }
            if (current_char == ':') {
                token_type = TokenType::COLON;
            }
            if (current_char == '\n') {
                token_type = TokenType::EOL;
            }
            Token token;
            token.str = current_token_str;
            token.type = token_type;
            tokens->push_back(token);
            current_token_str.clear();
            continue;
        }
        if (next_char == ' ' || next_char == '\t' || next_char == ',' || next_char == '\n' || next_char == ':') {
            TokenType token_type = TokenType::IDENT;
            if (current_token_str.substr(0, 2) == "0x") {
                token_type = TokenType::HEX;
            }

            if (get_inst_by_mnemonic(current_token_str) != nullptr) {
                token_type = TokenType::RESERVED;
            }
            if (get_register_by_name(current_token_str) != nullptr) {
                token_type = TokenType::RESERVED;
            }

            Token token;
            token.str = current_token_str;
            token.type = token_type;
            tokens->push_back(token);
            current_token_str.clear();
        }

    }
    return tokens;
}

int main(int argc, char *argv[]) {
    char* input_file = argv[1];
    char *output_file = argv[2];
    ifstream ifs(input_file);
    shared_ptr<vector<string>> lines(new vector<string>());
    shared_ptr<string> text(new string());
    string line;
    while(getline(ifs, line)) {
        text->append(line + '\n');
    }
    ifs.close();

    auto tokens = tokenize(text);
    auto code = generate(tokens);

    ofstream ofs(output_file, ios::binary);
    ofs.write((char*)&code->at(0), code->size() * 2);
    ofs.close();

    return 0;
}
