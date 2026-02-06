/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.5 - Virtual Machine Completa
 * VM stack-based com JIT, Lambda, Streams, Async/Await
 */

#ifndef KAVA_VM_H
#define KAVA_VM_H

#include "bytecode.h"
#include "jit.h"
#include "async.h"
#include "../gc/gc.h"
#include "../threads/threads.h"
#include "../collections/collections.h"
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <chrono>
#include <iostream>
#include <fstream>
#include <cmath>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <numeric>

#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif

namespace Kava {

class VM;
class Frame;
class ClassInfo;
class MethodInfo;

// ============================================================
// VALUE
// ============================================================
union ValueData {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    GCObject* obj;
    void* ptr;
};

struct Value {
    enum class Type : uint8_t {
        Null, Int, Long, Float, Double, Object, Lambda
    };
    
    Type type;
    ValueData data;
    
    Value() : type(Type::Null) { data.i64 = 0; }
    Value(int32_t v) : type(Type::Int) { data.i32 = v; }
    Value(int64_t v) : type(Type::Long) { data.i64 = v; }
    Value(float v) : type(Type::Float) { data.f32 = v; }
    Value(double v) : type(Type::Double) { data.f64 = v; }
    Value(GCObject* v) : type(Type::Object) { data.obj = v; }
    
    bool isNull() const { return type == Type::Null || (type == Type::Object && data.obj == nullptr); }
    bool isInt() const { return type == Type::Int; }
    bool isLong() const { return type == Type::Long; }
    bool isFloat() const { return type == Type::Float; }
    bool isDouble() const { return type == Type::Double; }
    bool isObject() const { return type == Type::Object; }
    bool isLambda() const { return type == Type::Lambda; }
    
    int32_t asInt() const { return data.i32; }
    int64_t asLong() const { return data.i64; }
    float asFloat() const { return data.f32; }
    double asDouble() const { return data.f64; }
    GCObject* asObject() const { return data.obj; }
    
    int32_t toInt() const {
        switch (type) {
            case Type::Int: return data.i32;
            case Type::Long: return static_cast<int32_t>(data.i64);
            case Type::Float: return static_cast<int32_t>(data.f32);
            case Type::Double: return static_cast<int32_t>(data.f64);
            default: return 0;
        }
    }
    
    int64_t toLong() const {
        switch (type) {
            case Type::Int: return data.i32;
            case Type::Long: return data.i64;
            case Type::Float: return static_cast<int64_t>(data.f32);
            case Type::Double: return static_cast<int64_t>(data.f64);
            default: return 0;
        }
    }
    
    double toDouble() const {
        switch (type) {
            case Type::Int: return data.i32;
            case Type::Long: return static_cast<double>(data.i64);
            case Type::Float: return data.f32;
            case Type::Double: return data.f64;
            default: return 0.0;
        }
    }
    
    bool toBool() const { return toInt() != 0; }
};

// ============================================================
// CONSTANT POOL ENTRY
// ============================================================
struct ConstantPoolEntry {
    ConstantPoolTag tag;
    union {
        int32_t intValue;
        int64_t longValue;
        float floatValue;
        double doubleValue;
        uint16_t indices[2];
    };
    std::string stringValue;
};

// ============================================================
// METHOD / FIELD / CLASS INFO
// ============================================================
struct MethodInfo {
    std::string name;
    std::string descriptor;
    uint16_t accessFlags;
    uint16_t maxStack;
    uint16_t maxLocals;
    std::vector<uint8_t> code;
    std::vector<KavaExceptionEntry> exceptionTable;
    int codeOffset;
    
    bool isStatic() const { return accessFlags & KAVA_ACC_STATIC; }
    bool isNative() const { return accessFlags & KAVA_ACC_NATIVE; }
    bool isAbstract() const { return accessFlags & KAVA_ACC_ABSTRACT; }
    bool isSynchronized() const { return accessFlags & KAVA_ACC_SYNCHRONIZED; }
};

struct FieldInfo {
    std::string name;
    std::string descriptor;
    uint16_t accessFlags;
    int offset;
    Value defaultValue;
    
    bool isStatic() const { return accessFlags & KAVA_ACC_STATIC; }
};

struct ClassInfo {
    std::string name;
    uint16_t accessFlags;
    int32_t classId;
    int32_t superClassId;
    std::vector<int32_t> interfaceIds;
    
    std::vector<FieldInfo> fields;
    std::vector<FieldInfo> staticFields;
    std::vector<MethodInfo> methods;
    std::vector<Value> staticFieldValues;
    
    int instanceSize;
    bool initialized;
    
    ClassInfo() : classId(-1), superClassId(-1), instanceSize(0), initialized(false), accessFlags(0) {}
    
    MethodInfo* findMethod(const std::string& name, const std::string& desc) {
        for (auto& m : methods) {
            if (m.name == name && (desc.empty() || m.descriptor == desc)) return &m;
        }
        return nullptr;
    }
    
    FieldInfo* findField(const std::string& name) {
        for (auto& f : fields) { if (f.name == name) return &f; }
        for (auto& f : staticFields) { if (f.name == name) return &f; }
        return nullptr;
    }
    
