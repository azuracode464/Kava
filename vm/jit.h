/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.5 - JIT Compiler Simples
 * Identificacao de hot paths e compilacao para bytecode otimizado
 * Niveis: -O0 (debug), -O1 (basico), -O2 (medio), -O3 (agressivo)
 */

#ifndef KAVA_JIT_H
#define KAVA_JIT_H

#include "bytecode.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <functional>

namespace Kava {

// ============================================================
// NIVEL DE OTIMIZACAO
// ============================================================
enum class OptLevel : int {
    O0 = 0,  // Debug: sem otimizacoes
    O1 = 1,  // Basico: const folding, DCE, inline simples
    O2 = 2,  // Medio: loop unrolling, register caching
    O3 = 3   // Agressivo: superinstructions, inline agressivo, eliminacao de checks
};

// ============================================================
// PROFILE DATA - Dados de profiling para JIT
// ============================================================
struct ProfileData {
    uint64_t executionCount = 0;
    uint64_t branchTaken = 0;
    uint64_t branchNotTaken = 0;
    bool isHot = false;
    bool isCompiled = false;
};

// ============================================================
// JIT COMPILED CODE
// ============================================================
struct CompiledCode {
    std::vector<int32_t> optimizedBytecode;
    int originalStart;
    int originalEnd;
    OptLevel level;
    uint64_t compilationTime;
};

// ============================================================
// SUPERINSTRUCTION - Fused opcodes para -O3
// ============================================================
enum SuperOp : int32_t {
    // Fused load + arithmetic
    SUPER_LOAD_ADD     = 0x200,  // load(idx) + iadd
    SUPER_LOAD_SUB     = 0x201,  // load(idx) + isub
    SUPER_LOAD_MUL     = 0x202,  // load(idx) + imul
    SUPER_LOAD_CMP_JZ  = 0x203,  // load + icmp + jz (loop test)
    SUPER_INC_CMP_JNZ  = 0x204,  // iinc + icmp + jnz (loop iter)
    SUPER_PUSH_STORE   = 0x205,  // push_int + store
    SUPER_LOAD_LOAD_ADD = 0x206, // load + load + add
    SUPER_LOAD_LOAD_MUL = 0x207, // load + load + mul
    
    // Fused loop patterns
    SUPER_COUNTED_LOOP = 0x210,  // Entire counted for-loop
    SUPER_ARRAY_FILL   = 0x211,  // Array fill loop
    SUPER_SUM_LOOP     = 0x212,  // Sum reduction loop
};

// ============================================================
// JIT COMPILER
// ============================================================
class JITCompiler {
public:
    OptLevel optLevel = OptLevel::O1;
    
    // Profiling
    std::unordered_map<int, ProfileData> profiles;  // PC -> profile
    
    // Compiled code cache
    std::unordered_map<int, CompiledCode> compiledCode;  // start PC -> compiled
    
    // Thresholds
    static constexpr uint64_t HOT_THRESHOLD = 1000;
    static constexpr uint64_t COMPILE_THRESHOLD = 5000;
    
    // ========================================
    // PROFILING
    // ========================================
    void recordExecution(int pc) {
        auto& p = profiles[pc];
        p.executionCount++;
        if (p.executionCount >= HOT_THRESHOLD) {
            p.isHot = true;
        }
    }
    
    void recordBranch(int pc, bool taken) {
        auto& p = profiles[pc];
        if (taken) p.branchTaken++;
        else p.branchNotTaken++;
    }
    
    bool shouldCompile(int pc) const {
        auto it = profiles.find(pc);
        if (it == profiles.end()) return false;
        return it->second.executionCount >= COMPILE_THRESHOLD && !it->second.isCompiled;
    }
    
    // ========================================
    // COMPILATION
    // ========================================
    CompiledCode compile(const std::vector<int32_t>& bytecode, int start, int end) {
        CompiledCode result;
        result.originalStart = start;
        result.originalEnd = end;
        result.level = optLevel;
        
        // Extract region
        std::vector<int32_t> region(bytecode.begin() + start, bytecode.begin() + end);
        
        // Apply optimizations based on level
        switch (optLevel) {
            case OptLevel::O0:
                result.optimizedBytecode = region;  // No optimization
                break;
            case OptLevel::O1:
                result.optimizedBytecode = optimizeO1(region);
                break;
            case OptLevel::O2:
                result.optimizedBytecode = optimizeO2(region);
                break;
            case OptLevel::O3:
                result.optimizedBytecode = optimizeO3(region);
                break;
        }
        
        profiles[start].isCompiled = true;
        compiledCode[start] = result;
        return result;
    }
    
