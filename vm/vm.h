/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Virtual Machine Otimizada
 * VM stack-based inspirada na JVM HotSpot
 */

#ifndef KAVA_VM_H
#define KAVA_VM_H

#include "bytecode.h"
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

#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif

namespace Kava {

// Forward declarations
class VM;
class Frame;
class ClassInfo;
class MethodInfo;

// ============================================================
// VALUE - Valor na pilha/variáveis
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
        Null, Int, Long, Float, Double, Object
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
    
    int32_t asInt() const { return data.i32; }
    int64_t asLong() const { return data.i64; }
    float asFloat() const { return data.f32; }
    double asDouble() const { return data.f64; }
    GCObject* asObject() const { return data.obj; }
    
    // Conversões
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
    
    bool toBool() const {
        return toInt() != 0;
    }
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
        uint16_t indices[2];  // Para refs
    };
    std::string stringValue;
};

// ============================================================
// METHOD INFO
// ============================================================
struct MethodInfo {
    std::string name;
    std::string descriptor;
    uint16_t accessFlags;
    uint16_t maxStack;
    uint16_t maxLocals;
    std::vector<uint8_t> code;
    std::vector<KavaExceptionEntry> exceptionTable;
    int codeOffset;  // Offset no bytecode global
    
    bool isStatic() const { return accessFlags & KAVA_ACC_STATIC; }
    bool isNative() const { return accessFlags & KAVA_ACC_NATIVE; }
    bool isAbstract() const { return accessFlags & KAVA_ACC_ABSTRACT; }
    bool isSynchronized() const { return accessFlags & KAVA_ACC_SYNCHRONIZED; }
};

// ============================================================
// FIELD INFO
// ============================================================
struct FieldInfo {
    std::string name;
    std::string descriptor;
    uint16_t accessFlags;
    int offset;  // Offset no objeto
    Value defaultValue;
    
    bool isStatic() const { return accessFlags & KAVA_ACC_STATIC; }
};

// ============================================================
// CLASS INFO
// ============================================================
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
    
    int instanceSize;  // Tamanho de instância em bytes
    bool initialized;
    
    ClassInfo() : classId(-1), superClassId(-1), instanceSize(0), initialized(false) {}
    
    MethodInfo* findMethod(const std::string& name, const std::string& desc) {
        for (auto& m : methods) {
            if (m.name == name && (desc.empty() || m.descriptor == desc)) {
                return &m;
            }
        }
        return nullptr;
    }
    
    FieldInfo* findField(const std::string& name) {
        for (auto& f : fields) {
            if (f.name == name) return &f;
        }
        for (auto& f : staticFields) {
            if (f.name == name) return &f;
        }
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
    int sp;  // Stack pointer
    Frame* caller;
    
    // Para tratamento de exceções
    GCObject* pendingException;
    
    Frame(MethodInfo* m, ClassInfo* c, Frame* parent = nullptr)
        : method(m), classInfo(c), pc(0), sp(0), caller(parent), pendingException(nullptr) {
        if (m) {
            locals.resize(m->maxLocals);
            operandStack.resize(m->maxStack);
        }
    }
    
    void push(const Value& v) {
        operandStack[sp++] = v;
    }
    
    Value pop() {
        return operandStack[--sp];
    }
    
    Value& peek(int offset = 0) {
        return operandStack[sp - 1 - offset];
    }
    
    void setLocal(int index, const Value& v) {
        locals[index] = v;
    }
    
    Value& getLocal(int index) {
        return locals[index];
    }
};

// ============================================================
// NATIVE METHOD HANDLER
// ============================================================
using NativeMethod = std::function<Value(VM*, Frame*, const std::vector<Value>&)>;

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
    bool enableJIT = false;  // Não implementado ainda
    bool enableAssertions = true;
};

// ============================================================
// VIRTUAL MACHINE
// ============================================================
class VM {
public:
    // Configuração
    VMConfig config;
    
    // Heap e GC
    Heap heap;
    GarbageCollector gc;
    
    // Classes carregadas
    std::map<std::string, ClassInfo*> classes;
    std::map<int32_t, ClassInfo*> classById;
    int32_t nextClassId = 1;
    
    // Constant pool global
    std::vector<ConstantPoolEntry> constantPool;
    
