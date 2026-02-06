/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.5 - Code Generator
 * Suporte a Lambdas, Streams, Async/Await, Pipe
 */

#ifndef KAVA_CODEGEN_H
#define KAVA_CODEGEN_H

#include "ast.h"
#include "../vm/bytecode.h"
#include <vector>
#include <map>
#include <string>

namespace Kava {

class Codegen {
public:
    std::vector<int32_t> generate(const Program& program);

private:
    std::vector<int32_t> bytecode;
    std::map<std::string, int> variables;
    std::map<std::string, int> methods;
    int nextVarIdx = 0;
    int nextMethodIdx = 0;
    
    // Stack de loops para break/continue
    std::vector<int> loopStarts;
    std::vector<std::vector<int>> breakPatches;
    std::vector<std::vector<int>> continuePatches;

    void emit(int32_t value) { bytecode.push_back(value); }
    void emitAt(int index, int32_t value) { bytecode[index] = value; }
    int currentAddress() const { return bytecode.size(); }

    // Lambda/closure tracking
    struct LambdaInfo {
        int codeStart;
        int paramCount;
        std::vector<std::string> captures;
    };
    std::vector<LambdaInfo> lambdas;
    int nextLambdaIdx = 0;
    
    void visitStatement(StmtPtr stmt);
    void visitExpression(ExprPtr expr);
    void visitLambda(std::shared_ptr<class LambdaExpr> lambda);
    void visitStream(std::shared_ptr<class StreamExpr> stream);
    void visitAwait(std::shared_ptr<class AwaitExpr> awaitExpr);
    void visitPipe(std::shared_ptr<class PipeExpr> pipe);
    
    // Helpers
    void enterLoop(int startAddr) {
        loopStarts.push_back(startAddr);
        breakPatches.push_back({});
        continuePatches.push_back({});
    }
    
    void exitLoop() {
        // Patch breaks
        for (int addr : breakPatches.back()) {
            bytecode[addr] = currentAddress();
        }
        breakPatches.pop_back();
        
        // Patch continues
        for (int addr : continuePatches.back()) {
            bytecode[addr] = loopStarts.back();
        }
        continuePatches.pop_back();
        
        loopStarts.pop_back();
    }
    
    void emitBreak() {
        if (!breakPatches.empty()) {
            emit(OP_JMP);
            breakPatches.back().push_back(currentAddress());
            emit(0);
        }
    }
    
    void emitContinue() {
        if (!continuePatches.empty()) {
            emit(OP_JMP);
            continuePatches.back().push_back(currentAddress());
            emit(0);
        }
    }
};

} // namespace Kava

#endif // KAVA_CODEGEN_H