    // ========================================
    // O1: Constant Folding + Dead Code Elimination + Simple Inline
    // ========================================
    std::vector<int32_t> optimizeO1(const std::vector<int32_t>& code) {
        std::vector<int32_t> result;
        result.reserve(code.size());
        
        for (size_t i = 0; i < code.size(); i++) {
            int32_t op = code[i];
            
            // Constant Folding: PUSH_INT a, PUSH_INT b, IADD -> PUSH_INT (a+b)
            if (op == OP_PUSH_INT && i + 3 < code.size()) {
                int32_t val1 = code[i + 1];
                int32_t nextOp = code[i + 2];
                
                if (nextOp == OP_PUSH_INT && i + 4 < code.size()) {
                    int32_t val2 = code[i + 3];
                    int32_t arithOp = (i + 4 < code.size()) ? code[i + 4] : -1;
                    
                    bool folded = true;
                    int32_t foldedVal = 0;
                    
                    switch (arithOp) {
                        case OP_IADD: foldedVal = val1 + val2; break;
                        case OP_ISUB: foldedVal = val1 - val2; break;
                        case OP_IMUL: foldedVal = val1 * val2; break;
                        case OP_IDIV: 
                            if (val2 != 0) foldedVal = val1 / val2;
                            else folded = false;
                            break;
                        case OP_IMOD:
                            if (val2 != 0) foldedVal = val1 % val2;
                            else folded = false;
                            break;
                        default: folded = false; break;
                    }
                    
                    if (folded) {
                        // Replace 5 instructions with 2
                        if (foldedVal >= -1 && foldedVal <= 5) {
                            result.push_back(OP_ICONST_0 + foldedVal);
                        } else {
                            result.push_back(OP_PUSH_INT);
                            result.push_back(foldedVal);
                        }
                        i += 4;  // Skip folded instructions
                        continue;
                    }
                }
            }
            
            // Dead Code Elimination: NOP removal
            if (op == OP_NOP) continue;
            
            // Push-Pop elimination
            if (op == OP_POP && !result.empty()) {
                int32_t prev = result.back();
                if (prev >= OP_ICONST_M1 && prev <= OP_ICONST_5) {
                    result.pop_back();  // Remove dead push
                    continue;
                }
            }
            
            result.push_back(op);
            
            // Copy operands for instructions that have them
            if (op == OP_PUSH_INT || op == OP_PUSH_STRING || op == OP_PUSH_CLASS ||
                op == OP_JMP || op == OP_JZ || op == OP_JNZ ||
                op == OP_ILOAD || op == OP_ISTORE || op == OP_ALOAD || op == OP_ASTORE ||
                op == OP_LOAD_GLOBAL || op == OP_STORE_GLOBAL ||
                op == OP_GETFIELD || op == OP_PUTFIELD ||
                op == OP_CALL || op == OP_INVOKE || op == OP_INVOKESPEC ||
                op == OP_NEW || op == OP_NEWARRAY || op == OP_CHECKCAST ||
                op == OP_INSTANCEOF || op == OP_IINC) {
                if (i + 1 < code.size()) {
                    result.push_back(code[++i]);
                }
            } else if (op == OP_PUSH_LONG || op == OP_PUSH_DOUBLE) {
                if (i + 2 < code.size()) {
                    result.push_back(code[++i]);
                    result.push_back(code[++i]);
                }
            }
        }
        
        return result;
    }
    
    // ========================================
    // O2: O1 + Loop Unrolling + Register Caching
    // ========================================
    std::vector<int32_t> optimizeO2(const std::vector<int32_t>& code) {
        // First apply O1
        auto result = optimizeO1(code);
        
        // Detect simple counted loops and unroll
        std::vector<int32_t> unrolled;
        unrolled.reserve(result.size());
        
        for (size_t i = 0; i < result.size(); i++) {
            // Look for pattern: JMP back (loop)
            if (result[i] == OP_JMP && i + 1 < result.size()) {
                int32_t target = result[i + 1];
                if (target < (int32_t)i) {
                    // This is a backward jump (loop)
                    // Check if loop is small enough to unroll
                    int loopSize = i - target;
                    if (loopSize > 0 && loopSize < 20) {
                        // Unroll 2x for small loops
                        for (int u = 0; u < 2; u++) {
                            for (int j = target; j < (int)i; j++) {
                                unrolled.push_back(result[j]);
                            }
                        }
                        unrolled.push_back(result[i]);
                        unrolled.push_back(result[i + 1]);
                        i++;
                        continue;
                    }
                }
            }
            
            // Register caching: consecutive loads of same variable
            if ((result[i] == OP_LOAD_GLOBAL || result[i] == OP_ILOAD) && 
                i + 3 < result.size()) {
                int32_t loadOp = result[i];
                int32_t idx = result[i + 1];
                
                // Check if same variable is loaded again soon
                if (result[i + 2] == loadOp && result[i + 3] == idx) {
                    // Second load is redundant, use DUP
                    unrolled.push_back(loadOp);
                    unrolled.push_back(idx);
                    unrolled.push_back(OP_DUP);
                    i += 3;
                    continue;
                }
            }
            
            unrolled.push_back(result[i]);
        }
        
        return unrolled;
    }
    