    // Métodos nativos
    std::map<std::string, NativeMethod> nativeMethods;
    
    // Variáveis globais (para modo script)
    std::vector<Value> globals;
    std::map<std::string, int> globalNames;
    int nextGlobalIndex = 0;
    
    // Strings internadas
    std::map<std::string, GCObject*> internedStrings;
    
    // Estado de execução
    Frame* currentFrame = nullptr;
    bool running = false;
    GCObject* thrownException = nullptr;
    
    // Estatísticas
    uint64_t instructionsExecuted = 0;
    uint64_t methodCalls = 0;
    uint64_t objectsAllocated = 0;
    std::chrono::high_resolution_clock::time_point startTime;
    
#ifdef USE_SDL
    // SDL para gráficos
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
#endif

public:
    VM() : gc(heap) {
        heap.initialize({config.initialHeapSize, config.maxHeapSize});
        globals.resize(1024);
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
    // CARREGAMENTO DE CLASSES
    // ========================================
    ClassInfo* loadClass(const std::string& name);
    ClassInfo* defineClass(const std::string& name, const uint8_t* data, size_t length);
    void initializeClass(ClassInfo* cls);
    
    // ========================================
    // CARREGAMENTO DE BYTECODE
    // ========================================
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
    
    // ========================================
    // EXECUÇÃO DE INSTRUÇÕES
    // ========================================
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
    
private:
    // Bytecode para modo script
    std::vector<int32_t> scriptBytecode;
    int scriptPC = 0;
    
    void executeScriptMode();
    
    // Helpers para instruções
    int32_t readInt();
    int64_t readLong();
    float readFloat();
    double readDouble();
    uint16_t readShort();
    uint8_t readByte();
    
    // Inline caching (otimização simples)
    struct InlineCache {
        ClassInfo* cachedClass;
        MethodInfo* cachedMethod;
        int callSite;
    };
    std::vector<InlineCache> inlineCaches;
};

// ============================================================
// IMPLEMENTAÇÃO INLINE
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
    return true;
}

