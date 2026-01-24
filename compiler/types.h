/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Sistema de Tipos Completo
 * Inspirado no Java 6: Tipagem estática forte, Generics, Interfaces
 */

#ifndef KAVA_TYPES_H
#define KAVA_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <optional>
#include <algorithm>

namespace Kava {

// Forward declarations
class Type;
class ClassType;
class InterfaceType;
class GenericType;
class ArrayType;
class MethodSignature;

using TypePtr = std::shared_ptr<Type>;
using ClassTypePtr = std::shared_ptr<ClassType>;
using InterfaceTypePtr = std::shared_ptr<InterfaceType>;

// ============================================================
// TIPOS PRIMITIVOS
// ============================================================
enum class PrimitiveKind {
    VOID,
    BOOLEAN,
    BYTE,
    CHAR,
    SHORT,
    INT,
    LONG,
    FLOAT,
    DOUBLE
};

// ============================================================
// MODIFICADORES DE ACESSO (igual ao Java 6)
// ============================================================
enum class AccessModifier {
    PUBLIC,
    PROTECTED,
    PRIVATE,
    PACKAGE_PRIVATE  // default (sem modificador)
};

// ============================================================
// MODIFICADORES ADICIONAIS
// ============================================================
struct Modifiers {
    AccessModifier access = AccessModifier::PACKAGE_PRIVATE;
    bool isStatic = false;
    bool isFinal = false;
    bool isAbstract = false;
    bool isNative = false;
    bool isSynchronized = false;
    bool isVolatile = false;
    bool isTransient = false;
    bool isStrictfp = false;
    
    std::string toString() const {
        std::string result;
        switch (access) {
            case AccessModifier::PUBLIC: result += "public "; break;
            case AccessModifier::PROTECTED: result += "protected "; break;
            case AccessModifier::PRIVATE: result += "private "; break;
            default: break;
        }
        if (isStatic) result += "static ";
        if (isFinal) result += "final ";
        if (isAbstract) result += "abstract ";
        if (isNative) result += "native ";
        if (isSynchronized) result += "synchronized ";
        if (isVolatile) result += "volatile ";
        if (isTransient) result += "transient ";
        return result;
    }
};

// ============================================================
// TIPO BASE ABSTRATO
// ============================================================
enum class TypeKind {
    PRIMITIVE,
    CLASS,
    INTERFACE,
    ARRAY,
    GENERIC_PARAM,
    GENERIC_INSTANTIATION,
    NULL_TYPE
};

class Type {
public:
    virtual ~Type() = default;
    virtual TypeKind getKind() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getDescriptor() const = 0;  // JVM-style descriptor
    virtual bool equals(const TypePtr& other) const = 0;
    virtual bool isAssignableFrom(const TypePtr& other) const = 0;
    virtual bool isPrimitive() const { return getKind() == TypeKind::PRIMITIVE; }
    virtual bool isReference() const { return !isPrimitive(); }
    virtual bool isArray() const { return getKind() == TypeKind::ARRAY; }
    virtual int getSize() const = 0;  // Tamanho em slots de stack (4 bytes cada)
};

// ============================================================
// TIPO PRIMITIVO
// ============================================================
class PrimitiveType : public Type {
public:
    PrimitiveKind kind;
    
    explicit PrimitiveType(PrimitiveKind k) : kind(k) {}
    
    TypeKind getKind() const override { return TypeKind::PRIMITIVE; }
    
    std::string getName() const override {
        switch (kind) {
            case PrimitiveKind::VOID: return "void";
            case PrimitiveKind::BOOLEAN: return "boolean";
            case PrimitiveKind::BYTE: return "byte";
            case PrimitiveKind::CHAR: return "char";
            case PrimitiveKind::SHORT: return "short";
            case PrimitiveKind::INT: return "int";
            case PrimitiveKind::LONG: return "long";
            case PrimitiveKind::FLOAT: return "float";
            case PrimitiveKind::DOUBLE: return "double";
        }
        return "unknown";
    }
    
    std::string getDescriptor() const override {
        switch (kind) {
            case PrimitiveKind::VOID: return "V";
            case PrimitiveKind::BOOLEAN: return "Z";
            case PrimitiveKind::BYTE: return "B";
            case PrimitiveKind::CHAR: return "C";
            case PrimitiveKind::SHORT: return "S";
            case PrimitiveKind::INT: return "I";
            case PrimitiveKind::LONG: return "J";
            case PrimitiveKind::FLOAT: return "F";
            case PrimitiveKind::DOUBLE: return "D";
        }
        return "V";
    }
    