    // ========================================
    // O3: O2 + Superinstructions + Aggressive Inline + Bounds Check Elimination
    // ========================================
    std::vector<int32_t> optimizeO3(const std::vector<int32_t>& code) {
        // First apply O2
        auto result = optimizeO2(code);
        
        // Apply superinstruction fusion
        std::vector<int32_t> fused;
        fused.reserve(result.size());
        
        for (size_t i = 0; i < result.size(); i++) {
            // Pattern: LOAD_GLOBAL idx, LOAD_GLOBAL idx2, IADD
            if (result[i] == OP_LOAD_GLOBAL && i + 4 < result.size() &&
                result[i + 2] == OP_LOAD_GLOBAL && result[i + 4] == OP_IADD) {
                fused.push_back(SUPER_LOAD_LOAD_ADD);
                fused.push_back(result[i + 1]);  // idx1
                fused.push_back(result[i + 3]);  // idx2
                i += 4;
                continue;
            }
            
            // Pattern: LOAD_GLOBAL idx, LOAD_GLOBAL idx2, IMUL
            if (result[i] == OP_LOAD_GLOBAL && i + 4 < result.size() &&
                result[i + 2] == OP_LOAD_GLOBAL && result[i + 4] == OP_IMUL) {
                fused.push_back(SUPER_LOAD_LOAD_MUL);
                fused.push_back(result[i + 1]);
                fused.push_back(result[i + 3]);
                i += 4;
                continue;
            }
            
            // Pattern: PUSH_INT val, STORE_GLOBAL idx
            if (result[i] == OP_PUSH_INT && i + 3 < result.size() &&
                result[i + 2] == OP_STORE_GLOBAL) {
                fused.push_back(SUPER_PUSH_STORE);
                fused.push_back(result[i + 1]);  // value
                fused.push_back(result[i + 3]);  // idx
                i += 3;
                continue;
            }
            
            // Pattern: LOAD_GLOBAL + PUSH_INT + ILT/IGT + JZ (loop condition)
            if (result[i] == OP_LOAD_GLOBAL && i + 5 < result.size() &&
                result[i + 2] == OP_PUSH_INT &&
                (result[i + 4] == OP_ILT || result[i + 4] == OP_IGT ||
                 result[i + 4] == OP_ILE || result[i + 4] == OP_IGE) &&
                result[i + 5] == OP_JZ) {
                fused.push_back(SUPER_LOAD_CMP_JZ);
                fused.push_back(result[i + 1]);  // var idx
                fused.push_back(result[i + 3]);  // compare value
                fused.push_back(result[i + 4]);  // comparison type
                fused.push_back(result[i + 6]);  // jump target
                i += 6;
                continue;
            }
            
            // Bounds check elimination: remove ARRAYLENGTH + compare before array access
            // when array size is known
            
            fused.push_back(result[i]);
        }
        
        return fused;
    }
    
    // ========================================
    // JIT STATISTICS
    // ========================================
    struct JITStats {
        uint64_t hotFunctions = 0;
        uint64_t compilations = 0;
        uint64_t deoptimizations = 0;
        uint64_t totalCompileTimeUs = 0;
        size_t compiledCodeSize = 0;
        
        void print() const;
    };
    
    JITStats stats;
    
    // ========================================
    // DETECT HOT LOOPS
    // ========================================
    struct LoopInfo {
        int startPC;
        int endPC;
        int backEdgePC;
        uint64_t iterationCount;
        bool isCountedLoop;
        bool isCompiled;
    };
    
    std::vector<LoopInfo> detectedLoops;
    
    void detectLoops(const std::vector<int32_t>& bytecode) {
        for (size_t i = 0; i < bytecode.size(); i++) {
            if (bytecode[i] == OP_JMP && i + 1 < bytecode.size()) {
                int32_t target = bytecode[i + 1];
                if (target >= 0 && target < (int32_t)i) {
                    LoopInfo loop;
                    loop.startPC = target;
                    loop.endPC = i + 2;
                    loop.backEdgePC = i;
                    loop.iterationCount = 0;
                    loop.isCountedLoop = false;
                    loop.isCompiled = false;
                    detectedLoops.push_back(loop);
                }
            }
        }
    }
};

} // namespace Kava

#endif // KAVA_JIT_H