    bool isInterface() const { return accessFlags & KAVA_ACC_INTERFACE; }
    bool isAbstract() const { return accessFlags & KAVA_ACC_ABSTRACT; }
};

// ============================================================
// STACK FRAME
// ============================================================
struct Frame {
    MethodInfo* method;
    ClassInfo* classInfo;
    std::vector<Value> locals;
    std::vector<Value> operandStack;
    int pc;
    int sp;
    Frame* caller;
    GCObject* pendingException;
    
    Frame(MethodInfo* m, ClassInfo* c, Frame* parent = nullptr)
        : method(m), classInfo(c), pc(0), sp(0), caller(parent), pendingException(nullptr) {
        if (m) {
            locals.resize(m->maxLocals);
            operandStack.resize(m->maxStack);
        }
    }
    
    void push(const Value& v) { operandStack[sp++] = v; }
    Value pop() { return operandStack[--sp]; }
    Value& peek(int offset = 0) { return operandStack[sp - 1 - offset]; }
    void setLocal(int index, const Value& v) { locals[index] = v; }
    Value& getLocal(int index) { return locals[index]; }
};

using NativeMethod = std::function<Value(VM*, Frame*, const std::vector<Value>&)>;

// ============================================================
// LAMBDA CLOSURE
// ============================================================
struct LambdaClosure {
    int codeStart;
    int paramCount;
    std::vector<Value> captures;
};

// ============================================================
// VM CONFIGURATION
// ============================================================
struct VMConfig {
    size_t maxHeapSize = 256 * 1024 * 1024;
    size_t initialHeapSize = 16 * 1024 * 1024;
    size_t maxStackSize = 1024 * 1024;
    int maxCallDepth = 1000;
    bool enableGC = true;
    bool verboseGC = false;
    bool verboseClass = false;
    bool enableJIT = true;
    bool enableAssertions = true;
    OptLevel optLevel = OptLevel::O1;
};

// ============================================================
// VIRTUAL MACHINE
// ============================================================
class VM {
public:
    VMConfig config;
    Heap heap;
    GarbageCollector gc;
    JITCompiler jit;
    EventLoop eventLoop;
    
    std::map<std::string, ClassInfo*> classes;
    std::map<int32_t, ClassInfo*> classById;
    int32_t nextClassId = 1;
    
    std::vector<ConstantPoolEntry> constantPool;
    std::map<std::string, NativeMethod> nativeMethods;
    
    std::vector<Value> globals;
    std::map<std::string, int> globalNames;
    int nextGlobalIndex = 0;
    
    std::map<std::string, GCObject*> internedStrings;
    
    // Lambda closures
    std::vector<LambdaClosure> lambdaClosures;
    
    // String constant pool
    std::vector<std::string> stringPool;
    
    Frame* currentFrame = nullptr;
    bool running = false;
    GCObject* thrownException = nullptr;
    
    uint64_t instructionsExecuted = 0;
    uint64_t methodCalls = 0;
    uint64_t objectsAllocated = 0;
    std::chrono::high_resolution_clock::time_point startTime;
    
#ifdef USE_SDL
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
#endif

public:
    VM() : gc(heap) {
        heap.initialize({config.initialHeapSize, config.maxHeapSize});
        globals.resize(4096);
        jit.optLevel = config.optLevel;
        registerBuiltinNatives();
    }
    
    ~VM() {
#ifdef USE_SDL
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
#endif
    }
    
    // ========================================
    // CARREGAMENTO
    // ========================================
    ClassInfo* loadClass(const std::string& name);
    ClassInfo* defineClass(const std::string& name, const uint8_t* data, size_t length);
    void initializeClass(ClassInfo* cls);
    
    bool loadBytecodeFile(const std::string& filename);
    bool loadBytecode(const std::vector<int32_t>& code);
    bool loadBytecode(const uint8_t* data, size_t length);
    
    // ========================================
    // EXECUÇÃO
    // ========================================
    void run();
    void runMethod(MethodInfo* method, ClassInfo* cls, const std::vector<Value>& args);
    Value invoke(const std::string& className, const std::string& methodName,
                 const std::vector<Value>& args);
    
    void executeInstruction();
    void executeMethod(Frame* frame);
    
    // ========================================
    // ALOCAÇÃO
    // ========================================
    GCObject* newInstance(ClassInfo* cls);
    GCObject* newArray(int type, int32_t length);
    GCObject* newObjectArray(ClassInfo* elemClass, int32_t length);
    GCObject* newString(const std::string& str);
    GCObject* internString(const std::string& str);
    
    // ========================================
    // EXCEÇÕES
    // ========================================
    void throwException(GCObject* exception);
    void throwException(const std::string& className, const std::string& message);
    bool handleException(Frame* frame);
    
    // ========================================
    // MÉTODOS NATIVOS
    // ========================================
    void registerNative(const std::string& signature, NativeMethod method);
    void registerBuiltinNatives();
    