    bool equals(const TypePtr& other) const override {
        if (other->getKind() != TypeKind::PRIMITIVE) return false;
        auto p = std::static_pointer_cast<PrimitiveType>(other);
        return kind == p->kind;
    }
    
    bool isAssignableFrom(const TypePtr& other) const override {
        if (!other->isPrimitive()) return false;
        auto p = std::static_pointer_cast<PrimitiveType>(other);
        // Promoção automática de tipos numéricos (igual Java)
        if (kind == p->kind) return true;
        // byte -> short -> int -> long -> float -> double
        // char -> int
        switch (kind) {
            case PrimitiveKind::SHORT:
                return p->kind == PrimitiveKind::BYTE;
            case PrimitiveKind::INT:
                return p->kind == PrimitiveKind::BYTE || 
                       p->kind == PrimitiveKind::SHORT ||
                       p->kind == PrimitiveKind::CHAR;
            case PrimitiveKind::LONG:
                return p->kind == PrimitiveKind::BYTE ||
                       p->kind == PrimitiveKind::SHORT ||
                       p->kind == PrimitiveKind::CHAR ||
                       p->kind == PrimitiveKind::INT;
            case PrimitiveKind::FLOAT:
                return p->kind != PrimitiveKind::DOUBLE &&
                       p->kind != PrimitiveKind::BOOLEAN &&
                       p->kind != PrimitiveKind::VOID;
            case PrimitiveKind::DOUBLE:
                return p->kind != PrimitiveKind::BOOLEAN &&
                       p->kind != PrimitiveKind::VOID;
            default:
                return false;
        }
    }
    
    int getSize() const override {
        switch (kind) {
            case PrimitiveKind::LONG:
            case PrimitiveKind::DOUBLE:
                return 2;  // 64 bits = 2 slots
            case PrimitiveKind::VOID:
                return 0;
            default:
                return 1;  // 32 bits ou menos = 1 slot
        }
    }
};

// ============================================================
// PARÂMETRO DE TIPO GENÉRICO (ex: T, E, K, V)
// ============================================================
class GenericTypeParam : public Type {
public:
    std::string name;              // Nome do parâmetro (T, E, etc)
    TypePtr upperBound;            // extends (null = Object)
    std::vector<TypePtr> bounds;   // bounds adicionais para & intersecção
    
    explicit GenericTypeParam(const std::string& n) : name(n), upperBound(nullptr) {}
    
    TypeKind getKind() const override { return TypeKind::GENERIC_PARAM; }
    std::string getName() const override { return name; }
    std::string getDescriptor() const override { 
        return upperBound ? upperBound->getDescriptor() : "Ljava/lang/Object;"; 
    }
    
    bool equals(const TypePtr& other) const override {
        if (other->getKind() != TypeKind::GENERIC_PARAM) return false;
        auto g = std::static_pointer_cast<GenericTypeParam>(other);
        return name == g->name;
    }
    
    bool isAssignableFrom(const TypePtr& other) const override {
        return true;  // Erasure - aceita qualquer tipo compatível
    }
    
    int getSize() const override { return 1; }
};

// ============================================================
// CAMPO DE CLASSE
// ============================================================
struct FieldInfo {
    std::string name;
    TypePtr type;
    Modifiers modifiers;
    int index;           // Índice no objeto/classe
    bool hasInitializer = false;
    
    std::string getDescriptor() const {
        return type->getDescriptor();
    }
};

// ============================================================
// PARÂMETRO DE MÉTODO
// ============================================================
struct ParameterInfo {
    std::string name;
    TypePtr type;
    bool isFinal = false;
    bool isVarArgs = false;  // Último parâmetro pode ser varargs
};

// ============================================================
// ASSINATURA DE MÉTODO
// ============================================================
class MethodSignature {
public:
    std::string name;
    TypePtr returnType;
    std::vector<ParameterInfo> parameters;
    Modifiers modifiers;
    std::vector<TypePtr> thrownExceptions;  // throws clause
    std::vector<std::shared_ptr<GenericTypeParam>> typeParams;  // Generics do método
    
    // Para métodos nativos
    bool isNative = false;
    std::string nativeBinding;
    
    // Para o bytecode
    int localVarCount = 0;
    int maxStackDepth = 0;
    int codeOffset = 0;
    
    std::string getDescriptor() const {
        std::string desc = "(";
        for (const auto& param : parameters) {
            desc += param.type->getDescriptor();
        }
        desc += ")";
        desc += returnType->getDescriptor();
        return desc;
    }
    
