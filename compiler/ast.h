/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.5 - Abstract Syntax Tree Completo
 * Suporte a Java 6 + Lambdas, Streams, Async/Await, Functional Interfaces
 */

#ifndef KAVA_AST_H
#define KAVA_AST_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "types.h"

namespace Kava {

// Forward declarations
class ASTNode;
class Expression;
class Statement;

typedef std::shared_ptr<ASTNode> ASTNodePtr;
typedef std::shared_ptr<Expression> ExprPtr;
typedef std::shared_ptr<Statement> StmtPtr;

// ============================================================
// TIPOS DE NÓS DA AST
// ============================================================
enum class NodeType {
    // Programa e Declarações
    Program,
    PackageDecl,
    ImportDecl,
    ClassDecl,
    InterfaceDecl,
    EnumDecl,
    AnnotationDecl,
    
    // Membros de Classe
    FieldDecl,
    MethodDecl,
    ConstructorDecl,
    StaticBlock,
    InstanceBlock,
    
    // Variáveis
    VarDecl,
    MultiVarDecl,
    
    // Statements
    Block,
    ExprStmt,
    IfStmt,
    WhileStmt,
    DoWhileStmt,
    ForStmt,
    ForEachStmt,
    SwitchStmt,
    CaseClause,
    BreakStmt,
    ContinueStmt,
    ReturnStmt,
    ThrowStmt,
    TryStmt,
    CatchClause,
    FinallyClause,
    SynchronizedStmt,
    AssertStmt,
    LabeledStmt,
    PrintStmt,
    
    // Expressões
    Literal,
    Identifier,
    BinaryExpr,
    UnaryExpr,
    TernaryExpr,
    AssignExpr,
    CompoundAssignExpr,
    CallExpr,
    MethodCallExpr,
    NewExpr,
    NewArrayExpr,
    ArrayAccessExpr,
    MemberExpr,
    ThisExpr,
    SuperExpr,
    CastExpr,
    InstanceOfExpr,
    LambdaExpr,
    MethodRefExpr,
    StreamExpr,
    PipeExpr,
    AwaitExpr,
    AsyncMethodDecl,
    YieldStmt,
    
    // Anotações
    Annotation,
    AnnotationElement,
    
    // Tipos
    TypeRef,
    GenericTypeRef,
    ArrayTypeRef
};

// ============================================================
// NÓ BASE ABSTRATO
// ============================================================
class ASTNode {
public:
    int line = 0;
    int column = 0;
    
    virtual ~ASTNode() = default;
    virtual NodeType getType() const = 0;
    virtual void accept(class ASTVisitor& visitor) = 0;
};

// ============================================================
// EXPRESSÃO BASE
// ============================================================
class Expression : public ASTNode {
public:
    TypePtr resolvedType;  // Tipo resolvido durante análise semântica
    bool isLValue = false;  // Pode aparecer do lado esquerdo de atribuição
};

// ============================================================
// STATEMENT BASE
// ============================================================
class Statement : public ASTNode {
public:
    bool isReachable = true;  // Para análise de fluxo
};

// ============================================================
// REFERÊNCIA DE TIPO (para parsing)
// ============================================================
class TypeRefNode : public ASTNode {
public:
    std::string name;
    std::vector<std::shared_ptr<TypeRefNode>> typeArgs;  // Generics
    int arrayDimensions = 0;
    
    NodeType getType() const override { return NodeType::TypeRef; }
    void accept(ASTVisitor& visitor) override;
};
typedef std::shared_ptr<TypeRefNode> TypeRefPtr;

// ============================================================
// ANOTAÇÃO
// ============================================================
class AnnotationNode : public ASTNode {
public:
    std::string name;
    std::map<std::string, ExprPtr> elements;  // name -> value
    
    NodeType getType() const override { return NodeType::Annotation; }
    void accept(ASTVisitor& visitor) override;
};
typedef std::shared_ptr<AnnotationNode> AnnotationPtr;

// ============================================================
// LITERAIS
// ============================================================
class LiteralExpr : public Expression {
public:
    enum class LitType { 
        Null, Boolean, Int, Long, Float, Double, Char, String, Class 
    };
    