    // ========================================
    // UTILITÁRIOS
    // ========================================
    ClassInfo* getClass(int32_t classId);
    ClassInfo* getClass(const std::string& name);
    int getGlobalIndex(const std::string& name);
    
    void collectGarbage();
    void printStats();
    
    // ========================================
    // JIT
    // ========================================
    void setOptLevel(OptLevel level) { 
        config.optLevel = level; 
        jit.optLevel = level; 
    }

private:
    std::vector<int32_t> scriptBytecode;
    int scriptPC = 0;
    
    void executeScriptMode();
    
    // Execution stack for script mode
    std::vector<Value> execStack;
    int execSP = 0;
    
    inline void stackPush(const Value& v) { execStack[execSP++] = v; }
    inline Value stackPop() { return execStack[--execSP]; }
    inline Value& stackPeek() { return execStack[execSP - 1]; }
    
    struct InlineCache {
        ClassInfo* cachedClass;
        MethodInfo* cachedMethod;
        int callSite;
    };
    std::vector<InlineCache> inlineCaches;
    
    // Lambda execution
    Value executeLambda(int lambdaIdx, const std::vector<Value>& args);
};

// ============================================================
// IMPLEMENTATION
// ============================================================

inline bool VM::loadBytecodeFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    
    std::vector<int32_t> code;
    int32_t op;
    while (file.read(reinterpret_cast<char*>(&op), sizeof(int32_t))) {
        code.push_back(op);
    }
    
    return loadBytecode(code);
}

inline bool VM::loadBytecode(const std::vector<int32_t>& code) {
    scriptBytecode = code;
    scriptPC = 0;
    
    // JIT optimization is applied per-loop at runtime, not ahead-of-time
    // This preserves jump addresses in the original bytecode
    if (config.enableJIT) {
        jit.detectLoops(scriptBytecode);
    }
    
    return true;
}

