/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 *
 * KAVA 2.0 - VM Entry Point
 */

#include "vm.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: kavavm <arquivo.kvb>" << std::endl;
        return 1;
    }
    
    Kava::VM vm;
    
    if (!vm.loadBytecodeFile(argv[1])) {
        std::cerr << "Erro ao carregar: " << argv[1] << std::endl;
        return 1;
    }
    
    vm.run();
    
    return 0;
}
