/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 */

#ifndef KAVA_SEMANTIC_H
#define KAVA_SEMANTIC_H

#include "ast.h"
#include <map>
#include <string>

namespace Kava {

struct Symbol {
    std::string name;
    bool isGlobal;
    int index;
};

class SemanticAnalyzer {
public:
    void analyze(Program& program);
private:
    std::map<std::string, Symbol> symbolTable;
    int nextGlobalIndex = 0;
};

} // namespace Kava

#endif // KAVA_SEMANTIC_H
