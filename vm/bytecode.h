/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Bytecode Specification
 * Formato de bytecode otimizado, stack-based como JVM
 */

#ifndef KAVA_BYTECODE_H
#define KAVA_BYTECODE_H

#include <stdint.h>

#define KAVA_VERSION_MAJOR 2
#define KAVA_VERSION_MINOR 5
#define KAVA_BYTECODE_MAGIC 0x4B415641  // "KAVA"

// ============================================================
// OPCODES - Organizado por categoria
// ============================================================
typedef enum {
    // ========================================
    // CONSTANTES E PILHA (0x00 - 0x1F)
    // ========================================
    OP_NOP          = 0x00,  // No operation
    OP_HALT         = 0x01,  // Termina execução
    
    // Push de constantes
    OP_PUSH_NULL    = 0x02,  // Push null
    OP_PUSH_TRUE    = 0x03,  // Push true
    OP_PUSH_FALSE   = 0x04,  // Push false
    OP_PUSH_INT     = 0x05,  // Push int32 (seguido por 4 bytes)
    OP_PUSH_LONG    = 0x06,  // Push int64 (seguido por 8 bytes)
    OP_PUSH_FLOAT   = 0x07,  // Push float32 (seguido por 4 bytes)
    OP_PUSH_DOUBLE  = 0x08,  // Push float64 (seguido por 8 bytes)
    OP_PUSH_STRING  = 0x09,  // Push string (index na constant pool)
    OP_PUSH_CLASS   = 0x0A,  // Push class reference
    
    // Constantes compactas (-1, 0, 1, 2, 3, 4, 5)
    OP_ICONST_M1    = 0x0B,  // Push -1
    OP_ICONST_0     = 0x0C,  // Push 0
    OP_ICONST_1     = 0x0D,  // Push 1
    OP_ICONST_2     = 0x0E,  // Push 2
    OP_ICONST_3     = 0x0F,  // Push 3
    OP_ICONST_4     = 0x10,  // Push 4
    OP_ICONST_5     = 0x11,  // Push 5
    
    // Manipulação de pilha
    OP_POP          = 0x12,  // Remove top
    OP_POP2         = 0x13,  // Remove 2 slots (long/double)
    OP_DUP          = 0x14,  // Duplica top
    OP_DUP2         = 0x15,  // Duplica 2 slots
    OP_DUP_X1       = 0x16,  // Duplica top abaixo do segundo
    OP_DUP_X2       = 0x17,  // Duplica top abaixo do terceiro
    OP_SWAP         = 0x18,  // Troca top e segundo
    
    // ========================================
    // ARITMÉTICA INTEIRA (0x20 - 0x3F)
    // ========================================
    OP_IADD         = 0x20,  // int add
    OP_ISUB         = 0x21,  // int subtract
    OP_IMUL         = 0x22,  // int multiply
    OP_IDIV         = 0x23,  // int divide
    OP_IMOD         = 0x24,  // int modulo
    OP_INEG         = 0x25,  // int negate
    OP_IINC         = 0x26,  // Incrementa local (index, amount)
    
    // Long
    OP_LADD         = 0x27,
    OP_LSUB         = 0x28,
    OP_LMUL         = 0x29,
    OP_LDIV         = 0x2A,
    OP_LMOD         = 0x2B,
    OP_LNEG         = 0x2C,
    
    // Float
    OP_FADD         = 0x2D,
    OP_FSUB         = 0x2E,
    OP_FMUL         = 0x2F,
    OP_FDIV         = 0x30,
    OP_FMOD         = 0x31,
    OP_FNEG         = 0x32,
    
    // Double
    OP_DADD         = 0x33,
    OP_DSUB         = 0x34,
    OP_DMUL         = 0x35,
    OP_DDIV         = 0x36,
    OP_DMOD         = 0x37,
    OP_DNEG         = 0x38,
    
    // ========================================
    // BITWISE (0x40 - 0x4F)
    // ========================================
    OP_IAND         = 0x40,  // int and
    OP_IOR          = 0x41,  // int or
    OP_IXOR         = 0x42,  // int xor
    OP_ISHL         = 0x43,  // int shift left
    OP_ISHR         = 0x44,  // int shift right (aritmético)
    OP_IUSHR        = 0x45,  // int shift right (unsigned)
    
    OP_LAND         = 0x46,  // long and
    OP_LOR          = 0x47,  // long or
    OP_LXOR         = 0x48,  // long xor
    OP_LSHL         = 0x49,  // long shift left
    OP_LSHR         = 0x4A,  // long shift right
    OP_LUSHR        = 0x4B,  // long unsigned shift right
    
    // ========================================
    // COMPARAÇÃO (0x50 - 0x5F)
    // ========================================
    OP_ICMP         = 0x50,  // Compara ints: -1, 0, 1
    OP_LCMP         = 0x51,  // Compara longs
    OP_FCMPL        = 0x52,  // Compara floats (NaN = -1)
    OP_FCMPG        = 0x53,  // Compara floats (NaN = 1)
    OP_DCMPL        = 0x54,  // Compara doubles (NaN = -1)
    OP_DCMPG        = 0x55,  // Compara doubles (NaN = 1)
    
    // Comparações booleanas (push resultado)
    OP_IEQ          = 0x56,  // a == b
    OP_INE          = 0x57,  // a != b
    OP_ILT          = 0x58,  // a < b
    OP_IGE          = 0x59,  // a >= b
    OP_IGT          = 0x5A,  // a > b
    OP_ILE          = 0x5B,  // a <= b
    
    // Comparação com referências
    OP_ACMPEQ       = 0x5C,  // refs iguais
    OP_ACMPNE       = 0x5D,  // refs diferentes
    OP_ANULL        = 0x5E,  // ref é null
    OP_ANNULL       = 0x5F,  // ref não é null
    
    // ========================================
    // CONVERSÕES (0x60 - 0x6F)
    // ========================================
    OP_I2L          = 0x60,  // int to long
    OP_I2F          = 0x61,  // int to float
    OP_I2D          = 0x62,  // int to double
    OP_L2I          = 0x63,  // long to int
    OP_L2F          = 0x64,  // long to float
    OP_L2D          = 0x65,  // long to double
    OP_F2I          = 0x66,  // float to int
    OP_F2L          = 0x67,  // float to long
    OP_F2D          = 0x68,  // float to double
    OP_D2I          = 0x69,  // double to int
    OP_D2L          = 0x6A,  // double to long
    OP_D2F          = 0x6B,  // double to float
    OP_I2B          = 0x6C,  // int to byte
    OP_I2C          = 0x6D,  // int to char
    OP_I2S          = 0x6E,  // int to short
    
    // ========================================
    // VARIÁVEIS LOCAIS (0x70 - 0x8F)
    // ========================================
    // Load (push valor)
    OP_ILOAD        = 0x70,  // Load int local (index)
    OP_LLOAD        = 0x71,  // Load long local
    OP_FLOAD        = 0x72,  // Load float local
    OP_DLOAD        = 0x73,  // Load double local
    OP_ALOAD        = 0x74,  // Load reference local
    
    // Load compacto (índices 0-3)
    OP_ILOAD_0      = 0x75,
    OP_ILOAD_1      = 0x76,
    OP_ILOAD_2      = 0x77,
    OP_ILOAD_3      = 0x78,
    OP_ALOAD_0      = 0x79,  // Geralmente 'this'
    OP_ALOAD_1      = 0x7A,
    OP_ALOAD_2      = 0x7B,
    OP_ALOAD_3      = 0x7C,
    
    // Store (pop valor)
    OP_ISTORE       = 0x80,  // Store int local (index)
    OP_LSTORE       = 0x81,  // Store long local
    OP_FSTORE       = 0x82,  // Store float local
    OP_DSTORE       = 0x83,  // Store double local
    OP_ASTORE       = 0x84,  // Store reference local
    
    // Store compacto (índices 0-3)
    OP_ISTORE_0     = 0x85,
    OP_ISTORE_1     = 0x86,
    OP_ISTORE_2     = 0x87,
    OP_ISTORE_3     = 0x88,
    OP_ASTORE_0     = 0x89,
    OP_ASTORE_1     = 0x8A,
    OP_ASTORE_2     = 0x8B,
    OP_ASTORE_3     = 0x8C,
    
    // ========================================
    // CAMPOS E GLOBAIS (0x90 - 0x9F)
    // ========================================
    OP_GETFIELD     = 0x90,  // Get instance field (field_index)
    OP_PUTFIELD     = 0x91,  // Put instance field
    OP_GETSTATIC    = 0x92,  // Get static field
    OP_PUTSTATIC    = 0x93,  // Put static field
    
    OP_LOAD_GLOBAL  = 0x94,  // Load global (legacy)
    OP_STORE_GLOBAL = 0x95,  // Store global (legacy)
    
    // ========================================
    // ARRAYS (0xA0 - 0xAF)
    // ========================================
    OP_NEWARRAY     = 0xA0,  // new primitive array (type, count)
    OP_ANEWARRAY    = 0xA1,  // new reference array (class_index)
    OP_MULTIANEW    = 0xA2,  // new multidimensional array
    OP_ARRAYLENGTH  = 0xA3,  // Get array length
    
    // Load de array
    OP_IALOAD       = 0xA4,  // Load int from array
    OP_LALOAD       = 0xA5,  // Load long from array
    OP_FALOAD       = 0xA6,  // Load float from array
    OP_DALOAD       = 0xA7,  // Load double from array
    OP_AALOAD       = 0xA8,  // Load reference from array
    OP_BALOAD       = 0xA9,  // Load byte/boolean from array
    OP_CALOAD       = 0xAA,  // Load char from array
    OP_SALOAD       = 0xAB,  // Load short from array
    
    // Store em array
    OP_IASTORE      = 0xAC,  // Store int in array
    OP_LASTORE      = 0xAD,  // Store long in array
    OP_FASTORE      = 0xAE,  // Store float in array
    OP_DASTORE      = 0xAF,  // Store double in array
    OP_AASTORE      = 0xB0,  // Store reference in array
    OP_BASTORE      = 0xB1,  // Store byte/boolean in array
    OP_CASTORE      = 0xB2,  // Store char in array
    OP_SASTORE      = 0xB3,  // Store short in array
    
    // ========================================
    // CONTROLE DE FLUXO (0xC0 - 0xCF)
    // ========================================
    OP_JMP          = 0xC0,  // Jump incondicional (offset)
    OP_JZ           = 0xC1,  // Jump if zero/false
    OP_JNZ          = 0xC2,  // Jump if not zero/true
    
    // Jumps condicionais otimizados
    OP_IFEQ         = 0xC3,  // if int == 0
    OP_IFNE         = 0xC4,  // if int != 0
    OP_IFLT         = 0xC5,  // if int < 0
    OP_IFGE         = 0xC6,  // if int >= 0
    OP_IFGT         = 0xC7,  // if int > 0
    OP_IFLE         = 0xC8,  // if int <= 0
    
    // Jumps com comparação
    OP_IF_ICMPEQ    = 0xC9,  // if int == int
    OP_IF_ICMPNE    = 0xCA,  // if int != int
    OP_IF_ICMPLT    = 0xCB,  // if int < int
    OP_IF_ICMPGE    = 0xCC,  // if int >= int
    OP_IF_ICMPGT    = 0xCD,  // if int > int
    OP_IF_ICMPLE    = 0xCE,  // if int <= int
    
    // Switch
    OP_TABLESWITCH  = 0xCF,  // Table switch (dense)
    OP_LOOKUPSWITCH = 0xD0,  // Lookup switch (sparse)
    
    // ========================================
    // MÉTODOS E CHAMADAS (0xD1 - 0xDF)
    // ========================================
    OP_CALL         = 0xD1,  // Chama função/método estático
    OP_INVOKE       = 0xD2,  // Invoca método de instância (virtual)
    OP_INVOKESPEC   = 0xD3,  // Invoca construtor ou super
    OP_INVOKEINTF   = 0xD4,  // Invoca método de interface
    OP_INVOKEDYN    = 0xD5,  // Invoke dynamic (lambdas)
    OP_RET          = 0xD6,  // Return void
    OP_IRET         = 0xD7,  // Return int
    OP_LRET         = 0xD8,  // Return long
    OP_FRET         = 0xD9,  // Return float
    OP_DRET         = 0xDA,  // Return double
    OP_ARET         = 0xDB,  // Return reference
    
    // ========================================
    // OBJETOS (0xE0 - 0xEF)
    // ========================================
    OP_NEW          = 0xE0,  // Cria nova instância (class_index)
    OP_INSTANCEOF   = 0xE1,  // Verifica tipo (class_index)
    OP_CHECKCAST    = 0xE2,  // Cast com verificação
    OP_ATHROW       = 0xE3,  // Lança exceção
    
    // ========================================
    // SINCRONIZAÇÃO (0xF0 - 0xF3)
    // ========================================
    OP_MONITORENTER = 0xF0,  // Entra em monitor (lock)
    OP_MONITOREXIT  = 0xF1,  // Sai de monitor (unlock)
    
    // ========================================
    // EXCEÇÕES (0xF4 - 0xF7)
    // ========================================
    OP_TRY_BEGIN    = 0xF4,  // Marca início de try block
    OP_TRY_END      = 0xF5,  // Marca fim de try block
    OP_CATCH        = 0xF6,  // Handler de exceção
    OP_FINALLY      = 0xF7,  // Handler finally
    
    // ========================================
    // I/O E NATIVOS (0xF8 - 0xFB)
    // ========================================
    OP_PRINT        = 0xF8,  // Print (extensão KAVA)
    OP_PRINTLN      = 0xF9,  // Print com newline
    OP_NATIVE       = 0xFA,  // Chama método nativo
    OP_BREAKPOINT   = 0xFB,  // Breakpoint para debug
    
    // ========================================
    // GRÁFICOS (extensão KAVA)
    // ========================================
    OP_GFX_INIT     = 0xFC,  // Inicializa janela
    OP_GFX_CLEAR    = 0xFD,  // Limpa tela
    OP_GFX_DRAW     = 0xFE,  // Desenha (sub-opcode)
    OP_GFX_EVENT    = 0xFF,  // Poll eventos
    
    // Aliases legados (para compatibilidade)
    OP_ADD = OP_IADD,
    OP_SUB = OP_ISUB,
    OP_MUL = OP_IMUL,
    OP_DIV = OP_IDIV,
    OP_MOD = OP_IMOD,
    OP_EQ  = OP_IEQ,
    OP_LT  = OP_ILT,
    OP_GT  = OP_IGT,
    OP_LTE = OP_ILE,
    OP_GTE = OP_IGE,
    OP_AND = OP_IAND,
    OP_OR  = OP_IOR,
    OP_NOT = 0x19,
    OP_NEQ = OP_INE,
    
    // Aliases para graficos
    OP_GFX_DRAW_RECT = OP_GFX_DRAW,
    OP_GFX_PRESENT = OP_GFX_DRAW,
    OP_GFX_POLL_EVENT = OP_GFX_EVENT,
    
    // ========================================
    // KAVA 2.5 - LAMBDA & FUNCTIONAL (0x100+)
    // ========================================
    OP_LAMBDA_NEW    = 0x100, // Cria closure (func_index, capture_count)
    OP_LAMBDA_CALL   = 0x101, // Invoca lambda (arg_count)
    OP_CAPTURE_LOCAL = 0x102, // Captura variavel local para closure
    OP_CAPTURE_LOAD  = 0x103, // Carrega variavel capturada
    
    // ========================================
    // KAVA 2.5 - STREAMS (0x110+)
    // ========================================
    OP_STREAM_NEW    = 0x110, // Cria stream de array/colecao
    OP_STREAM_FILTER = 0x111, // Filter com lambda
    OP_STREAM_MAP    = 0x112, // Map com lambda
    OP_STREAM_REDUCE = 0x113, // Reduce com lambda
    OP_STREAM_FOREACH= 0x114, // ForEach com lambda
    OP_STREAM_COLLECT= 0x115, // Coleta resultados
    OP_STREAM_COUNT  = 0x116, // Conta elementos
    OP_STREAM_SUM    = 0x117, // Soma elementos
    OP_STREAM_SORT   = 0x118, // Ordena stream
    OP_STREAM_DISTINCT= 0x119,// Remove duplicatas
    OP_STREAM_LIMIT  = 0x11A, // Limita quantidade
    OP_STREAM_SKIP   = 0x11B, // Pula elementos
    OP_STREAM_TOLIST = 0x11C, // Converte para lista
    OP_STREAM_MIN    = 0x11D, // Menor elemento
    OP_STREAM_MAX    = 0x11E, // Maior elemento
    OP_STREAM_FLATMAP= 0x11F, // FlatMap
    OP_STREAM_ANYMATCH = 0x120,
    OP_STREAM_ALLMATCH = 0x121,
    
    // ========================================
    // KAVA 2.5 - ASYNC/AWAIT (0x130+)
    // ========================================
    OP_ASYNC_CALL    = 0x130, // Chama funcao async
    OP_AWAIT         = 0x131, // Suspende ate Promise resolver
    OP_PROMISE_NEW   = 0x132, // Cria nova Promise
    OP_PROMISE_RESOLVE = 0x133, // Resolve Promise
    OP_PROMISE_REJECT = 0x134,  // Rejeita Promise
    OP_YIELD         = 0x135, // Yield em generator
    OP_EVENT_LOOP_TICK = 0x136, // Tick do event loop
    
    // ========================================
    // KAVA 2.5 - PIPE OPERATOR (0x140)
    // ========================================
    OP_PIPE          = 0x140, // Pipe: a |> f  =>  f(a)
    
    // ========================================
    // KAVA 2.5 - JIT HINTS (0x150+)
    // ========================================
    OP_JIT_HOTLOOP   = 0x150, // Marca loop como hot (threshold)
    OP_JIT_HOTFUNC   = 0x151, // Marca funcao como hot
    OP_JIT_DEOPT     = 0x152, // Deoptimize - volta para interpretado
    OP_JIT_OSR       = 0x153  // On-Stack Replacement
} OpCode;