    bool matchesSignature(const std::string& n, const std::vector<TypePtr>& argTypes) const {
        if (name != n) return false;
        if (parameters.size() != argTypes.size()) return false;
        for (size_t i = 0; i < parameters.size(); i++) {
            if (!parameters[i].type->isAssignableFrom(argTypes[i])) {
                return false;
            }
        }
        return true;
    }
};

// ============================================================
// TIPO DE INTERFACE (igual Java 6)
// ============================================================
class InterfaceType : public Type {
public:
    std::string name;
    std::string package;
    std::vector<InterfaceTypePtr> superInterfaces;  // extends
    std::vector<MethodSignature> methods;           // Todos abstract
    std::vector<FieldInfo> fields;                  // Constantes (public static final)
    std::vector<std::shared_ptr<GenericTypeParam>> typeParams;
    Modifiers modifiers;
    
    explicit InterfaceType(const std::string& n) : name(n) {
        modifiers.isAbstract = true;  // Interfaces são sempre abstract
    }
    
    TypeKind getKind() const override { return TypeKind::INTERFACE; }
    std::string getName() const override { return name; }
    
    std::string getFullName() const {
        return package.empty() ? name : package + "." + name;
    }
    
    std::string getDescriptor() const override {
        std::string full = getFullName();
        std::replace(full.begin(), full.end(), '.', '/');
        return "L" + full + ";";
    }
    
    bool equals(const TypePtr& other) const override {
        if (other->getKind() != TypeKind::INTERFACE) return false;
        auto i = std::static_pointer_cast<InterfaceType>(other);
        return getFullName() == i->getFullName();
    }
    
    bool isAssignableFrom(const TypePtr& other) const override;
    
    int getSize() const override { return 1; }  // Referência
    
    void addMethod(const MethodSignature& method) {
        methods.push_back(method);
    }
    
    MethodSignature* findMethod(const std::string& name, const std::vector<TypePtr>& argTypes) {
        for (auto& m : methods) {
            if (m.matchesSignature(name, argTypes)) return &m;
        }
        for (auto& super : superInterfaces) {
            auto* m = super->findMethod(name, argTypes);
            if (m) return m;
        }
        return nullptr;
    }
};

// ============================================================
// TIPO DE CLASSE (igual Java 6)
// ============================================================
class ClassType : public Type {
public:
    std::string name;
    std::string package;
    ClassTypePtr superClass;                        // extends (null = Object)
    std::vector<InterfaceTypePtr> interfaces;       // implements
    std::vector<FieldInfo> fields;
    std::vector<FieldInfo> staticFields;
    std::vector<MethodSignature> methods;
    std::vector<MethodSignature> constructors;
    std::vector<std::shared_ptr<GenericTypeParam>> typeParams;
    Modifiers modifiers;
    
    // Para inner classes (Java 6 feature)
    ClassTypePtr outerClass;
    std::vector<ClassTypePtr> innerClasses;
    
    // Para enum (Java 5+ feature)
    bool isEnum = false;
    std::vector<std::string> enumConstants;
    
    explicit ClassType(const std::string& n) : name(n), superClass(nullptr) {}
    
    TypeKind getKind() const override { return TypeKind::CLASS; }
    std::string getName() const override { return name; }
    
    std::string getFullName() const {
        return package.empty() ? name : package + "." + name;
    }
    
    std::string getDescriptor() const override {
        std::string full = getFullName();
        std::replace(full.begin(), full.end(), '.', '/');
        return "L" + full + ";";
    }
    
    bool equals(const TypePtr& other) const override {
        if (other->getKind() != TypeKind::CLASS) return false;
        auto c = std::static_pointer_cast<ClassType>(other);
        return getFullName() == c->getFullName();
    }
    
    bool isAssignableFrom(const TypePtr& other) const override;
    
    int getSize() const override { return 1; }  // Referência
    
    // Adiciona campo
    void addField(const FieldInfo& field) {
        if (field.modifiers.isStatic) {
            staticFields.push_back(field);
        } else {
            fields.push_back(field);
        }
    }
    
    // Adiciona método
    void addMethod(const MethodSignature& method) {
        methods.push_back(method);
    }
    
    // Adiciona construtor
    void addConstructor(const MethodSignature& ctor) {
        constructors.push_back(ctor);
    }
    