inline void VM::run() {
    startTime = std::chrono::high_resolution_clock::now();
    running = true;
    
    if (!scriptBytecode.empty()) {
        executeScriptMode();
    }
    
    // Run event loop if there's pending async work
    if (eventLoop.hasPendingWork()) {
        eventLoop.runFor(5000);  // max 5s
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (config.verboseGC) {
        printStats();
    }
}

inline void VM::executeScriptMode() {
    execStack.resize(16384);
    execSP = 0;
    
    while (running && scriptPC < static_cast<int>(scriptBytecode.size())) {
        // JIT profiling
        if (config.enableJIT) {
            jit.recordExecution(scriptPC);
        }
        executeInstruction();
    }
}

inline void VM::executeInstruction() {
    if (scriptPC >= static_cast<int>(scriptBytecode.size())) {
        running = false;
        return;
    }
    
    int32_t opcode = scriptBytecode[scriptPC++];
    instructionsExecuted++;
    
    switch (opcode) {
        case OP_HALT:
            running = false;
            break;
            
        case OP_NOP:
            break;
            
        // ========== CONSTANTES ==========
        case OP_PUSH_NULL:
            stackPush(Value());
            break;
            
        case OP_PUSH_TRUE:
            stackPush(Value(1));
            break;
            
        case OP_PUSH_FALSE:
            stackPush(Value(0));
            break;
            
        case OP_PUSH_INT:
        case OP_ICONST_M1:
        case OP_ICONST_0:
        case OP_ICONST_1:
        case OP_ICONST_2:
        case OP_ICONST_3:
        case OP_ICONST_4:
        case OP_ICONST_5: {
            int32_t val;
            if (opcode == OP_PUSH_INT) {
                val = scriptBytecode[scriptPC++];
            } else if (opcode == OP_ICONST_M1) {
                val = -1;
            } else {
                val = opcode - OP_ICONST_0;
            }
            stackPush(Value(val));
            break;
        }
        
        case OP_PUSH_LONG: {
            int32_t lo = scriptBytecode[scriptPC++];
            int32_t hi = scriptBytecode[scriptPC++];
            int64_t val = (static_cast<int64_t>(hi) << 32) | (static_cast<uint32_t>(lo));
            stackPush(Value(val));
            break;
        }
        
        case OP_PUSH_FLOAT: {
            int32_t bits = scriptBytecode[scriptPC++];
            float val;
            std::memcpy(&val, &bits, sizeof(float));
            stackPush(Value(val));
            break;
        }
        
        case OP_PUSH_DOUBLE: {
            int32_t lo = scriptBytecode[scriptPC++];
            int32_t hi = scriptBytecode[scriptPC++];
            int64_t bits = (static_cast<int64_t>(hi) << 32) | (static_cast<uint32_t>(lo));
            double val;
            std::memcpy(&val, &bits, sizeof(double));
            stackPush(Value(val));
            break;
        }
        
        case OP_PUSH_STRING: {
            int32_t idx = scriptBytecode[scriptPC++];
            if (idx >= 0 && idx < static_cast<int>(stringPool.size())) {
                GCObject* str = newString(stringPool[idx]);
                stackPush(Value(str));
            } else {
                stackPush(Value());
            }
            break;
        }
        
        // ========== PILHA ==========
        case OP_POP:
            stackPop();
            break;
            
        case OP_DUP:
            stackPush(stackPeek());
            break;
            
        case OP_SWAP: {
            Value a = stackPop();
            Value b = stackPop();
            stackPush(a);
            stackPush(b);
            break;
        }
        
        // ========== ARITMÉTICA INT ==========
        case OP_IADD: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() + b.asInt())); break; }
        case OP_ISUB: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() - b.asInt())); break; }
        case OP_IMUL: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() * b.asInt())); break; }
        case OP_IDIV: { 
            Value b = stackPop(); Value a = stackPop();
            stackPush(Value(b.asInt() != 0 ? a.asInt() / b.asInt() : 0)); 
            break; 
        }
        case OP_IMOD: { 
            Value b = stackPop(); Value a = stackPop();
            stackPush(Value(b.asInt() != 0 ? a.asInt() % b.asInt() : 0)); 
            break; 
        }
        case OP_INEG: { Value a = stackPop(); stackPush(Value(-a.asInt())); break; }
        case OP_IINC: {
            int32_t idx = scriptBytecode[scriptPC++];
            int32_t amount = scriptBytecode[scriptPC++];
            globals[idx] = Value(globals[idx].asInt() + amount);
            break;
        }
        
        // ========== ARITMÉTICA LONG ==========
        case OP_LADD: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.toLong() + b.toLong())); break; }
        case OP_LSUB: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.toLong() - b.toLong())); break; }
        case OP_LMUL: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.toLong() * b.toLong())); break; }
        case OP_LDIV: {
            Value b = stackPop(); Value a = stackPop();
            stackPush(Value(b.toLong() != 0 ? a.toLong() / b.toLong() : (int64_t)0));
            break;
        }
        
        // ========== ARITMÉTICA FLOAT ==========
        case OP_FADD: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asFloat() + b.asFloat())); break; }
        case OP_FSUB: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asFloat() - b.asFloat())); break; }
        case OP_FMUL: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asFloat() * b.asFloat())); break; }
        case OP_FDIV: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asFloat() / b.asFloat())); break; }
        
        // ========== ARITMÉTICA DOUBLE ==========
        case OP_DADD: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.toDouble() + b.toDouble())); break; }
        case OP_DSUB: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.toDouble() - b.toDouble())); break; }
        case OP_DMUL: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.toDouble() * b.toDouble())); break; }
        case OP_DDIV: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.toDouble() / b.toDouble())); break; }
        
        // ========== COMPARAÇÕES ==========
        case OP_IEQ: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() == b.asInt() ? 1 : 0)); break; }
        case OP_INE: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() != b.asInt() ? 1 : 0)); break; }
        case OP_ILT: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() < b.asInt() ? 1 : 0)); break; }
        case OP_ILE: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() <= b.asInt() ? 1 : 0)); break; }
        case OP_IGT: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() > b.asInt() ? 1 : 0)); break; }
        case OP_IGE: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() >= b.asInt() ? 1 : 0)); break; }
        
        // ========== BITWISE ==========
        case OP_IAND: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() & b.asInt())); break; }
        case OP_IOR:  { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() | b.asInt())); break; }
        case OP_IXOR: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() ^ b.asInt())); break; }
        case OP_ISHL: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() << b.asInt())); break; }
        case OP_ISHR: { Value b = stackPop(); Value a = stackPop(); stackPush(Value(a.asInt() >> b.asInt())); break; }
        case OP_IUSHR: { 
            Value b = stackPop(); Value a = stackPop(); 
            stackPush(Value(static_cast<int32_t>(static_cast<uint32_t>(a.asInt()) >> b.asInt()))); 
            break; 
        }
        
        // ========== NOT ==========
        case OP_NOT: {
            Value a = stackPop();
            stackPush(Value(a.asInt() == 0 ? 1 : 0));
            break;
        }
        
        // ========== CONVERSÕES ==========
        case OP_I2L: { stackPush(Value(static_cast<int64_t>(stackPop().asInt()))); break; }
        case OP_I2F: { stackPush(Value(static_cast<float>(stackPop().asInt()))); break; }
        case OP_I2D: { stackPush(Value(static_cast<double>(stackPop().asInt()))); break; }
        case OP_L2I: { stackPush(Value(static_cast<int32_t>(stackPop().toLong()))); break; }
        case OP_F2I: { stackPush(Value(static_cast<int32_t>(stackPop().asFloat()))); break; }
        case OP_D2I: { stackPush(Value(static_cast<int32_t>(stackPop().toDouble()))); break; }
        case OP_F2D: { stackPush(Value(static_cast<double>(stackPop().asFloat()))); break; }
        case OP_D2F: { stackPush(Value(static_cast<float>(stackPop().toDouble()))); break; }
        
        // ========== VARIÁVEIS ==========
        case OP_ILOAD:
        case OP_ALOAD:
        case OP_FLOAD:
        case OP_DLOAD:
        case OP_LLOAD: {
            int index = scriptBytecode[scriptPC++];
            stackPush(globals[index]);
            break;
        }
        
        case OP_ISTORE:
        case OP_ASTORE:
        case OP_FSTORE:
        case OP_DSTORE:
        case OP_LSTORE: {
            int index = scriptBytecode[scriptPC++];
            globals[index] = stackPop();
            break;
        }
        
        case OP_LOAD_GLOBAL: {
            int index = scriptBytecode[scriptPC++];
            stackPush(globals[index]);
            break;
        }
        
        case OP_STORE_GLOBAL: {
            int index = scriptBytecode[scriptPC++];
            globals[index] = stackPop();
            break;
        }
        
        // ========== ARRAYS ==========
        case OP_NEWARRAY: {
            int32_t type = scriptBytecode[scriptPC++];
            Value lengthVal = stackPop();
            GCObject* arr = newArray(type, lengthVal.asInt());
            stackPush(Value(arr));
            break;
        }
        
        case OP_ARRAYLENGTH: {
            Value arrVal = stackPop();
            if (arrVal.asObject()) {
                stackPush(Value(arrVal.asObject()->arrayLength()));
            } else {
                stackPush(Value(0));
            }
            break;
        }
        
        case OP_IALOAD: {
            Value idx = stackPop();
            Value arr = stackPop();
            if (arr.asObject()) {
                stackPush(Value(arr.asObject()->arrayElement<int32_t>(idx.asInt())));
            } else {
                stackPush(Value(0));
            }
            break;
        }
        
        case OP_IASTORE: {
            Value val = stackPop();
            Value idx = stackPop();
            Value arr = stackPop();
            if (arr.asObject()) {
                arr.asObject()->arrayElement<int32_t>(idx.asInt()) = val.asInt();
            }
            break;
        }
        
        // ========== CONTROLE DE FLUXO ==========
        case OP_JMP: {
            int32_t addr = scriptBytecode[scriptPC];
            scriptPC = addr;
            break;
        }
        
        case OP_JZ: {
            int32_t addr = scriptBytecode[scriptPC++];
            Value v = stackPop();
            if (v.asInt() == 0) scriptPC = addr;
            break;
        }
        
        case OP_JNZ: {
            int32_t addr = scriptBytecode[scriptPC++];
            Value v = stackPop();
            if (v.asInt() != 0) scriptPC = addr;
            break;
        }
        
        // ========== CHAMADAS ==========
        case OP_CALL: {
            int32_t argCount = scriptBytecode[scriptPC++];
            methodCalls++;
            // For now, function calls are handled via native methods
            (void)argCount;
            break;
        }
        
        case OP_INVOKE: {
            int32_t argCount = scriptBytecode[scriptPC++];
            methodCalls++;
            (void)argCount;
            break;
        }
        
        case OP_INVOKESPEC: {
            int32_t argCount = scriptBytecode[scriptPC++];
            methodCalls++;
            (void)argCount;
            break;
        }
        
        case OP_RET:
        case OP_IRET:
        case OP_ARET:
            // Return from method - in script mode, just continue
            break;
        
        // ========== OBJETOS ==========
        case OP_NEW: {
            int32_t classIdx = scriptBytecode[scriptPC++];
            (void)classIdx;
            stackPush(Value()); // Push null for now
            break;
        }
        
        case OP_GETFIELD: {
            int32_t fieldIdx = scriptBytecode[scriptPC++];
            stackPop(); // object
            stackPush(Value(0)); // placeholder
            (void)fieldIdx;
            break;
        }
        
        case OP_PUTFIELD: {
            int32_t fieldIdx = scriptBytecode[scriptPC++];
            stackPop(); // value
            stackPop(); // object
            (void)fieldIdx;
            break;
        }
        
        case OP_INSTANCEOF: {
            int32_t classIdx = scriptBytecode[scriptPC++];
            stackPop();
            stackPush(Value(0));
            (void)classIdx;
            break;
        }
        
        case OP_CHECKCAST: {
            int32_t classIdx = scriptBytecode[scriptPC++];
            (void)classIdx;
            break;
        }
        
        // ========== EXCEÇÕES ==========
        case OP_TRY_BEGIN: {
            scriptPC++; // handler offset
            break;
        }
        case OP_TRY_END:
        case OP_CATCH:
        case OP_FINALLY:
            break;
            
        case OP_ATHROW:
            stackPop(); // exception object
            break;
            
        case OP_MONITORENTER:
        case OP_MONITOREXIT:
            stackPop();
            break;
        
        // ========== I/O ==========
        case OP_PRINT: {
            Value v = stackPop();
            switch (v.type) {
                case Value::Type::Int: std::cout << v.asInt() << std::endl; break;
                case Value::Type::Long: std::cout << v.asLong() << std::endl; break;
                case Value::Type::Float: std::cout << v.asFloat() << std::endl; break;
                case Value::Type::Double: std::cout << v.asDouble() << std::endl; break;
                case Value::Type::Object:
                    if (v.asObject()) {
                        if (v.asObject()->header.type == GCObjectType::STRING) {
                            char* str = reinterpret_cast<char*>(v.asObject()->data + sizeof(int32_t));
                            std::cout << str << std::endl;
                        } else {
                            std::cout << "<object@" << v.asObject() << ">" << std::endl;
                        }
                    } else {
                        std::cout << "null" << std::endl;
                    }
                    break;
                default: std::cout << "null" << std::endl; break;
            }
            break;
        }
        
        // ========== KAVA 2.5 - LAMBDA ==========
        case OP_LAMBDA_NEW: {
            int32_t lambdaIdx = scriptBytecode[scriptPC++];
            int32_t paramCount = scriptBytecode[scriptPC++];
            
            LambdaClosure closure;
            closure.codeStart = 0; // Will be set by JMP skip
            closure.paramCount = paramCount;
            
            if (lambdaIdx >= static_cast<int>(lambdaClosures.size())) {
                lambdaClosures.resize(lambdaIdx + 1);
            }
            lambdaClosures[lambdaIdx] = closure;
            
            // Push lambda reference (encoded as int)
            Value lambdaVal;
            lambdaVal.type = Value::Type::Lambda;
            lambdaVal.data.i32 = lambdaIdx;
            stackPush(lambdaVal);
            break;
        }
        
        case OP_LAMBDA_CALL: {
            int32_t argCount = scriptBytecode[scriptPC++];
            // Pop args, then lambda ref
            std::vector<Value> args(argCount);
            for (int i = argCount - 1; i >= 0; i--) {
                args[i] = stackPop();
            }
            Value lambdaRef = stackPop();
            
            if (lambdaRef.isLambda()) {
                Value result = executeLambda(lambdaRef.data.i32, args);
                stackPush(result);
            } else {
                stackPush(Value(0));
            }
            break;
        }
        
        case OP_CAPTURE_LOCAL: {
            int32_t localIdx = scriptBytecode[scriptPC++];
            stackPush(globals[localIdx]);
            break;
        }
        
        case OP_CAPTURE_LOAD: {
            int32_t captureIdx = scriptBytecode[scriptPC++];
            stackPush(Value(captureIdx));
            break;
        }
        
        // ========== KAVA 2.5 - STREAMS ==========
        case OP_STREAM_NEW: {
            // Source array/collection is on stack
            // For now, keep it there for operations
            break;
        }
        
        case OP_STREAM_FILTER:
        case OP_STREAM_MAP:
        case OP_STREAM_FOREACH:
        case OP_STREAM_REDUCE:
        case OP_STREAM_FLATMAP: {
            // Lambda is on stack, source below
            stackPop(); // lambda - consumed
            // Result stays on stack
            break;
        }
        
        case OP_STREAM_COUNT: {
            Value source = stackPop();
            if (source.asObject()) {
                stackPush(Value(source.asObject()->arrayLength()));
            } else {
                stackPush(Value(0));
            }
            break;
        }
        
        case OP_STREAM_SUM: {
            Value source = stackPop();
            if (source.asObject()) {
                int32_t len = source.asObject()->arrayLength();
                int64_t sum = 0;
                for (int32_t i = 0; i < len; i++) {
                    sum += source.asObject()->arrayElement<int32_t>(i);
                }
                stackPush(Value(sum));
            } else {
                stackPush(Value((int64_t)0));
            }
            break;
        }
        
        case OP_STREAM_MIN: {
            Value source = stackPop();
            if (source.asObject() && source.asObject()->arrayLength() > 0) {
                int32_t len = source.asObject()->arrayLength();
                int32_t minVal = source.asObject()->arrayElement<int32_t>(0);
                for (int32_t i = 1; i < len; i++) {
                    int32_t v = source.asObject()->arrayElement<int32_t>(i);
                    if (v < minVal) minVal = v;
                }
                stackPush(Value(minVal));
            } else {
                stackPush(Value(0));
            }
            break;
        }
        
        case OP_STREAM_MAX: {
            Value source = stackPop();
            if (source.asObject() && source.asObject()->arrayLength() > 0) {
                int32_t len = source.asObject()->arrayLength();
                int32_t maxVal = source.asObject()->arrayElement<int32_t>(0);
                for (int32_t i = 1; i < len; i++) {
                    int32_t v = source.asObject()->arrayElement<int32_t>(i);
                    if (v > maxVal) maxVal = v;
                }
                stackPush(Value(maxVal));
            } else {
                stackPush(Value(0));
            }
            break;
        }
        
        case OP_STREAM_COLLECT:
        case OP_STREAM_TOLIST:
        case OP_STREAM_SORT:
        case OP_STREAM_DISTINCT:
        case OP_STREAM_LIMIT:
        case OP_STREAM_SKIP:
        case OP_STREAM_ANYMATCH:
        case OP_STREAM_ALLMATCH:
            break;
        
        // ========== KAVA 2.5 - ASYNC/AWAIT ==========
        case OP_ASYNC_CALL: {
            // Create promise for async function call
            auto promise = eventLoop.createPromise();
            stackPush(Value(promise->promiseId));
            break;
        }
        
        case OP_AWAIT: {
            Value promiseId = stackPop();
            auto promise = eventLoop.getPromise(promiseId.asInt());
            if (promise) {
                // Spin-wait for settlement (simple approach)
                while (!promise->isSettled()) {
                    eventLoop.tick();
                }
                stackPush(Value(static_cast<int32_t>(promise->value)));
            } else {
                stackPush(Value(0));
            }
            break;
        }
        
        case OP_PROMISE_NEW: {
            auto promise = eventLoop.createPromise();
            stackPush(Value(promise->promiseId));
            break;
        }
        
        case OP_PROMISE_RESOLVE: {
            Value val = stackPop();
            Value promiseId = stackPop();
            eventLoop.resolvePromise(promiseId.asInt(), val.toLong());
            break;
        }
        
        case OP_PROMISE_REJECT: {
            stackPop(); // error
            stackPop(); // promise id
            break;
        }
        
        case OP_YIELD: {
            // Yield value from generator
            break;
        }
        
        case OP_EVENT_LOOP_TICK: {
            eventLoop.tick();
            break;
        }
        
        // ========== KAVA 2.5 - PIPE ==========
        case OP_PIPE: {
            // value |> func => func(value)
            Value func = stackPop();
            Value val = stackPop();
            
            if (func.isLambda()) {
                std::vector<Value> args = {val};
                Value result = executeLambda(func.data.i32, args);
                stackPush(result);
            } else {
                stackPush(val);
            }
            break;
        }
        
        // ========== JIT SUPERINSTRUCTIONS ==========
        case SUPER_LOAD_LOAD_ADD: {
            int32_t idx1 = scriptBytecode[scriptPC++];
            int32_t idx2 = scriptBytecode[scriptPC++];
            stackPush(Value(globals[idx1].asInt() + globals[idx2].asInt()));
            break;
        }
        
        case SUPER_LOAD_LOAD_MUL: {
            int32_t idx1 = scriptBytecode[scriptPC++];
            int32_t idx2 = scriptBytecode[scriptPC++];
            stackPush(Value(globals[idx1].asInt() * globals[idx2].asInt()));
            break;
        }
        
        case SUPER_PUSH_STORE: {
            int32_t val = scriptBytecode[scriptPC++];
            int32_t idx = scriptBytecode[scriptPC++];
            globals[idx] = Value(val);
            break;
        }
        
        case SUPER_LOAD_CMP_JZ: {
            int32_t varIdx = scriptBytecode[scriptPC++];
            int32_t cmpVal = scriptBytecode[scriptPC++];
            int32_t cmpOp = scriptBytecode[scriptPC++];
            int32_t target = scriptBytecode[scriptPC++];
            
            int32_t v = globals[varIdx].asInt();
            bool result = false;
            switch (cmpOp) {
                case OP_ILT: result = v < cmpVal; break;
                case OP_IGT: result = v > cmpVal; break;
                case OP_ILE: result = v <= cmpVal; break;
                case OP_IGE: result = v >= cmpVal; break;
                default: break;
            }
            if (!result) scriptPC = target;
            break;
        }
        