// ============================================================
// TIPOS PRIMITIVOS PARA ARRAYS
// ============================================================
typedef enum {
    KAVA_T_BOOLEAN = 4,
    KAVA_T_CHAR    = 5,
    KAVA_T_FLOAT   = 6,
    KAVA_T_DOUBLE  = 7,
    KAVA_T_BYTE    = 8,
    KAVA_T_SHORT   = 9,
    KAVA_T_INT     = 10,
    KAVA_T_LONG    = 11
} KavaPrimitiveType;

// ============================================================
// CONSTANT POOL TAGS
// ============================================================
typedef enum {
    CONST_UTF8          = 1,
    CONST_INTEGER       = 3,
    CONST_FLOAT         = 4,
    CONST_LONG          = 5,
    CONST_DOUBLE        = 6,
    CONST_CLASS         = 7,
    CONST_STRING        = 8,
    CONST_FIELDREF      = 9,
    CONST_METHODREF     = 10,
    CONST_INTERFACEREF  = 11,
    CONST_NAMEANDTYPE   = 12
} ConstantPoolTag;

// ============================================================
// ESTRUTURAS DO BYTECODE FILE
// ============================================================
#pragma pack(push, 1)

typedef struct {
    uint32_t magic;           // KAVA_BYTECODE_MAGIC
    uint16_t version_major;
    uint16_t version_minor;
    uint16_t constant_pool_count;
    // Seguido por constant_pool entries
} KavaFileHeader;