inline void VM::run() {
    startTime = std::chrono::high_resolution_clock::now();
    running = true;
    
    if (!scriptBytecode.empty()) {
        executeScriptMode();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (config.verboseGC) {
        std::cout << "\n=== VM Statistics ===" << std::endl;
        std::cout << "Execution time: " << duration.count() << " ms" << std::endl;
        std::cout << "Instructions: " << instructionsExecuted << std::endl;
        std::cout << "Method calls: " << methodCalls << std::endl;
        std::cout << "Objects allocated: " << objectsAllocated << std::endl;
    }
}

inline void VM::executeScriptMode() {
    while (running && scriptPC < static_cast<int>(scriptBytecode.size())) {
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
    
    // Stack de valores para modo script
    static std::vector<Value> stack(1024);
    static int sp = 0;
    
    auto push = [&](const Value& v) { stack[sp++] = v; };
    auto pop = [&]() -> Value { return stack[--sp]; };
    auto peek = [&]() -> Value& { return stack[sp - 1]; };
    
    switch (opcode) {
        case OP_HALT:
            running = false;
            break;
            
        case OP_NOP:
            break;
            
        // Constantes
        case OP_PUSH_NULL:
            push(Value());
            break;
            
        case OP_PUSH_TRUE:
            push(Value(1));
            break;
            
        case OP_PUSH_FALSE:
            push(Value(0));
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
            } else {
                val = opcode - OP_ICONST_0;
                if (opcode == OP_ICONST_M1) val = -1;
            }
            push(Value(val));
            break;
        }
        
        // Manipulação de pilha
        case OP_POP:
            pop();
            break;
            
        case OP_DUP:
            push(peek());
            break;
            
        case OP_SWAP: {
            Value a = pop();
            Value b = pop();
            push(a);
            push(b);
            break;
        }
        
        // Aritmética inteira
        case OP_IADD: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() + b.asInt()));
            break;
        }
        
        case OP_ISUB: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() - b.asInt()));
            break;
        }
        
        case OP_IMUL: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() * b.asInt()));
            break;
        }
        
        case OP_IDIV: {
            Value b = pop();
            Value a = pop();
            if (b.asInt() != 0) {
                push(Value(a.asInt() / b.asInt()));
            } else {
                push(Value(0));  // TODO: throw ArithmeticException
            }
            break;
        }
        
        case OP_IMOD: {
            Value b = pop();
            Value a = pop();
            if (b.asInt() != 0) {
                push(Value(a.asInt() % b.asInt()));
            } else {
                push(Value(0));
            }
            break;
        }
        
        case OP_INEG: {
            Value a = pop();
            push(Value(-a.asInt()));
            break;
        }
        
        // Comparações
        case OP_IEQ: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() == b.asInt() ? 1 : 0));
            break;
        }
        
        case OP_INE: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() != b.asInt() ? 1 : 0));
            break;
        }
        
        case OP_ILT: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() < b.asInt() ? 1 : 0));
            break;
        }
        
        case OP_ILE: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() <= b.asInt() ? 1 : 0));
            break;
        }
        
        case OP_IGT: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() > b.asInt() ? 1 : 0));
            break;
        }
        
        case OP_IGE: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() >= b.asInt() ? 1 : 0));
            break;
        }
        
        // Bitwise
        case OP_IAND: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() & b.asInt()));
            break;
        }
        
        case OP_IOR: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() | b.asInt()));
            break;
        }
        
        case OP_IXOR: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() ^ b.asInt()));
            break;
        }
        
        case OP_ISHL: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() << b.asInt()));
            break;
        }
        
        case OP_ISHR: {
            Value b = pop();
            Value a = pop();
            push(Value(a.asInt() >> b.asInt()));
            break;
        }
        
        // Variáveis
        case OP_ILOAD:
        case OP_ALOAD: {
            int index = scriptBytecode[scriptPC++];
            push(globals[index]);
            break;
        }
        
        case OP_ISTORE:
        case OP_ASTORE: {
            int index = scriptBytecode[scriptPC++];
            globals[index] = pop();
            break;
        }
        
        case OP_LOAD_GLOBAL: {
            int index = scriptBytecode[scriptPC++];
            push(globals[index]);
            break;
        }
        
        case OP_STORE_GLOBAL: {
            int index = scriptBytecode[scriptPC++];
            globals[index] = pop();
            break;
        }
        
        // Controle de fluxo
        case OP_JMP: {
            int32_t addr = scriptBytecode[scriptPC];
            scriptPC = addr;
            break;
        }
        
        case OP_JZ: {
            int32_t addr = scriptBytecode[scriptPC++];
            Value v = pop();
            if (v.asInt() == 0) {
                scriptPC = addr;
            }
            break;
        }
        
        case OP_JNZ: {
            int32_t addr = scriptBytecode[scriptPC++];
            Value v = pop();
            if (v.asInt() != 0) {
                scriptPC = addr;
            }
            break;
        }
        
        // I/O
        case OP_PRINT: {
            Value v = pop();
            switch (v.type) {
                case Value::Type::Int:
                    std::cout << v.asInt() << std::endl;
                    break;
                case Value::Type::Long:
                    std::cout << v.asLong() << std::endl;
                    break;
                case Value::Type::Float:
                    std::cout << v.asFloat() << std::endl;
                    break;
                case Value::Type::Double:
                    std::cout << v.asDouble() << std::endl;
                    break;
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
                case Value::Type::Null:
                    std::cout << "null" << std::endl;
                    break;
            }
            break;
        }
        
#ifdef USE_SDL
        // Gráficos
        case OP_GFX_INIT: {
            Value h = pop();
            Value w = pop();
            SDL_Init(SDL_INIT_VIDEO);
            window = SDL_CreateWindow("KAVA 2.0", 
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                w.asInt(), h.asInt(), 0);
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            break;
        }
        
        case OP_GFX_CLEAR: {
            Value b = pop();
            Value g = pop();
            Value r = pop();
            SDL_SetRenderDrawColor(renderer, r.asInt(), g.asInt(), b.asInt(), 255);
            SDL_RenderClear(renderer);
            break;
        }
        
        case OP_GFX_DRAW: {
            Value h = pop();
            Value w = pop();
            Value y = pop();
            Value x = pop();
            SDL_Rect rect = {x.asInt(), y.asInt(), w.asInt(), h.asInt()};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect);
            SDL_RenderPresent(renderer);
            break;
        }
        
        case OP_GFX_EVENT: {
            SDL_Event event;
            int hasEvent = SDL_PollEvent(&event);
            if (hasEvent && event.type == SDL_QUIT) {
                running = false;
            }
            push(Value(hasEvent != 0 ? 1 : 0));
            break;
        }
