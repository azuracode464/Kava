/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Parser Completo
 * Gramática completa equivalente ao Java 6
 */

#ifndef KAVA_PARSER_H
#define KAVA_PARSER_H

#include "lexer.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <set>
#include <functional>

namespace Kava {

// ============================================================
// ERRO DE PARSING
// ============================================================
struct ParseError {
    std::string message;
    int line;
    int column;
    
    ParseError(const std::string& msg, int ln, int col)
        : message(msg), line(ln), column(col) {}
};

// ============================================================
// PARSER
// ============================================================
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::shared_ptr<Program> parse();
    
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<ParseError>& getErrors() const { return errors; }

private:
    std::vector<Token> tokens;
    size_t current = 0;
    std::vector<ParseError> errors;
    
    // Para controle de sincronização em caso de erro
    std::set<TokenType> synchronizeSet;
    
    // ========================================
    // HELPERS DE NAVEGAÇÃO
    // ========================================
    Token peek() const;
    Token previous() const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    
    // ========================================
    // TRATAMENTO DE ERROS
    // ========================================
    ParseError error(const Token& token, const std::string& message);
    void synchronize();
    
    // ========================================
    // PARSING DE MODIFICADORES
    // ========================================
    Modifiers parseModifiers();
    std::vector<AnnotationPtr> parseAnnotations();
    AnnotationPtr parseAnnotation();
    
    // ========================================
    // PARSING DE TIPOS
    // ========================================
    TypeRefPtr parseType();
    TypeRefPtr parseTypeWithGenerics();
    std::vector<std::string> parseTypeParameters();
    std::vector<TypeRefPtr> parseTypeArguments();
    
    // ========================================
    // PARSING DE DECLARAÇÕES TOP-LEVEL
    // ========================================
    void parseCompilationUnit(std::shared_ptr<Program>& program);
    std::shared_ptr<PackageDecl> parsePackageDeclaration();
    std::shared_ptr<ImportDecl> parseImportDeclaration();
    std::shared_ptr<ClassDecl> parseClassDeclaration(const Modifiers& mods, 
                                                      const std::vector<AnnotationPtr>& annots);
    std::shared_ptr<InterfaceDecl> parseInterfaceDeclaration(const Modifiers& mods,
                                                              const std::vector<AnnotationPtr>& annots);
    std::shared_ptr<EnumDecl> parseEnumDeclaration(const Modifiers& mods,
                                                    const std::vector<AnnotationPtr>& annots);
    
    // ========================================
    // PARSING DE MEMBROS DE CLASSE
    // ========================================
    void parseClassBody(std::shared_ptr<ClassDecl>& cls);
    void parseInterfaceBody(std::shared_ptr<InterfaceDecl>& iface);
    void parseEnumBody(std::shared_ptr<EnumDecl>& enumDecl);
    
    std::shared_ptr<FieldDecl> parseFieldDeclaration(const Modifiers& mods,
                                                      const std::vector<AnnotationPtr>& annots,
                                                      TypeRefPtr type,
                                                      const std::string& name);
    std::shared_ptr<MethodDecl> parseMethodDeclaration(const Modifiers& mods,
                                                        const std::vector<AnnotationPtr>& annots,
                                                        const std::vector<std::string>& typeParams,
                                                        TypeRefPtr returnType,
                                                        const std::string& name);
    std::shared_ptr<ConstructorDecl> parseConstructorDeclaration(const Modifiers& mods,
                                                                  const std::vector<AnnotationPtr>& annots,
                                                                  const std::string& name);
    
    std::vector<ParameterDecl> parseParameters();
    ParameterDecl parseParameter();
    
    // ========================================
    // PARSING DE STATEMENTS
    // ========================================
    StmtPtr parseStatement();
    StmtPtr parseBlockStatement();
    std::shared_ptr<BlockStmt> parseBlock();
    StmtPtr parseLocalVariableDeclaration();
    StmtPtr parseIfStatement();
    StmtPtr parseWhileStatement();
    StmtPtr parseDoWhileStatement();
    StmtPtr parseForStatement();
    StmtPtr parseSwitchStatement();
    StmtPtr parseReturnStatement();
    StmtPtr parseThrowStatement();
    StmtPtr parseTryStatement();
    StmtPtr parseSynchronizedStatement();
    StmtPtr parseAssertStatement();
    StmtPtr parseBreakStatement();
    StmtPtr parseContinueStatement();
    StmtPtr parsePrintStatement();  // Extensão KAVA
    StmtPtr parseExpressionStatement();
    
    // ========================================
    // PARSING DE EXPRESSÕES
    // ========================================
    ExprPtr parseExpression();
    ExprPtr parseAssignmentExpression();
    ExprPtr parseTernaryExpression();
    ExprPtr parseLogicalOrExpression();
    ExprPtr parseLogicalAndExpression();
    ExprPtr parseBitwiseOrExpression();
    ExprPtr parseBitwiseXorExpression();
    ExprPtr parseBitwiseAndExpression();
    ExprPtr parseEqualityExpression();
    ExprPtr parseRelationalExpression();
    ExprPtr parseShiftExpression();
    ExprPtr parseAdditiveExpression();
    ExprPtr parseMultiplicativeExpression();
    ExprPtr parseUnaryExpression();
    ExprPtr parsePostfixExpression();
    ExprPtr parsePrimaryExpression();
    
    // Expressões específicas
    ExprPtr parseNewExpression();
    ExprPtr parseMethodCall(ExprPtr object, const std::string& name);
    ExprPtr parseArrayAccess(ExprPtr array);
    ExprPtr parseMemberAccess(ExprPtr object);
    std::vector<ExprPtr> parseArguments();
    ExprPtr parseArrayInitializer();
    
    // ========================================
    // HELPERS PARA EXPRESSÕES
    // ========================================
    bool isAssignmentOperator() const;
    BinaryExpr::Op getAssignmentOp() const;
    BinaryExpr::Op getBinaryOp(TokenType type) const;
    UnaryExpr::Op getUnaryOp(TokenType type) const;
};

} // namespace Kava

#endif // KAVA_PARSER_H