typedef struct {
    uint16_t access_flags;
    uint16_t this_class;      // Index na constant pool
    uint16_t super_class;     // Index na constant pool
    uint16_t interfaces_count;
    // Seguido por interfaces, fields, methods
} KavaClassInfo;

typedef struct {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
} KavaFieldInfo;

typedef struct {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    // Seguido por Code attribute
} KavaMethodInfo;

typedef struct {
    uint16_t max_stack;
    uint16_t max_locals;
    uint32_t code_length;
    // Seguido por bytecode
} KavaCodeAttribute;

typedef struct {
    uint16_t start_pc;
    uint16_t end_pc;
    uint16_t handler_pc;
    uint16_t catch_type;  // 0 = finally
} KavaExceptionEntry;

#pragma pack(pop)

// ============================================================
// FLAGS DE ACESSO
// ============================================================
#define KAVA_ACC_PUBLIC       0x0001
#define KAVA_ACC_PRIVATE      0x0002
#define KAVA_ACC_PROTECTED    0x0004
#define KAVA_ACC_STATIC       0x0008
#define KAVA_ACC_FINAL        0x0010
#define KAVA_ACC_SYNCHRONIZED 0x0020
#define KAVA_ACC_VOLATILE     0x0040
#define KAVA_ACC_TRANSIENT    0x0080
#define KAVA_ACC_NATIVE       0x0100
#define KAVA_ACC_INTERFACE    0x0200
#define KAVA_ACC_ABSTRACT     0x0400
#define KAVA_ACC_STRICTFP     0x0800
#define KAVA_ACC_SYNTHETIC    0x1000
#define KAVA_ACC_ANNOTATION   0x2000
#define KAVA_ACC_ENUM         0x4000