    // Busca campo (inclui herança)
    FieldInfo* findField(const std::string& name) {
        for (auto& f : fields) {
            if (f.name == name) return &f;
        }
        for (auto& f : staticFields) {
            if (f.name == name) return &f;
        }
        if (superClass) {
            return superClass->findField(name);
        }
        return nullptr;
    }
    
    // Busca método (inclui herança e interfaces)
    MethodSignature* findMethod(const std::string& name, const std::vector<TypePtr>& argTypes) {
        for (auto& m : methods) {
            if (m.matchesSignature(name, argTypes)) return &m;
        }
        if (superClass) {
            auto* m = superClass->findMethod(name, argTypes);
            if (m) return m;
        }
        for (auto& iface : interfaces) {
            auto* m = iface->findMethod(name, argTypes);
            if (m) return m;
        }
        return nullptr;
    }
    
    // Verifica se implementa interface
    bool implementsInterface(const InterfaceTypePtr& iface) const {
        for (const auto& i : interfaces) {
            if (i->equals(iface)) return true;
            // Verifica super-interfaces
            for (const auto& si : i->superInterfaces) {
                if (si->equals(iface)) return true;
            }
        }
        if (superClass) {
            return superClass->implementsInterface(iface);
        }
        return false;
    }
    
    // Tamanho da instância em bytes
    int getInstanceSize() const {
        int size = 8;  // Header (classId + GC flags)
        for (const auto& f : fields) {
            size += f.type->getSize() * 4;  // 4 bytes por slot
        }
        return size;
    }
    
    // Verifica se é classe abstrata
    bool isAbstractClass() const {
        return modifiers.isAbstract;
    }
};

// ============================================================
// TIPO ARRAY (igual Java)
// ============================================================
class ArrayType : public Type {
public:
    TypePtr elementType;
    int dimensions;
    
    ArrayType(TypePtr elem, int dims = 1) : elementType(elem), dimensions(dims) {}
    
    TypeKind getKind() const override { return TypeKind::ARRAY; }
    
    std::string getName() const override {
        std::string name = elementType->getName();
        for (int i = 0; i < dimensions; i++) name += "[]";
        return name;
    }
    
    std::string getDescriptor() const override {
        std::string desc;
        for (int i = 0; i < dimensions; i++) desc += "[";
        desc += elementType->getDescriptor();
        return desc;
    }
    
    bool equals(const TypePtr& other) const override {
        if (other->getKind() != TypeKind::ARRAY) return false;
        auto a = std::static_pointer_cast<ArrayType>(other);
        return dimensions == a->dimensions && elementType->equals(a->elementType);
    }
    
    bool isAssignableFrom(const TypePtr& other) const override {
        if (other->getKind() != TypeKind::ARRAY) return false;
        auto a = std::static_pointer_cast<ArrayType>(other);
        if (dimensions != a->dimensions) return false;
        return elementType->isAssignableFrom(a->elementType);
    }
    
    int getSize() const override { return 1; }  // Referência
};

// ============================================================
// TIPO GENÉRICO INSTANCIADO (ex: List<String>)
// ============================================================
class GenericInstantiation : public Type {
public:
    TypePtr rawType;                     // O tipo genérico base
    std::vector<TypePtr> typeArguments;  // Os argumentos de tipo
    
    GenericInstantiation(TypePtr raw, const std::vector<TypePtr>& args)
        : rawType(raw), typeArguments(args) {}
    
    TypeKind getKind() const override { return TypeKind::GENERIC_INSTANTIATION; }
    
    std::string getName() const override {
        std::string name = rawType->getName() + "<";
        for (size_t i = 0; i < typeArguments.size(); i++) {
            if (i > 0) name += ", ";
            name += typeArguments[i]->getName();
        }
        name += ">";
        return name;
    }
    
    std::string getDescriptor() const override {
        return rawType->getDescriptor();  // Erasure - usa o tipo raw
    }
    
    bool equals(const TypePtr& other) const override {
        if (other->getKind() != TypeKind::GENERIC_INSTANTIATION) {
            return rawType->equals(other);  // Erasure
        }
        auto g = std::static_pointer_cast<GenericInstantiation>(other);
        if (!rawType->equals(g->rawType)) return false;
        if (typeArguments.size() != g->typeArguments.size()) return false;
        for (size_t i = 0; i < typeArguments.size(); i++) {
            if (!typeArguments[i]->equals(g->typeArguments[i])) return false;
        }
        return true;
    }
    
    bool isAssignableFrom(const TypePtr& other) const override {
        return rawType->isAssignableFrom(other);
    }
    
