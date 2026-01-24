/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Uso: kavac <arquivo.kava>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    try {
        // 1. Lexer
        Kava::Lexer lexer(source);
        std::vector<Kava::Token> tokens = lexer.scanTokens();

        // 2. Parser
        Kava::Parser parser(tokens);
        auto programPtr = parser.parse();

        // 3. Codegen
        Kava::Codegen codegen;
        std::vector<int32_t> bytecode = codegen.generate(*programPtr);

        // 4. Salvar .kvb
        std::string outPath = argv[1];
        size_t lastDot = outPath.find_last_of(".");
        if (lastDot != std::string::npos) outPath = outPath.substr(0, lastDot);
        outPath += ".kvb";

        std::ofstream outFile(outPath, std::ios::binary);
        for (int32_t op : bytecode) {
            outFile.write(reinterpret_cast<const char*>(&op), sizeof(int32_t));
        }
        outFile.close();

        std::cout << "Compilado com sucesso: " << outPath << " (" << bytecode.size() << " instruções)" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Erro de compilação: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