#ifdef USE_SDL
        case OP_GFX_INIT: {
            Value h = stackPop(); Value w = stackPop();
            SDL_Init(SDL_INIT_VIDEO);
            window = SDL_CreateWindow("KAVA 2.5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w.asInt(), h.asInt(), 0);
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            break;
        }
        
        case OP_GFX_CLEAR: {
            Value b = stackPop(); Value g = stackPop(); Value r = stackPop();
            SDL_SetRenderDrawColor(renderer, r.asInt(), g.asInt(), b.asInt(), 255);
            SDL_RenderClear(renderer);
            break;
        }
        
        case OP_GFX_DRAW: {
            Value h = stackPop(); Value w = stackPop(); Value y = stackPop(); Value x = stackPop();
            SDL_Rect rect = {x.asInt(), y.asInt(), w.asInt(), h.asInt()};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect);
            SDL_RenderPresent(renderer);
            break;
        }
        
        case OP_GFX_EVENT: {
            SDL_Event event;
            int hasEvent = SDL_PollEvent(&event);
            if (hasEvent && event.type == SDL_QUIT) running = false;
            stackPush(Value(hasEvent != 0 ? 1 : 0));
            break;
        }
#endif
        
        default:
            // Unknown opcode - skip
            break;
    }
}

inline Value VM::executeLambda(int lambdaIdx, const std::vector<Value>& args) {
    if (lambdaIdx < 0 || lambdaIdx >= static_cast<int>(lambdaClosures.size())) {
        return Value(0);
    }
    
    auto& closure = lambdaClosures[lambdaIdx];
    
    // Save VM state
    auto savedGlobals = std::vector<Value>(globals.begin(), globals.begin() + std::min((int)args.size() + 10, (int)globals.size()));
    
    // Set parameters as globals
    for (size_t i = 0; i < args.size(); i++) {
        globals[i] = args[i];
    }
    
    // Execute lambda code
    int savedPC = scriptPC;
    scriptPC = closure.codeStart;
    
    // Execute until RET
    while (running && scriptPC < static_cast<int>(scriptBytecode.size())) {
        int32_t op = scriptBytecode[scriptPC];
        if (op == OP_RET || op == OP_IRET) {
            scriptPC++;
            break;
        }
        executeInstruction();
    }
    
    // Restore state
    scriptPC = savedPC;
    Value result = (execSP > 0) ? stackPop() : Value(0);
    
    // Restore globals
    for (size_t i = 0; i < savedGlobals.size(); i++) {
        globals[i] = savedGlobals[i];
    }
    
    return result;
}