    LitType litType;
    std::string value;
    
    NodeType getType() const override { return NodeType::Literal; }
    void accept(ASTVisitor& visitor) override;
    
    // Helpers
    bool isNull() const { return litType == LitType::Null; }
    bool isNumeric() const { 
        return litType == LitType::Int || litType == LitType::Long ||
               litType == LitType::Float || litType == LitType::Double;
    }
    int64_t getIntValue() const { return std::stoll(value); }
    double getDoubleValue() const { return std::stod(value); }
    bool getBoolValue() const { return value == "true"; }
};

// ============================================================
// IDENTIFICADOR
// ============================================================
class IdentifierExpr : public Expression {
public:
    std::string name;
    
    // Resolução semântica
    enum class Kind { Unknown, LocalVar, Field, StaticField, Parameter, ClassName };
    Kind kind = Kind::Unknown;
    int index = -1;  // Índice na tabela de símbolos
    
    NodeType getType() const override { return NodeType::Identifier; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// THIS E SUPER
// ============================================================
class ThisExpr : public Expression {
public:
    NodeType getType() const override { return NodeType::ThisExpr; }
    void accept(ASTVisitor& visitor) override;
};

class SuperExpr : public Expression {
public:
    NodeType getType() const override { return NodeType::SuperExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// EXPRESSÃO BINÁRIA
// ============================================================
class BinaryExpr : public Expression {
public:
    enum class Op {
        // Aritméticos
        Add, Sub, Mul, Div, Mod,
        // Bitwise
        BitAnd, BitOr, BitXor, LeftShift, RightShift, UnsignedRightShift,
        // Comparação
        Eq, NotEq, Lt, LtEq, Gt, GtEq,
        // Lógicos
        And, Or
    };
    
    Op op;
    ExprPtr left;
    ExprPtr right;
    
    NodeType getType() const override { return NodeType::BinaryExpr; }
    void accept(ASTVisitor& visitor) override;
    
    static std::string opToString(Op op) {
        switch (op) {
            case Op::Add: return "+";
            case Op::Sub: return "-";
            case Op::Mul: return "*";
            case Op::Div: return "/";
            case Op::Mod: return "%";
            case Op::BitAnd: return "&";
            case Op::BitOr: return "|";
            case Op::BitXor: return "^";
            case Op::LeftShift: return "<<";
            case Op::RightShift: return ">>";
            case Op::UnsignedRightShift: return ">>>";
            case Op::Eq: return "==";
            case Op::NotEq: return "!=";
            case Op::Lt: return "<";
            case Op::LtEq: return "<=";
            case Op::Gt: return ">";
            case Op::GtEq: return ">=";
            case Op::And: return "&&";
            case Op::Or: return "||";
        }
        return "?";
    }
};

// ============================================================
// EXPRESSÃO UNÁRIA
// ============================================================
class UnaryExpr : public Expression {
public:
    enum class Op {
        Negate, Not, BitNot, PreInc, PreDec, PostInc, PostDec
    };
    
    Op op;
    ExprPtr operand;
    
    NodeType getType() const override { return NodeType::UnaryExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// EXPRESSÃO TERNÁRIA (condition ? then : else)
// ============================================================
class TernaryExpr : public Expression {
public:
    ExprPtr condition;
    ExprPtr thenExpr;
    ExprPtr elseExpr;
    
    NodeType getType() const override { return NodeType::TernaryExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// ATRIBUIÇÃO
// ============================================================
class AssignExpr : public Expression {
public:
    ExprPtr target;
    ExprPtr value;
    
    NodeType getType() const override { return NodeType::AssignExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// ATRIBUIÇÃO COMPOSTA (+=, -=, etc)
// ============================================================
class CompoundAssignExpr : public Expression {
public:
    BinaryExpr::Op op;
    ExprPtr target;
    ExprPtr value;
    
    NodeType getType() const override { return NodeType::CompoundAssignExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// CHAMADA DE MÉTODO
// ============================================================
class MethodCallExpr : public Expression {
public:
    ExprPtr object;            // Objeto que recebe a chamada (ou null para this)
    std::string methodName;
    std::vector<ExprPtr> arguments;
    std::vector<TypeRefPtr> typeArgs;  // Generics explícitos
    
    // Resolução semântica
    MethodSignature* resolvedMethod = nullptr;
    bool isStaticCall = false;
    bool isSuperCall = false;
    
    NodeType getType() const override { return NodeType::MethodCallExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// NEW (criação de objeto)
// ============================================================
class NewExpr : public Expression {
public:
    TypeRefPtr classType;
    std::vector<ExprPtr> arguments;
    
    // Para anonymous inner classes
    std::vector<StmtPtr> anonymousClassBody;
    
    NodeType getType() const override { return NodeType::NewExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// NEW ARRAY
// ============================================================
class NewArrayExpr : public Expression {
public:
    TypeRefPtr elementType;
    std::vector<ExprPtr> dimensions;    // Tamanhos das dimensões
    std::vector<ExprPtr> initializer;   // Array initializer {...}
    
    NodeType getType() const override { return NodeType::NewArrayExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// ACESSO A ARRAY
// ============================================================
class ArrayAccessExpr : public Expression {
public:
    ExprPtr array;
    ExprPtr index;
    
    ArrayAccessExpr() { isLValue = true; }
    
    NodeType getType() const override { return NodeType::ArrayAccessExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// ACESSO A MEMBRO (objeto.campo)
// ============================================================
class MemberExpr : public Expression {
public:
    ExprPtr object;
    std::string memberName;
    
    // Resolução
    FieldInfo* resolvedField = nullptr;
    
    MemberExpr() { isLValue = true; }
    
    NodeType getType() const override { return NodeType::MemberExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// CAST
// ============================================================
class CastExpr : public Expression {
public:
    TypeRefPtr targetType;
    ExprPtr operand;
    
    NodeType getType() const override { return NodeType::CastExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// PARÂMETRO DE MÉTODO (forward para uso em Lambda)
// ============================================================
struct ParameterDecl {
    std::vector<AnnotationPtr> annotations;
    Modifiers modifiers;
    TypeRefPtr type;
    std::string name;
    bool isVarArgs = false;
};

// ============================================================
// LAMBDA EXPRESSION (KAVA 2.5)
// ============================================================
class LambdaExpr : public Expression {
public:
    std::vector<ParameterDecl> parameters;
    // Body pode ser uma expressao (body simples) ou BlockStmt
    ExprPtr bodyExpr;  // Para (x) -> x * 2
    std::shared_ptr<class BlockStmt> bodyBlock;  // Para (x) -> { ... }
    
    // Tipo da interface funcional inferido
    TypeRefPtr inferredType;
    
    NodeType getType() const override { return NodeType::LambdaExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// METHOD REFERENCE (KAVA 2.5) - Class::method
// ============================================================
class MethodRefExpr : public Expression {
public:
    ExprPtr object;      // null para referencia estatica
    TypeRefPtr classType; // Para Class::method
    std::string methodName;
    
    NodeType getType() const override { return NodeType::MethodRefExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// STREAM EXPRESSION (KAVA 2.5)
// ============================================================
class StreamExpr : public Expression {
public:
    ExprPtr source;  // A colecao/array de origem
    
    struct StreamOp {
        enum class Kind {
            Filter, Map, FlatMap, Reduce, ForEach,
            Collect, Count, Sum, Min, Max,
            Distinct, Sorted, Limit, Skip,
            AnyMatch, AllMatch, NoneMatch,
            FindFirst, FindAny, ToList, ToArray
        };
        Kind kind;
        ExprPtr argument;  // Lambda ou valor
    };
    
    std::vector<StreamOp> operations;
    
    NodeType getType() const override { return NodeType::StreamExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// PIPE EXPRESSION (KAVA 2.5) - value |> func
// ============================================================
class PipeExpr : public Expression {
public:
    ExprPtr left;   // Valor de entrada
    ExprPtr right;  // Funcao a aplicar
    
    NodeType getType() const override { return NodeType::PipeExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// AWAIT EXPRESSION (KAVA 2.5)
// ============================================================
class AwaitExpr : public Expression {
public:
    ExprPtr operand;  // A Promise/Future a esperar
    
    NodeType getType() const override { return NodeType::AwaitExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// YIELD STATEMENT (KAVA 2.5)
// ============================================================
class YieldStmt : public Statement {
public:
    ExprPtr value;
    
    NodeType getType() const override { return NodeType::YieldStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// INSTANCEOF
class InstanceOfExpr : public Expression {
public:
    ExprPtr operand;
    TypeRefPtr checkType;
    
    NodeType getType() const override { return NodeType::InstanceOfExpr; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// DECLARAÇÃO DE VARIÁVEL
// ============================================================
class VarDeclStmt : public Statement {
public:
    std::vector<AnnotationPtr> annotations;
    Modifiers modifiers;
    TypeRefPtr varType;
    std::string name;
    ExprPtr initializer;
    
    // Resolução
    int localIndex = -1;
    
    NodeType getType() const override { return NodeType::VarDecl; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// BLOCO
// ============================================================
class BlockStmt : public Statement {
public:
    std::vector<StmtPtr> statements;
    
    // Escopo
    std::map<std::string, int> localVars;
    
    NodeType getType() const override { return NodeType::Block; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// EXPRESSION STATEMENT
// ============================================================
class ExprStmt : public Statement {
public:
    ExprPtr expression;
    
    NodeType getType() const override { return NodeType::ExprStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// PRINT STATEMENT (extensão KAVA)
// ============================================================
class PrintStmt : public Statement {
public:
    ExprPtr expression;
    
    NodeType getType() const override { return NodeType::PrintStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// IF STATEMENT
// ============================================================
class IfStmt : public Statement {
public:
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;  // Pode ser null
    
    NodeType getType() const override { return NodeType::IfStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// WHILE STATEMENT
// ============================================================
class WhileStmt : public Statement {
public:
    ExprPtr condition;
    StmtPtr body;
    
    NodeType getType() const override { return NodeType::WhileStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// DO-WHILE STATEMENT
// ============================================================
class DoWhileStmt : public Statement {
public:
    StmtPtr body;
    ExprPtr condition;
    
    NodeType getType() const override { return NodeType::DoWhileStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// FOR STATEMENT
// ============================================================
class ForStmt : public Statement {
public:
    std::vector<StmtPtr> init;  // Pode ser VarDecl ou ExprStmt
    ExprPtr condition;           // Pode ser null (loop infinito)
    std::vector<ExprPtr> update;
    StmtPtr body;
    
    NodeType getType() const override { return NodeType::ForStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// FOR-EACH STATEMENT (Java 5+)
// ============================================================
class ForEachStmt : public Statement {
public:
    Modifiers modifiers;
    TypeRefPtr varType;
    std::string varName;
    ExprPtr iterable;
    StmtPtr body;
    
    NodeType getType() const override { return NodeType::ForEachStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// SWITCH STATEMENT
// ============================================================
class CaseClause : public ASTNode {
public:
    std::vector<ExprPtr> labels;  // Vazio = default
    std::vector<StmtPtr> statements;
    
    NodeType getType() const override { return NodeType::CaseClause; }
    void accept(ASTVisitor& visitor) override;
};

class SwitchStmt : public Statement {
public:
    ExprPtr selector;
    std::vector<std::shared_ptr<CaseClause>> cases;
    
    NodeType getType() const override { return NodeType::SwitchStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// BREAK E CONTINUE
// ============================================================
class BreakStmt : public Statement {
public:
    std::string label;  // Para labeled break
    
    NodeType getType() const override { return NodeType::BreakStmt; }
    void accept(ASTVisitor& visitor) override;
};

class ContinueStmt : public Statement {
public:
    std::string label;  // Para labeled continue
    
    NodeType getType() const override { return NodeType::ContinueStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// RETURN STATEMENT
// ============================================================
class ReturnStmt : public Statement {
public:
    ExprPtr value;  // Pode ser null para void
    
    NodeType getType() const override { return NodeType::ReturnStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// THROW STATEMENT
// ============================================================
class ThrowStmt : public Statement {
public:
    ExprPtr exception;
    
    NodeType getType() const override { return NodeType::ThrowStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// TRY-CATCH-FINALLY
// ============================================================
class CatchClause : public ASTNode {
public:
    std::vector<TypeRefPtr> exceptionTypes;  // Multi-catch (Java 7 style)
    std::string varName;
    std::shared_ptr<BlockStmt> body;
    
    NodeType getType() const override { return NodeType::CatchClause; }
    void accept(ASTVisitor& visitor) override;
};

class TryStmt : public Statement {
public:
    std::shared_ptr<BlockStmt> tryBlock;
    std::vector<std::shared_ptr<CatchClause>> catchClauses;
    std::shared_ptr<BlockStmt> finallyBlock;  // Pode ser null
    
    NodeType getType() const override { return NodeType::TryStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// SYNCHRONIZED STATEMENT
// ============================================================
class SynchronizedStmt : public Statement {
public:
    ExprPtr lockObject;
    std::shared_ptr<BlockStmt> body;
    
    NodeType getType() const override { return NodeType::SynchronizedStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// ASSERT STATEMENT
// ============================================================
class AssertStmt : public Statement {
public:
    ExprPtr condition;
    ExprPtr message;  // Opcional
    
    NodeType getType() const override { return NodeType::AssertStmt; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// DECLARAÇÃO DE CAMPO
// ============================================================
class FieldDecl : public ASTNode {
public:
    std::vector<AnnotationPtr> annotations;
    Modifiers modifiers;
    TypeRefPtr fieldType;
    std::string name;
    ExprPtr initializer;
    
    NodeType getType() const override { return NodeType::FieldDecl; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// DECLARAÇÃO DE MÉTODO
// ============================================================
class MethodDecl : public ASTNode {
public:
    std::vector<AnnotationPtr> annotations;
    Modifiers modifiers;
    std::vector<std::string> typeParams;  // Generics
    TypeRefPtr returnType;
    std::string name;
    std::vector<ParameterDecl> parameters;
    std::vector<TypeRefPtr> throwsTypes;
    std::shared_ptr<BlockStmt> body;  // null para abstract/native
    
    NodeType getType() const override { return NodeType::MethodDecl; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// DECLARAÇÃO DE CONSTRUTOR
// ============================================================
class ConstructorDecl : public ASTNode {
public:
    std::vector<AnnotationPtr> annotations;
    Modifiers modifiers;
    std::string name;  // Deve ser igual ao nome da classe
    std::vector<ParameterDecl> parameters;
    std::vector<TypeRefPtr> throwsTypes;
    std::shared_ptr<BlockStmt> body;
    
    // Para chamada de this() ou super()
    bool hasExplicitConstructorCall = false;
    bool callsThis = false;  // this(...) vs super(...)
    std::vector<ExprPtr> constructorArgs;
    
    NodeType getType() const override { return NodeType::ConstructorDecl; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// BLOCO ESTÁTICO E DE INSTÂNCIA
// ============================================================
class StaticBlock : public ASTNode {
public:
    std::shared_ptr<BlockStmt> body;
    
    NodeType getType() const override { return NodeType::StaticBlock; }
    void accept(ASTVisitor& visitor) override;
};

class InstanceBlock : public ASTNode {
public:
    std::shared_ptr<BlockStmt> body;
    
    NodeType getType() const override { return NodeType::InstanceBlock; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// DECLARAÇÃO DE CLASSE
// ============================================================
class ClassDecl : public ASTNode {
public:
    std::vector<AnnotationPtr> annotations;
    Modifiers modifiers;
    std::string name;
    std::vector<std::string> typeParams;  // Generics
    TypeRefPtr superClass;                 // extends
    std::vector<TypeRefPtr> interfaces;   // implements
    
    // Membros
    std::vector<std::shared_ptr<FieldDecl>> fields;
    std::vector<std::shared_ptr<MethodDecl>> methods;
    std::vector<std::shared_ptr<ConstructorDecl>> constructors;
    std::vector<std::shared_ptr<StaticBlock>> staticBlocks;
    std::vector<std::shared_ptr<InstanceBlock>> instanceBlocks;
    
    // Inner classes
    std::vector<std::shared_ptr<ClassDecl>> innerClasses;
    
    NodeType getType() const override { return NodeType::ClassDecl; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// DECLARAÇÃO DE INTERFACE
// ============================================================
class InterfaceDecl : public ASTNode {
public:
    std::vector<AnnotationPtr> annotations;
    Modifiers modifiers;
    std::string name;
    std::vector<std::string> typeParams;
    std::vector<TypeRefPtr> superInterfaces;  // extends
    
    std::vector<std::shared_ptr<FieldDecl>> fields;    // Constantes
    std::vector<std::shared_ptr<MethodDecl>> methods;  // Abstract methods
    
    NodeType getType() const override { return NodeType::InterfaceDecl; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// DECLARAÇÃO DE ENUM
// ============================================================
class EnumDecl : public ASTNode {
public:
    std::vector<AnnotationPtr> annotations;
    Modifiers modifiers;
    std::string name;
    std::vector<TypeRefPtr> interfaces;
    
    // Constantes do enum
    struct EnumConstant {
        std::string name;
        std::vector<ExprPtr> arguments;
        std::vector<std::shared_ptr<MethodDecl>> anonymousMethods;
    };
    std::vector<EnumConstant> constants;
    
    // Membros adicionais
    std::vector<std::shared_ptr<FieldDecl>> fields;
    std::vector<std::shared_ptr<MethodDecl>> methods;
    std::vector<std::shared_ptr<ConstructorDecl>> constructors;
    
    NodeType getType() const override { return NodeType::EnumDecl; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// IMPORT E PACKAGE
// ============================================================
class PackageDecl : public ASTNode {
public:
    std::string name;
    
    NodeType getType() const override { return NodeType::PackageDecl; }
    void accept(ASTVisitor& visitor) override;
};

class ImportDecl : public ASTNode {
public:
    std::string name;
    bool isStatic = false;
    bool isWildcard = false;  // import pkg.*
    
    NodeType getType() const override { return NodeType::ImportDecl; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// PROGRAMA (raiz da AST)
// ============================================================
class Program : public ASTNode {
public:
    std::shared_ptr<PackageDecl> package;
    std::vector<std::shared_ptr<ImportDecl>> imports;
    std::vector<std::shared_ptr<ClassDecl>> classes;
    std::vector<std::shared_ptr<InterfaceDecl>> interfaces;
    std::vector<std::shared_ptr<EnumDecl>> enums;
    
    // Statements de nível superior (para scripts simples)
    std::vector<StmtPtr> statements;
    
    NodeType getType() const override { return NodeType::Program; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================
// VISITOR PATTERN
// ============================================================
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Tipos e Anotações
    virtual void visit(TypeRefNode& node) {}
    virtual void visit(AnnotationNode& node) {}
    
    // Expressões
    virtual void visit(LiteralExpr& node) {}
    virtual void visit(IdentifierExpr& node) {}
    virtual void visit(ThisExpr& node) {}
    virtual void visit(SuperExpr& node) {}
    virtual void visit(BinaryExpr& node) {}
    virtual void visit(UnaryExpr& node) {}
    virtual void visit(TernaryExpr& node) {}
    virtual void visit(AssignExpr& node) {}
    virtual void visit(CompoundAssignExpr& node) {}
    virtual void visit(MethodCallExpr& node) {}
    virtual void visit(NewExpr& node) {}
    virtual void visit(NewArrayExpr& node) {}
    virtual void visit(ArrayAccessExpr& node) {}
    virtual void visit(MemberExpr& node) {}
    virtual void visit(CastExpr& node) {}
    virtual void visit(InstanceOfExpr& node) {}
    virtual void visit(LambdaExpr& node) {}
    virtual void visit(MethodRefExpr& node) {}
    virtual void visit(StreamExpr& node) {}
    virtual void visit(PipeExpr& node) {}
    virtual void visit(AwaitExpr& node) {}
    virtual void visit(YieldStmt& node) {}
    
    // Statements
    virtual void visit(VarDeclStmt& node) {}
    virtual void visit(BlockStmt& node) {}
    virtual void visit(ExprStmt& node) {}
    virtual void visit(PrintStmt& node) {}
    virtual void visit(IfStmt& node) {}
    virtual void visit(WhileStmt& node) {}
    virtual void visit(DoWhileStmt& node) {}
    virtual void visit(ForStmt& node) {}
    virtual void visit(ForEachStmt& node) {}
    virtual void visit(SwitchStmt& node) {}
    virtual void visit(CaseClause& node) {}
    virtual void visit(BreakStmt& node) {}
    virtual void visit(ContinueStmt& node) {}
    virtual void visit(ReturnStmt& node) {}
    virtual void visit(ThrowStmt& node) {}
    virtual void visit(TryStmt& node) {}
    virtual void visit(CatchClause& node) {}
    virtual void visit(SynchronizedStmt& node) {}
    virtual void visit(AssertStmt& node) {}
    
    // Declarações
    virtual void visit(FieldDecl& node) {}
    virtual void visit(MethodDecl& node) {}
    virtual void visit(ConstructorDecl& node) {}
    virtual void visit(StaticBlock& node) {}
    virtual void visit(InstanceBlock& node) {}
    virtual void visit(ClassDecl& node) {}
    virtual void visit(InterfaceDecl& node) {}
    virtual void visit(EnumDecl& node) {}
    virtual void visit(PackageDecl& node) {}
    virtual void visit(ImportDecl& node) {}
    virtual void visit(Program& node) {}
};

// Implementações do accept()
inline void TypeRefNode::accept(ASTVisitor& v) { v.visit(*this); }
inline void AnnotationNode::accept(ASTVisitor& v) { v.visit(*this); }
inline void LiteralExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void IdentifierExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void ThisExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void SuperExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void BinaryExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void UnaryExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void TernaryExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void AssignExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void CompoundAssignExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void MethodCallExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void NewExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void NewArrayExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void ArrayAccessExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void MemberExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void CastExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void InstanceOfExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void LambdaExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void MethodRefExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void StreamExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void PipeExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void AwaitExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void YieldStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void VarDeclStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void BlockStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void ExprStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void PrintStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void IfStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void WhileStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void DoWhileStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void ForStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void ForEachStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void SwitchStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void CaseClause::accept(ASTVisitor& v) { v.visit(*this); }
inline void BreakStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void ContinueStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void ReturnStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void ThrowStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void TryStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void CatchClause::accept(ASTVisitor& v) { v.visit(*this); }
inline void SynchronizedStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void AssertStmt::accept(ASTVisitor& v) { v.visit(*this); }
inline void FieldDecl::accept(ASTVisitor& v) { v.visit(*this); }
inline void MethodDecl::accept(ASTVisitor& v) { v.visit(*this); }
inline void ConstructorDecl::accept(ASTVisitor& v) { v.visit(*this); }
inline void StaticBlock::accept(ASTVisitor& v) { v.visit(*this); }
inline void InstanceBlock::accept(ASTVisitor& v) { v.visit(*this); }
inline void ClassDecl::accept(ASTVisitor& v) { v.visit(*this); }
inline void InterfaceDecl::accept(ASTVisitor& v) { v.visit(*this); }
inline void EnumDecl::accept(ASTVisitor& v) { v.visit(*this); }
inline void PackageDecl::accept(ASTVisitor& v) { v.visit(*this); }
inline void ImportDecl::accept(ASTVisitor& v) { v.visit(*this); }
inline void Program::accept(ASTVisitor& v) { v.visit(*this); }

} // namespace Kava

#endif // KAVA_AST_H