// ============================================================
// HELPERS PARA NOMES DE OPCODES (para debug)
// ============================================================
#ifdef __cplusplus
inline const char* opcodeName(uint8_t opcode) {
    switch (opcode) {
        case OP_NOP: return "NOP";
        case OP_HALT: return "HALT";
        case OP_PUSH_NULL: return "PUSH_NULL";
        case OP_PUSH_TRUE: return "PUSH_TRUE";
        case OP_PUSH_FALSE: return "PUSH_FALSE";
        case OP_PUSH_INT: return "PUSH_INT";
        case OP_PUSH_LONG: return "PUSH_LONG";
        case OP_PUSH_FLOAT: return "PUSH_FLOAT";
        case OP_PUSH_DOUBLE: return "PUSH_DOUBLE";
        case OP_PUSH_STRING: return "PUSH_STRING";
        case OP_ICONST_M1: return "ICONST_M1";
        case OP_ICONST_0: return "ICONST_0";
        case OP_ICONST_1: return "ICONST_1";
        case OP_ICONST_2: return "ICONST_2";
        case OP_ICONST_3: return "ICONST_3";
        case OP_ICONST_4: return "ICONST_4";
        case OP_ICONST_5: return "ICONST_5";
        case OP_POP: return "POP";
        case OP_DUP: return "DUP";
        case OP_SWAP: return "SWAP";
        case OP_IADD: return "IADD";
        case OP_ISUB: return "ISUB";
        case OP_IMUL: return "IMUL";
        case OP_IDIV: return "IDIV";
        case OP_IMOD: return "IMOD";
        case OP_INEG: return "INEG";
        case OP_IEQ: return "IEQ";
        case OP_INE: return "INE";
        case OP_ILT: return "ILT";
        case OP_IGT: return "IGT";
        case OP_ILE: return "ILE";
        case OP_IGE: return "IGE";
        case OP_ILOAD: return "ILOAD";
        case OP_ISTORE: return "ISTORE";
        case OP_ALOAD: return "ALOAD";
        case OP_ASTORE: return "ASTORE";
        case OP_GETFIELD: return "GETFIELD";
        case OP_PUTFIELD: return "PUTFIELD";
        case OP_GETSTATIC: return "GETSTATIC";
        case OP_PUTSTATIC: return "PUTSTATIC";
        case OP_JMP: return "JMP";
        case OP_JZ: return "JZ";
        case OP_JNZ: return "JNZ";
        case OP_CALL: return "CALL";
        case OP_INVOKE: return "INVOKE";
        case OP_RET: return "RET";
        case OP_IRET: return "IRET";
        case OP_ARET: return "ARET";
        case OP_NEW: return "NEW";
        case OP_NEWARRAY: return "NEWARRAY";
        case OP_ARRAYLENGTH: return "ARRAYLENGTH";
        case OP_INSTANCEOF: return "INSTANCEOF";
        case OP_ATHROW: return "ATHROW";
        case OP_MONITORENTER: return "MONITORENTER";
        case OP_MONITOREXIT: return "MONITOREXIT";
        case OP_PRINT: return "PRINT";
        case OP_NATIVE: return "NATIVE";
        default: return "UNKNOWN";
    }
}
#endif

#endif // KAVA_BYTECODE_H