inline GCObject* VM::newInstance(ClassInfo* cls) {
    if (!cls) return nullptr;
    GCObject* obj = heap.allocate(cls->classId, GCObjectType::INSTANCE, cls->instanceSize);
    if (obj) objectsAllocated++;
    if (heap.needsGC() && config.enableGC) collectGarbage();
    return obj;
}

inline GCObject* VM::newArray(int type, int32_t length) {
    GCObjectType objType;
    switch (type) {
        case KAVA_T_BOOLEAN: case KAVA_T_BYTE: objType = GCObjectType::ARRAY_BYTE; break;
        case KAVA_T_CHAR: objType = GCObjectType::ARRAY_CHAR; break;
        case KAVA_T_SHORT: objType = GCObjectType::ARRAY_SHORT; break;
        case KAVA_T_INT: objType = GCObjectType::ARRAY_INT; break;
        case KAVA_T_LONG: objType = GCObjectType::ARRAY_LONG; break;
        case KAVA_T_FLOAT: objType = GCObjectType::ARRAY_FLOAT; break;
        case KAVA_T_DOUBLE: objType = GCObjectType::ARRAY_DOUBLE; break;
        default: objType = GCObjectType::ARRAY_INT; break;
    }
    GCObject* obj = heap.allocateArray(objType, length);
    if (obj) objectsAllocated++;
    return obj;
}

