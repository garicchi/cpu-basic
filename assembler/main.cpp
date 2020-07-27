#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <map>
#include <bitset>

using namespace std;

map<string, uint16_t> instructions = {
        {"mov", 0b0000},
        {"add", 0b0001}
};

map<string, uint16_t> registers = {
        {"r0", 0b000},
        {"r1", 0b001}
};

enum TokenType {
    RESERVED,
    HEX,
    IDENT,
    EOL,
    COMMA
};

struct Token {
    string str;
    TokenType type;
};

uint16_t assemble_opcode(Token opcode_token) {
    if (instructions.find(opcode_token.str) == instructions.end()) {
        cerr << "invalid opcode " << opcode_token.str << endl;
        exit(1);
    }
    return instructions[opcode_token.str];
}

uint16_t assemble_operand(Token operand_token, shared_ptr<map<string, int>> label_table) {
    if (operand_token.type == TokenType::HEX) {
        return stoi(operand_token.str, 0, 16);
    }
    if (label_table->find(operand_token.str) != label_table->end()) {
        return label_table->at(operand_token.str);
    }
    if (registers.find(operand_token.str) != registers.end()) {
        return registers.at(operand_token.str);
    }
    cerr << "invalid operand " << operand_token.str << endl;
    exit(1);
    return NULL;
}

shared_ptr<vector<uint16_t>> parse(shared_ptr<vector<Token>> tokens) {
    shared_ptr<vector<uint16_t>> output_code(new vector<uint16_t>());
    uint16_t code;
    shared_ptr<map<string, int>> label_table(new map<string, int>());
    for (int current_pos = 0; current_pos < tokens->size(); current_pos++) {
        Token *first_token = &tokens->at(current_pos++);
        if (first_token->type != TokenType::IDENT) {
            Token *first_operand = &tokens->at(current_pos++);
            Token *second_operand = NULL;
            if (tokens->at(current_pos).type == TokenType::COMMA) {
                tokens->at(current_pos++); // comma
                second_operand = &tokens->at(current_pos++);
            }
            string debug_asm = "";
            code = 0;
            code |= (assemble_opcode(*first_token) << 12);
            debug_asm += first_token->str + " ";
            if (second_operand != NULL) {
                code |= (assemble_operand(*first_operand, label_table) << 9);
                code |= (assemble_operand(*second_operand, label_table) << 6);
                debug_asm += first_operand->str + ", " + second_operand->str;
            } else {
                code |= (assemble_operand(*first_operand, label_table));
                debug_asm += first_operand->str;
            }
            output_code->push_back(code);
            cout << debug_asm << "\t" << bitset<16>(code) << endl;
        } else {
            label_table->insert(make_pair(first_token->str, output_code->size() * 2));
            current_pos++;
        }
    }
    return output_code;
}

shared_ptr<vector<Token>> tokenize(shared_ptr<string> code_str) {
    shared_ptr<vector<Token>> tokens(new vector<Token>());
    string current_token_str;
    for (int current_pos = 0; current_pos < code_str->length(); current_pos++) {
        char current_char = code_str->at(current_pos);
        char next_char = NULL;
        if ((current_pos + 1) < code_str->length()) {
            next_char = code_str->at(current_pos + 1);
        }
        if (current_char != ' ' && current_char != '\t') {
            current_token_str.push_back(current_char);
        }
        if (current_char == ',' || current_char == '\n') {
            TokenType token_type = TokenType::COMMA;
            if (current_char == ',') {
                token_type = TokenType::COMMA;
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
            if (instructions.find(current_token_str) != instructions.end()) {
                token_type = TokenType::RESERVED;
            }
            if (registers.find(current_token_str) != registers.end()) {
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
    auto code = parse(tokens);

    ofstream ofs(output_file, ios::binary);
    ofs.write((char*)&code->at(0), code->size() * 2);
    ofs.close();

    return 0;
}