#endif
        
        default:
            std::cerr << "Opcode desconhecido: 0x" << std::hex << opcode 
                      << " no PC: " << std::dec << (scriptPC - 1) << std::endl;
            running = false;
            break;
    }
}

inline GCObject* VM::newInstance(ClassInfo* cls) {
    if (!cls) return nullptr;
    
    GCObject* obj = heap.allocate(cls->classId, GCObjectType::INSTANCE, cls->instanceSize);
    if (obj) {
        objectsAllocated++;
        
        // Inicializa campos com valores default
        for (const auto& field : cls->fields) {
            if (!field.isStatic()) {
                // Valores default já são zero
            }
        }
    }
    
    if (heap.needsGC() && config.enableGC) {
        collectGarbage();
    }
    
    return obj;
}

inline GCObject* VM::newArray(int type, int32_t length) {
    GCObjectType objType;
    switch (type) {
        case KAVA_T_BOOLEAN:
        case KAVA_T_BYTE: objType = GCObjectType::ARRAY_BYTE; break;
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
    if (it != internedStrings.end()) {
        return it->second;
    }
    
    GCObject* obj = newString(str);
    if (obj) {
        internedStrings[str] = obj;
    }
    return obj;
}

inline void VM::collectGarbage() {
    // Registra roots
    gc.setRootScanner([this](GCObject*& obj) {
        // Escaneia globals
        for (auto& g : globals) {
            if (g.isObject() && g.asObject()) {
                gc.addRoot(&g.data.obj);
            }
        }
        
        // Escaneia strings internadas
        for (auto& pair : internedStrings) {
            if (pair.second) {
                gc.addRoot(&pair.second);
            }
        }
    });
    
    gc.collect();
}

inline void VM::registerBuiltinNatives() {
    // System.currentTimeMillis()
    registerNative("System.currentTimeMillis", [](VM*, Frame*, const std::vector<Value>&) {
        auto now = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return Value(static_cast<int64_t>(millis));
    });
    
    // System.nanoTime()
    registerNative("System.nanoTime", [](VM*, Frame*, const std::vector<Value>&) {
        auto now = std::chrono::high_resolution_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        return Value(static_cast<int64_t>(nanos));
    });
    
    // System.gc()
    registerNative("System.gc", [](VM* vm, Frame*, const std::vector<Value>&) {
        vm->collectGarbage();
        return Value();
    });
    
    // Math.sqrt(double)
    registerNative("Math.sqrt", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::sqrt(args[0].toDouble()));
    });
    
    // Math.sin(double)
    registerNative("Math.sin", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::sin(args[0].toDouble()));
    });
    
    // Math.cos(double)
    registerNative("Math.cos", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::cos(args[0].toDouble()));
    });
    
    // Math.pow(double, double)
    registerNative("Math.pow", [](VM*, Frame*, const std::vector<Value>& args) {
        return Value(std::pow(args[0].toDouble(), args[1].toDouble()));
    });
    
    // Thread.sleep(long)
    registerNative("Thread.sleep", [](VM*, Frame*, const std::vector<Value>& args) {
        std::this_thread::sleep_for(std::chrono::milliseconds(args[0].toLong()));
        return Value();
    });
}

inline void VM::registerNative(const std::string& signature, NativeMethod method) {
    nativeMethods[signature] = std::move(method);
}

inline void VM::printStats() {
    std::cout << "\n=== KAVA 2.0 VM Statistics ===" << std::endl;
    std::cout << "Instructions executed: " << instructionsExecuted << std::endl;
    std::cout << "Method calls: " << methodCalls << std::endl;
    std::cout << "Objects allocated: " << objectsAllocated << std::endl;
    std::cout << "Heap used: " << heap.totalUsed() << " bytes" << std::endl;
    std::cout << "GC collections: " << heap.stats.totalCollections << std::endl;
    std::cout << "GC time: " << heap.stats.totalTimeMs << " ms" << std::endl;
}

} // namespace Kava

#endif // KAVA_VM_H