inline GCObject* VM::newString(const std::string& str) {
    GCObject* obj = heap.allocateString(str.c_str(), str.length());
    if (obj) objectsAllocated++;
    return obj;
}

inline GCObject* VM::internString(const std::string& str) {
    auto it = internedStrings.find(str);
    if (it != internedStrings.end()) return it->second;
    GCObject* obj = newString(str);
    if (obj) internedStrings[str] = obj;
    return obj;
}

inline void VM::collectGarbage() {
    gc.setRootScanner([this](GCObject*&) {
        for (auto& g : globals) {
            if (g.isObject() && g.asObject()) gc.addRoot(&g.data.obj);
        }
        for (auto& pair : internedStrings) {
            if (pair.second) gc.addRoot(&pair.second);
        }
    });
    gc.collect();
}

inline void VM::registerBuiltinNatives() {
    registerNative("System.currentTimeMillis", [](VM*, Frame*, const std::vector<Value>&) {
        auto now = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return Value(static_cast<int64_t>(millis));
    });
    
    registerNative("System.nanoTime", [](VM*, Frame*, const std::vector<Value>&) {
        auto now = std::chrono::high_resolution_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        return Value(static_cast<int64_t>(nanos));
    });
    
    registerNative("System.gc", [](VM* vm, Frame*, const std::vector<Value>&) {
        vm->collectGarbage();
        return Value();
    });
    
    registerNative("Math.sqrt", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::sqrt(args[0].toDouble()));
    });
    registerNative("Math.sin", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::sin(args[0].toDouble()));
    });
    registerNative("Math.cos", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::cos(args[0].toDouble()));
    });
    registerNative("Math.pow", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::pow(args[0].toDouble(), args[1].toDouble()));
    });
    registerNative("Math.abs", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::abs(args[0].toDouble()));
    });
    registerNative("Math.log", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::log(args[0].toDouble()));
    });
    
    registerNative("Thread.sleep", [](VM*, Frame*, const std::vector<Value>& args) {
        std::this_thread::sleep_for(std::chrono::milliseconds(args[0].toLong()));
        return Value();
    });
}

inline void VM::registerNative(const std::string& signature, NativeMethod method) {
    nativeMethods[signature] = std::move(method);
}

inline void VM::printStats() {
    std::cout << "\n=== KAVA 2.5 VM Statistics ===" << std::endl;
    std::cout << "Instructions executed: " << instructionsExecuted << std::endl;
    std::cout << "Method calls: " << methodCalls << std::endl;
    std::cout << "Objects allocated: " << objectsAllocated << std::endl;
    std::cout << "Heap used: " << heap.totalUsed() << " bytes" << std::endl;
    std::cout << "GC collections: " << heap.stats.totalCollections << std::endl;
    std::cout << "GC time: " << heap.stats.totalTimeMs << " ms" << std::endl;
    std::cout << "JIT opt level: -O" << static_cast<int>(jit.optLevel) << std::endl;
    std::cout << "Lambda closures: " << lambdaClosures.size() << std::endl;
}

} // namespace Kava

#endif // KAVA_VM_H