    int getSize() const override { return 1; }
};

// ============================================================
// NULL TYPE (para inferência)
// ============================================================
class NullType : public Type {
public:
    TypeKind getKind() const override { return TypeKind::NULL_TYPE; }
    std::string getName() const override { return "null"; }
    std::string getDescriptor() const override { return "Ljava/lang/Object;"; }
    
    bool equals(const TypePtr& other) const override {
        return other->getKind() == TypeKind::NULL_TYPE;
    }
    
    bool isAssignableFrom(const TypePtr& other) const override {
        return other->getKind() == TypeKind::NULL_TYPE;
    }
    
    int getSize() const override { return 1; }
};

// ============================================================
// TYPE REGISTRY - Gerencia todos os tipos
// ============================================================
class TypeRegistry {
public:
    // Tipos primitivos (singletons)
    static TypePtr VOID;
    static TypePtr BOOLEAN;
    static TypePtr BYTE;
    static TypePtr CHAR;
    static TypePtr SHORT;
    static TypePtr INT;
    static TypePtr LONG;
    static TypePtr FLOAT;
    static TypePtr DOUBLE;
    static TypePtr NULL_T;
    
    // Tipos básicos da biblioteca padrão
    static ClassTypePtr OBJECT;
    static ClassTypePtr STRING;
    static ClassTypePtr THROWABLE;
    static ClassTypePtr EXCEPTION;
    static ClassTypePtr RUNTIME_EXCEPTION;
    
    std::map<std::string, ClassTypePtr> classes;
    std::map<std::string, InterfaceTypePtr> interfaces;
    
    static void initialize();
    
    ClassTypePtr getClass(const std::string& name) {
        auto it = classes.find(name);
        return it != classes.end() ? it->second : nullptr;
    }
    
    InterfaceTypePtr getInterface(const std::string& name) {
        auto it = interfaces.find(name);
        return it != interfaces.end() ? it->second : nullptr;
    }
    
    void registerClass(ClassTypePtr cls) {
        classes[cls->getFullName()] = cls;
    }
    
    void registerInterface(InterfaceTypePtr iface) {
        interfaces[iface->getFullName()] = iface;
    }
    
    TypePtr resolveType(const std::string& name) {
        // Primitivos
        if (name == "void") return VOID;
        if (name == "boolean" || name == "bool") return BOOLEAN;
        if (name == "byte") return BYTE;
        if (name == "char") return CHAR;
        if (name == "short") return SHORT;
        if (name == "int") return INT;
        if (name == "long") return LONG;
        if (name == "float") return FLOAT;
        if (name == "double") return DOUBLE;
        
        // Arrays
        if (name.back() == ']') {
            size_t bracket = name.find('[');
            std::string elemName = name.substr(0, bracket);
            int dims = (name.length() - bracket) / 2;
            return std::make_shared<ArrayType>(resolveType(elemName), dims);
        }
        
        // Classes e interfaces
        auto cls = getClass(name);
        if (cls) return cls;
        
        auto iface = getInterface(name);
        if (iface) return iface;
        
        return nullptr;
    }
};

// Implementação de isAssignableFrom para Interface
inline bool InterfaceType::isAssignableFrom(const TypePtr& other) const {
    if (other->getKind() == TypeKind::NULL_TYPE) return true;
    if (other->getKind() == TypeKind::INTERFACE) {
        auto i = std::static_pointer_cast<InterfaceType>(other);
        if (equals(other)) return true;
        for (const auto& super : i->superInterfaces) {
            if (isAssignableFrom(super)) return true;
        }
    }
    if (other->getKind() == TypeKind::CLASS) {
        auto c = std::static_pointer_cast<ClassType>(other);
        // Check by name comparison instead of shared_from_this
        for (const auto& iface : c->interfaces) {
            if (iface->getFullName() == getFullName()) return true;
        }
    }
    return false;
}

// Implementação de isAssignableFrom para Class
inline bool ClassType::isAssignableFrom(const TypePtr& other) const {
    if (other->getKind() == TypeKind::NULL_TYPE) return true;
    if (other->getKind() != TypeKind::CLASS) return false;
    auto c = std::static_pointer_cast<ClassType>(other);
    
    // Mesmo tipo
    if (equals(other)) return true;
    
    // Verifica hierarquia de herança
    ClassTypePtr current = c;
    while (current) {
        if (getFullName() == current->getFullName()) return true;
        current = current->superClass;
    }
    
    return false;
}

} // namespace Kava

#endif // KAVA_TYPES_H
