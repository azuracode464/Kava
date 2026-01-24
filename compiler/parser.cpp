/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Implementação do Parser
 */

#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace Kava {

// ============================================================
// CONSTRUTOR
// ============================================================
Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {
    // Tokens para sincronização após erro
    synchronizeSet = {
        TokenType::CLASS, TokenType::INTERFACE, TokenType::ENUM,
        TokenType::PUBLIC, TokenType::PROTECTED, TokenType::PRIVATE,
        TokenType::STATIC, TokenType::FINAL, TokenType::ABSTRACT,
        TokenType::IF, TokenType::WHILE, TokenType::FOR, TokenType::DO,
        TokenType::SWITCH, TokenType::TRY, TokenType::RETURN,
        TokenType::BREAK, TokenType::CONTINUE, TokenType::THROW,
        TokenType::SYNCHRONIZED, TokenType::SEMICOLON, TokenType::RBRACE
    };
}

// ============================================================
// HELPERS DE NAVEGAÇÃO
// ============================================================
Token Parser::peek() const {
    if (current >= tokens.size()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens[current];
}

Token Parser::previous() const {
    if (current == 0) return tokens[0];
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

// ============================================================
// TRATAMENTO DE ERROS
// ============================================================
ParseError Parser::error(const Token& token, const std::string& message) {
    std::ostringstream oss;
    oss << "Erro de parsing [" << token.line << ":" << token.column << "]: " << message;
    if (token.type != TokenType::EOF_TOKEN) {
        oss << " (encontrado: '" << token.lexeme << "')";
    }
    
    ParseError err(oss.str(), token.line, token.column);
    errors.push_back(err);
    return err;
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        if (synchronizeSet.count(peek().type)) return;
        advance();
    }
}

// ============================================================
// PARSE PRINCIPAL
// ============================================================
std::shared_ptr<Program> Parser::parse() {
    auto program = std::make_shared<Program>();
    
    try {
        parseCompilationUnit(program);
    } catch (const ParseError&) {
        synchronize();
    }
    
    return program;
}

void Parser::parseCompilationUnit(std::shared_ptr<Program>& program) {
    // Package (opcional)
    if (check(TokenType::PACKAGE)) {
        program->package = parsePackageDeclaration();
    }
    
    // Imports
    while (check(TokenType::IMPORT)) {
        program->imports.push_back(parseImportDeclaration());
    }
    
    // Type declarations ou statements
    while (!isAtEnd()) {
        try {
            auto annots = parseAnnotations();
            Modifiers mods = parseModifiers();
            
            if (check(TokenType::CLASS) || check(TokenType::STRUCT)) {
                advance();
                program->classes.push_back(parseClassDeclaration(mods, annots));
            } else if (check(TokenType::INTERFACE)) {
                advance();
                program->interfaces.push_back(parseInterfaceDeclaration(mods, annots));
            } else if (check(TokenType::ENUM)) {
                advance();
                program->enums.push_back(parseEnumDeclaration(mods, annots));
            } else {
                // Statement de nível superior (modo script)
                program->statements.push_back(parseStatement());
            }
        } catch (const ParseError&) {
            synchronize();
        }
    }
}

// ============================================================
// PACKAGE E IMPORT
// ============================================================
std::shared_ptr<PackageDecl> Parser::parsePackageDeclaration() {
    consume(TokenType::PACKAGE, "Esperado 'package'");
    
    auto decl = std::make_shared<PackageDecl>();
    
    // Lê nome qualificado (pkg.subpkg.etc)
    std::string name = consume(TokenType::IDENTIFIER, "Esperado nome do pacote").lexeme;
    while (match(TokenType::DOT)) {
        name += ".";
        name += consume(TokenType::IDENTIFIER, "Esperado nome do pacote").lexeme;
    }
    decl->name = name;
    
    consume(TokenType::SEMICOLON, "Esperado ';' após declaração de pacote");
    return decl;
}

std::shared_ptr<ImportDecl> Parser::parseImportDeclaration() {
    consume(TokenType::IMPORT, "Esperado 'import'");
    
    auto decl = std::make_shared<ImportDecl>();
    
    // import static?
    decl->isStatic = match(TokenType::STATIC);
    
    // Nome qualificado
    std::string name = consume(TokenType::IDENTIFIER, "Esperado nome do import").lexeme;
    while (match(TokenType::DOT)) {
        if (match(TokenType::STAR)) {
            decl->isWildcard = true;
            break;
        }
        name += ".";
        name += consume(TokenType::IDENTIFIER, "Esperado nome do import").lexeme;
    }
    decl->name = name;
    
    consume(TokenType::SEMICOLON, "Esperado ';' após import");
    return decl;
}

// ============================================================
// ANOTAÇÕES
// ============================================================
std::vector<AnnotationPtr> Parser::parseAnnotations() {
    std::vector<AnnotationPtr> annotations;
    
    while (check(TokenType::AT)) {
        annotations.push_back(parseAnnotation());
    }
    
    return annotations;
}

AnnotationPtr Parser::parseAnnotation() {
    consume(TokenType::AT, "Esperado '@'");
    
    auto annot = std::make_shared<AnnotationNode>();
    annot->name = consume(TokenType::IDENTIFIER, "Esperado nome da anotação").lexeme;
    
    // Elementos opcionais
    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            // Pode ser value = expr ou apenas expr (value implícito)
            if (check(TokenType::IDENTIFIER) && 
                tokens.size() > current + 1 && 
                tokens[current + 1].type == TokenType::ASSIGN) {
                // Elementos nomeados
                do {
                    std::string elemName = consume(TokenType::IDENTIFIER, "Esperado nome do elemento").lexeme;
                    consume(TokenType::ASSIGN, "Esperado '='");
                    annot->elements[elemName] = parseExpression();
                } while (match(TokenType::COMMA));
            } else {
                // Valor único (value implícito)
                annot->elements["value"] = parseExpression();
            }
        }
        consume(TokenType::RPAREN, "Esperado ')' após anotação");
    }
    
    return annot;
}

// ============================================================
// MODIFICADORES
// ============================================================
Modifiers Parser::parseModifiers() {
    Modifiers mods;
    
    while (true) {
        if (match(TokenType::PUBLIC)) {
            mods.access = AccessModifier::PUBLIC;
        } else if (match(TokenType::PROTECTED)) {
            mods.access = AccessModifier::PROTECTED;
        } else if (match(TokenType::PRIVATE)) {
            mods.access = AccessModifier::PRIVATE;
        } else if (match(TokenType::STATIC)) {
            mods.isStatic = true;
        } else if (match(TokenType::FINAL)) {
            mods.isFinal = true;
        } else if (match(TokenType::ABSTRACT)) {
            mods.isAbstract = true;
        } else if (match(TokenType::NATIVE)) {
            mods.isNative = true;
        } else if (match(TokenType::SYNCHRONIZED)) {
            mods.isSynchronized = true;
        } else if (match(TokenType::VOLATILE)) {
            mods.isVolatile = true;
        } else if (match(TokenType::TRANSIENT)) {
            mods.isTransient = true;
        } else if (match(TokenType::STRICTFP)) {
            mods.isStrictfp = true;
        } else {
            break;
        }
    }
    
    return mods;
}

// ============================================================
// TIPOS
// ============================================================
TypeRefPtr Parser::parseType() {
    auto typeRef = std::make_shared<TypeRefNode>();
    
    // Tipo primitivo ou nome de classe
    if (match({TokenType::VOID, TokenType::BOOLEAN, TokenType::BYTE, 
               TokenType::CHAR, TokenType::SHORT, TokenType::INT,
               TokenType::LONG, TokenType::FLOAT, TokenType::DOUBLE})) {
        typeRef->name = previous().lexeme;
    } else {
        // Nome qualificado
        typeRef->name = consume(TokenType::IDENTIFIER, "Esperado nome do tipo").lexeme;
        while (match(TokenType::DOT)) {
            typeRef->name += ".";
            typeRef->name += consume(TokenType::IDENTIFIER, "Esperado nome do tipo").lexeme;
        }
        
        // Type arguments (generics)
        if (match(TokenType::LT)) {
            typeRef->typeArgs = parseTypeArguments();
        }
    }
    
    // Array dimensions
    while (match(TokenType::LBRACKET)) {
        consume(TokenType::RBRACKET, "Esperado ']'");
        typeRef->arrayDimensions++;
    }
    
    return typeRef;
}

std::vector<std::string> Parser::parseTypeParameters() {
    std::vector<std::string> params;
    
    consume(TokenType::LT, "Esperado '<'");
    
    do {
        std::string name = consume(TokenType::IDENTIFIER, "Esperado nome do parâmetro de tipo").lexeme;
        
        // extends bound
        if (match(TokenType::EXTENDS)) {
            parseType();  // Bound (ignorado por ora)
            while (match(TokenType::AMPERSAND)) {
                parseType();  // Additional bounds
            }
        }
        
        params.push_back(name);
    } while (match(TokenType::COMMA));
    
    consume(TokenType::GT, "Esperado '>'");
    
    return params;
}

std::vector<TypeRefPtr> Parser::parseTypeArguments() {
    std::vector<TypeRefPtr> args;
    
    do {
        if (match(TokenType::QUESTION)) {
            // Wildcard
            auto wildcard = std::make_shared<TypeRefNode>();
            wildcard->name = "?";
            
            if (match(TokenType::EXTENDS)) {
                wildcard->typeArgs.push_back(parseType());
            } else if (match(TokenType::SUPER)) {
                wildcard->typeArgs.push_back(parseType());
            }
            
            args.push_back(wildcard);
        } else {
            args.push_back(parseType());
        }
    } while (match(TokenType::COMMA));
    
    consume(TokenType::GT, "Esperado '>'");
    
    return args;
}

// ============================================================
// DECLARAÇÃO DE CLASSE
// ============================================================
std::shared_ptr<ClassDecl> Parser::parseClassDeclaration(
    const Modifiers& mods, const std::vector<AnnotationPtr>& annots) {
    
    auto cls = std::make_shared<ClassDecl>();
    cls->modifiers = mods;
    cls->annotations = annots;
    cls->name = consume(TokenType::IDENTIFIER, "Esperado nome da classe").lexeme;
    
    // Type parameters
    if (check(TokenType::LT)) {
        cls->typeParams = parseTypeParameters();
    }
    
    // extends
    if (match(TokenType::EXTENDS)) {
        cls->superClass = parseType();
    }
    
    // implements
    if (match(TokenType::IMPLEMENTS)) {
        do {
            cls->interfaces.push_back(parseType());
        } while (match(TokenType::COMMA));
    }
    
    // Body
    parseClassBody(cls);
    
    return cls;
}

void Parser::parseClassBody(std::shared_ptr<ClassDecl>& cls) {
    consume(TokenType::LBRACE, "Esperado '{' para corpo da classe");
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        try {
            // Static block
            if (check(TokenType::STATIC) && 
                tokens.size() > current + 1 && 
                tokens[current + 1].type == TokenType::LBRACE) {
                advance();  // static
                auto staticBlock = std::make_shared<StaticBlock>();
                staticBlock->body = parseBlock();
                cls->staticBlocks.push_back(staticBlock);
                continue;
            }
            
            // Instance block
            if (check(TokenType::LBRACE)) {
                auto instBlock = std::make_shared<InstanceBlock>();
                instBlock->body = parseBlock();
                cls->instanceBlocks.push_back(instBlock);
                continue;
            }
            
            auto annots = parseAnnotations();
            Modifiers mods = parseModifiers();
            
            // Type parameters para método
            std::vector<std::string> typeParams;
            if (check(TokenType::LT)) {
                typeParams = parseTypeParameters();
            }
            
            // Construtor (mesmo nome que a classe)
            if (check(TokenType::IDENTIFIER) && 
                peek().lexeme == cls->name &&
                tokens.size() > current + 1 &&
                tokens[current + 1].type == TokenType::LPAREN) {
                cls->constructors.push_back(parseConstructorDeclaration(mods, annots, cls->name));
                continue;
            }
            
            // Método ou campo
            TypeRefPtr type = parseType();
            std::string name = consume(TokenType::IDENTIFIER, "Esperado nome do membro").lexeme;
            
            if (check(TokenType::LPAREN)) {
                // Método
                cls->methods.push_back(parseMethodDeclaration(mods, annots, typeParams, type, name));
            } else {
                // Campo
                cls->fields.push_back(parseFieldDeclaration(mods, annots, type, name));
            }
            
        } catch (const ParseError&) {
            synchronize();
        }
    }
    
    consume(TokenType::RBRACE, "Esperado '}' ao fim da classe");
}

// ============================================================
// DECLARAÇÃO DE INTERFACE
// ============================================================
std::shared_ptr<InterfaceDecl> Parser::parseInterfaceDeclaration(
    const Modifiers& mods, const std::vector<AnnotationPtr>& annots) {
    
    auto iface = std::make_shared<InterfaceDecl>();
    iface->modifiers = mods;
    iface->modifiers.isAbstract = true;
    iface->annotations = annots;
    iface->name = consume(TokenType::IDENTIFIER, "Esperado nome da interface").lexeme;
    
    // Type parameters
    if (check(TokenType::LT)) {
        iface->typeParams = parseTypeParameters();
    }
    
    // extends (interfaces podem estender múltiplas)
    if (match(TokenType::EXTENDS)) {
        do {
            iface->superInterfaces.push_back(parseType());
        } while (match(TokenType::COMMA));
    }
    
    parseInterfaceBody(iface);
    
    return iface;
}

void Parser::parseInterfaceBody(std::shared_ptr<InterfaceDecl>& iface) {
    consume(TokenType::LBRACE, "Esperado '{' para corpo da interface");
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        try {
            auto annots = parseAnnotations();
            Modifiers mods = parseModifiers();
            
            // Em interfaces, tudo é implicitamente public abstract (para métodos)
            // e public static final (para campos)
            mods.access = AccessModifier::PUBLIC;
            
            std::vector<std::string> typeParams;
            if (check(TokenType::LT)) {
                typeParams = parseTypeParameters();
            }
            
            TypeRefPtr type = parseType();
            std::string name = consume(TokenType::IDENTIFIER, "Esperado nome do membro").lexeme;
            
            if (check(TokenType::LPAREN)) {
                mods.isAbstract = true;
                auto method = parseMethodDeclaration(mods, annots, typeParams, type, name);
                method->body = nullptr;  // Abstract
                iface->methods.push_back(method);
            } else {
                mods.isStatic = true;
                mods.isFinal = true;
                iface->fields.push_back(parseFieldDeclaration(mods, annots, type, name));
            }
            
        } catch (const ParseError&) {
            synchronize();
        }
    }
    
    consume(TokenType::RBRACE, "Esperado '}' ao fim da interface");
}

// ============================================================
// DECLARAÇÃO DE ENUM
// ============================================================
std::shared_ptr<EnumDecl> Parser::parseEnumDeclaration(
    const Modifiers& mods, const std::vector<AnnotationPtr>& annots) {
    
    auto enumDecl = std::make_shared<EnumDecl>();
    enumDecl->modifiers = mods;
    enumDecl->annotations = annots;
    enumDecl->name = consume(TokenType::IDENTIFIER, "Esperado nome do enum").lexeme;
    
    // implements
    if (match(TokenType::IMPLEMENTS)) {
        do {
            enumDecl->interfaces.push_back(parseType());
        } while (match(TokenType::COMMA));
    }
    
    parseEnumBody(enumDecl);
    
    return enumDecl;
}

void Parser::parseEnumBody(std::shared_ptr<EnumDecl>& enumDecl) {
    consume(TokenType::LBRACE, "Esperado '{' para corpo do enum");
    
    // Constantes do enum
    if (!check(TokenType::RBRACE) && !check(TokenType::SEMICOLON)) {
        do {
            EnumDecl::EnumConstant constant;
            constant.name = consume(TokenType::IDENTIFIER, "Esperado nome da constante").lexeme;
            
            // Argumentos opcionais
            if (match(TokenType::LPAREN)) {
                if (!check(TokenType::RPAREN)) {
                    do {
                        constant.arguments.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }
                consume(TokenType::RPAREN, "Esperado ')'");
            }
            
            enumDecl->constants.push_back(constant);
        } while (match(TokenType::COMMA));
    }
    
    // Membros adicionais (após ;)
    if (match(TokenType::SEMICOLON)) {
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            try {
                auto annots = parseAnnotations();
                Modifiers mods = parseModifiers();
                
                std::vector<std::string> typeParams;
                if (check(TokenType::LT)) {
                    typeParams = parseTypeParameters();
                }
                
                // Construtor
                if (check(TokenType::IDENTIFIER) && peek().lexeme == enumDecl->name &&
                    tokens.size() > current + 1 && tokens[current + 1].type == TokenType::LPAREN) {
                    mods.access = AccessModifier::PRIVATE;  // Construtores de enum são sempre private
                    enumDecl->constructors.push_back(parseConstructorDeclaration(mods, annots, enumDecl->name));
                    continue;
                }
                
                TypeRefPtr type = parseType();
                std::string name = consume(TokenType::IDENTIFIER, "Esperado nome do membro").lexeme;
                
                if (check(TokenType::LPAREN)) {
                    enumDecl->methods.push_back(parseMethodDeclaration(mods, annots, typeParams, type, name));
                } else {
                    enumDecl->fields.push_back(parseFieldDeclaration(mods, annots, type, name));
                }
                
            } catch (const ParseError&) {
                synchronize();
            }
        }
    }
    
    consume(TokenType::RBRACE, "Esperado '}' ao fim do enum");
}

// ============================================================
// CAMPOS E MÉTODOS
// ============================================================
std::shared_ptr<FieldDecl> Parser::parseFieldDeclaration(
    const Modifiers& mods, const std::vector<AnnotationPtr>& annots,
    TypeRefPtr type, const std::string& name) {
    
    auto field = std::make_shared<FieldDecl>();
    field->modifiers = mods;
    field->annotations = annots;
    field->fieldType = type;
    field->name = name;
    
    // Inicializador opcional
    if (match(TokenType::ASSIGN)) {
        field->initializer = parseExpression();
    }
    
    // Semicolon é opcional para compatibilidade com KAVA
    match(TokenType::SEMICOLON);
    
    return field;
}

std::shared_ptr<MethodDecl> Parser::parseMethodDeclaration(
    const Modifiers& mods, const std::vector<AnnotationPtr>& annots,
    const std::vector<std::string>& typeParams, TypeRefPtr returnType, const std::string& name) {
    
    auto method = std::make_shared<MethodDecl>();
    method->modifiers = mods;
    method->annotations = annots;
    method->typeParams = typeParams;
    method->returnType = returnType;
    method->name = name;
    
    // Parâmetros
    method->parameters = parseParameters();
    
    // throws
    if (match(TokenType::THROWS)) {
        do {
            method->throwsTypes.push_back(parseType());
        } while (match(TokenType::COMMA));
    }
    
    // Body (ou abstract/native)
    if (mods.isAbstract || mods.isNative) {
        consume(TokenType::SEMICOLON, "Esperado ';' para método abstrato/nativo");
        method->body = nullptr;
    } else if (check(TokenType::LBRACE)) {
        method->body = parseBlock();
    } else {
        // Sintaxe alternativa: func name() = expr
        if (match(TokenType::ASSIGN)) {
            auto body = std::make_shared<BlockStmt>();
            auto ret = std::make_shared<ReturnStmt>();
            ret->value = parseExpression();
            body->statements.push_back(ret);
            method->body = body;
        }
        match(TokenType::SEMICOLON);
    }
    
    return method;
}

std::shared_ptr<ConstructorDecl> Parser::parseConstructorDeclaration(
    const Modifiers& mods, const std::vector<AnnotationPtr>& annots, const std::string& name) {
    
    auto ctor = std::make_shared<ConstructorDecl>();
    ctor->modifiers = mods;
    ctor->annotations = annots;
    ctor->name = name;
    
    // Parâmetros
    ctor->parameters = parseParameters();
    
    // throws
    if (match(TokenType::THROWS)) {
        do {
            ctor->throwsTypes.push_back(parseType());
        } while (match(TokenType::COMMA));
    }
    
    // Body
    ctor->body = parseBlock();
    
    // Verifica se primeira instrução é this() ou super()
    if (ctor->body && !ctor->body->statements.empty()) {
        auto firstStmt = ctor->body->statements[0];
        if (firstStmt->getType() == NodeType::ExprStmt) {
            auto exprStmt = std::static_pointer_cast<ExprStmt>(firstStmt);
            if (exprStmt->expression->getType() == NodeType::MethodCallExpr) {
                auto call = std::static_pointer_cast<MethodCallExpr>(exprStmt->expression);
                if (call->methodName == "this" || call->methodName == "super") {
                    ctor->hasExplicitConstructorCall = true;
                    ctor->callsThis = (call->methodName == "this");
                    ctor->constructorArgs = call->arguments;
                }
            }
        }
    }
    
    return ctor;
}

std::vector<ParameterDecl> Parser::parseParameters() {
    std::vector<ParameterDecl> params;
    
    consume(TokenType::LPAREN, "Esperado '('");
    
    if (!check(TokenType::RPAREN)) {
        do {
            params.push_back(parseParameter());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Esperado ')'");
    
    return params;
}

ParameterDecl Parser::parseParameter() {
    ParameterDecl param;
    param.annotations = parseAnnotations();
    
    if (match(TokenType::FINAL)) {
        param.modifiers.isFinal = true;
    }
    
    param.type = parseType();
    
    // Varargs
    if (match(TokenType::ELLIPSIS)) {
        param.isVarArgs = true;
    }
    
    param.name = consume(TokenType::IDENTIFIER, "Esperado nome do parâmetro").lexeme;
    
    return param;
}

// ============================================================
// STATEMENTS
// ============================================================
StmtPtr Parser::parseStatement() {
    try {
        // Declaração de variável local
        if (check(TokenType::LET) || check(TokenType::FINAL)) {
            return parseLocalVariableDeclaration();
        }
        
        // Verifica se é declaração de tipo
        if (check(TokenType::INT) || check(TokenType::LONG) || check(TokenType::FLOAT) ||
            check(TokenType::DOUBLE) || check(TokenType::BOOLEAN) || check(TokenType::BYTE) ||
            check(TokenType::CHAR) || check(TokenType::SHORT)) {
            return parseLocalVariableDeclaration();
        }
        
        // Pode ser tipo complexo ou expressão
        if (check(TokenType::IDENTIFIER)) {
            // Olha adiante para ver se é declaração
            size_t saved = current;
            parseType();  // Consome o tipo
            if (check(TokenType::IDENTIFIER)) {
                current = saved;  // Volta
                return parseLocalVariableDeclaration();
            }
            current = saved;  // Volta e trata como expressão
        }
        
        // Statements de controle
        if (match(TokenType::IF)) return parseIfStatement();
        if (match(TokenType::WHILE)) return parseWhileStatement();
        if (match(TokenType::DO)) return parseDoWhileStatement();
        if (match(TokenType::FOR)) return parseForStatement();
        if (match(TokenType::SWITCH)) return parseSwitchStatement();
        if (match(TokenType::TRY)) return parseTryStatement();
        if (match(TokenType::SYNCHRONIZED)) return parseSynchronizedStatement();
        if (match(TokenType::RETURN)) return parseReturnStatement();
        if (match(TokenType::THROW)) return parseThrowStatement();
        if (match(TokenType::BREAK)) return parseBreakStatement();
        if (match(TokenType::CONTINUE)) return parseContinueStatement();
        if (match(TokenType::ASSERT)) return parseAssertStatement();
        if (match(TokenType::PRINT)) return parsePrintStatement();
        
        // Bloco
        if (check(TokenType::LBRACE)) return parseBlock();
        
        // Statement vazio
        if (match(TokenType::SEMICOLON)) {
            return std::make_shared<BlockStmt>();
        }
        
        // Expression statement
        return parseExpressionStatement();
        
    } catch (const ParseError&) {
        synchronize();
        return std::make_shared<BlockStmt>();
    }
}

std::shared_ptr<BlockStmt> Parser::parseBlock() {
    consume(TokenType::LBRACE, "Esperado '{'");
    
    auto block = std::make_shared<BlockStmt>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->statements.push_back(parseStatement());
    }
    
    consume(TokenType::RBRACE, "Esperado '}'");
    
    return block;
}

StmtPtr Parser::parseLocalVariableDeclaration() {
    auto decl = std::make_shared<VarDeclStmt>();
    
    // Modificadores (final)
    if (match(TokenType::FINAL)) {
        decl->modifiers.isFinal = true;
    }
    
    // let keyword (extensão KAVA)
    match(TokenType::LET);
    
    // Tipo (pode ser omitido se usar let com inicializador)
    if (!check(TokenType::IDENTIFIER) || 
        (tokens.size() > current + 1 && tokens[current + 1].type != TokenType::ASSIGN)) {
        decl->varType = parseType();
    }
    
    // Nome
    decl->name = consume(TokenType::IDENTIFIER, "Esperado nome da variável").lexeme;
    
    // Inicializador
    if (match(TokenType::ASSIGN)) {
        decl->initializer = parseExpression();
    }
    
    match(TokenType::SEMICOLON);  // Opcional
    
    return decl;
}

StmtPtr Parser::parseIfStatement() {
    auto stmt = std::make_shared<IfStmt>();
    
    // Parênteses opcionais (extensão KAVA)
    bool hasParen = match(TokenType::LPAREN);
    stmt->condition = parseExpression();
    if (hasParen) consume(TokenType::RPAREN, "Esperado ')'");
    
    stmt->thenBranch = parseStatement();
    
    if (match(TokenType::ELSE)) {
        stmt->elseBranch = parseStatement();
    }
    
    return stmt;
}

StmtPtr Parser::parseWhileStatement() {
    auto stmt = std::make_shared<WhileStmt>();
    
    bool hasParen = match(TokenType::LPAREN);
    stmt->condition = parseExpression();
    if (hasParen) consume(TokenType::RPAREN, "Esperado ')'");
    
    stmt->body = parseStatement();
    
    return stmt;
}

StmtPtr Parser::parseDoWhileStatement() {
    auto stmt = std::make_shared<DoWhileStmt>();
    
    stmt->body = parseStatement();
    
    consume(TokenType::WHILE, "Esperado 'while' após do-block");
    
    consume(TokenType::LPAREN, "Esperado '('");
    stmt->condition = parseExpression();
    consume(TokenType::RPAREN, "Esperado ')'");
    
    match(TokenType::SEMICOLON);
    
    return stmt;
}

StmtPtr Parser::parseForStatement() {
    consume(TokenType::LPAREN, "Esperado '('");
    
    // Verifica se é for-each
    // Heurística: se tem tipo seguido de identificador e ":"
    size_t saved = current;
    bool isForEach = false;
    
    try {
        Modifiers mods;
        if (match(TokenType::FINAL)) mods.isFinal = true;
        parseType();
        if (check(TokenType::IDENTIFIER)) {
            advance();
            if (check(TokenType::COLON)) {
                isForEach = true;
            }
        }
    } catch (...) {}
    
    current = saved;
    
    if (isForEach) {
        auto stmt = std::make_shared<ForEachStmt>();
        
        if (match(TokenType::FINAL)) {
            stmt->modifiers.isFinal = true;
        }
        
        stmt->varType = parseType();
        stmt->varName = consume(TokenType::IDENTIFIER, "Esperado nome da variável").lexeme;
        consume(TokenType::COLON, "Esperado ':'");
        stmt->iterable = parseExpression();
        consume(TokenType::RPAREN, "Esperado ')'");
        stmt->body = parseStatement();
        
        return stmt;
    }
    
    // For tradicional
    auto stmt = std::make_shared<ForStmt>();
    
    // Init
    if (!check(TokenType::SEMICOLON)) {
        stmt->init.push_back(parseStatement());
    } else {
        advance();  // ;
    }
    
    // Condition
    if (!check(TokenType::SEMICOLON)) {
        stmt->condition = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Esperado ';'");
    
    // Update
    if (!check(TokenType::RPAREN)) {
        do {
            stmt->update.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Esperado ')'");
    stmt->body = parseStatement();
    
    return stmt;
}

StmtPtr Parser::parseSwitchStatement() {
    auto stmt = std::make_shared<SwitchStmt>();
    
    consume(TokenType::LPAREN, "Esperado '('");
    stmt->selector = parseExpression();
    consume(TokenType::RPAREN, "Esperado ')'");
    
    consume(TokenType::LBRACE, "Esperado '{'");
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto caseClause = std::make_shared<CaseClause>();
        
        // Labels
        while (match(TokenType::CASE) || match(TokenType::DEFAULT)) {
            if (previous().type == TokenType::CASE) {
                caseClause->labels.push_back(parseExpression());
            }
            consume(TokenType::COLON, "Esperado ':'");
        }
        
        // Statements
        while (!check(TokenType::CASE) && !check(TokenType::DEFAULT) && 
               !check(TokenType::RBRACE) && !isAtEnd()) {
            caseClause->statements.push_back(parseStatement());
        }
        
        stmt->cases.push_back(caseClause);
    }
    
    consume(TokenType::RBRACE, "Esperado '}'");
    
    return stmt;
}

StmtPtr Parser::parseReturnStatement() {
    auto stmt = std::make_shared<ReturnStmt>();
    
    if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE)) {
        stmt->value = parseExpression();
    }
    
    match(TokenType::SEMICOLON);
    
    return stmt;
}

StmtPtr Parser::parseThrowStatement() {
    auto stmt = std::make_shared<ThrowStmt>();
    stmt->exception = parseExpression();
    match(TokenType::SEMICOLON);
    return stmt;
}

StmtPtr Parser::parseTryStatement() {
    auto stmt = std::make_shared<TryStmt>();
    
    stmt->tryBlock = parseBlock();
    
    // Catch clauses
    while (match(TokenType::CATCH)) {
        auto catchClause = std::make_shared<CatchClause>();
        
        consume(TokenType::LPAREN, "Esperado '('");
        
        // Multi-catch (Type1 | Type2)
        do {
            catchClause->exceptionTypes.push_back(parseType());
        } while (match(TokenType::PIPE));
        
        catchClause->varName = consume(TokenType::IDENTIFIER, "Esperado nome da variável").lexeme;
        consume(TokenType::RPAREN, "Esperado ')'");
        
        catchClause->body = parseBlock();
        
        stmt->catchClauses.push_back(catchClause);
    }
    
    // Finally
    if (match(TokenType::FINALLY)) {
        stmt->finallyBlock = parseBlock();
    }
    
    return stmt;
}

StmtPtr Parser::parseSynchronizedStatement() {
    auto stmt = std::make_shared<SynchronizedStmt>();
    
    consume(TokenType::LPAREN, "Esperado '('");
    stmt->lockObject = parseExpression();
    consume(TokenType::RPAREN, "Esperado ')'");
    
    stmt->body = parseBlock();
    
    return stmt;
}

StmtPtr Parser::parseAssertStatement() {
    auto stmt = std::make_shared<AssertStmt>();
    
    stmt->condition = parseExpression();
    
    if (match(TokenType::COLON)) {
        stmt->message = parseExpression();
    }
    
    match(TokenType::SEMICOLON);
    
    return stmt;
}

StmtPtr Parser::parseBreakStatement() {
    auto stmt = std::make_shared<BreakStmt>();
    
    if (check(TokenType::IDENTIFIER)) {
        stmt->label = advance().lexeme;
    }
    
    match(TokenType::SEMICOLON);
    
    return stmt;
}

StmtPtr Parser::parseContinueStatement() {
    auto stmt = std::make_shared<ContinueStmt>();
    
    if (check(TokenType::IDENTIFIER)) {
        stmt->label = advance().lexeme;
    }
    
    match(TokenType::SEMICOLON);
    
    return stmt;
}

StmtPtr Parser::parsePrintStatement() {
    auto stmt = std::make_shared<PrintStmt>();
    stmt->expression = parseExpression();
    match(TokenType::SEMICOLON);
    return stmt;
}

StmtPtr Parser::parseExpressionStatement() {
    auto stmt = std::make_shared<ExprStmt>();
    stmt->expression = parseExpression();
    match(TokenType::SEMICOLON);
    return stmt;
}

// ============================================================
// EXPRESSÕES
// ============================================================
ExprPtr Parser::parseExpression() {
    return parseAssignmentExpression();
}

ExprPtr Parser::parseAssignmentExpression() {
    ExprPtr expr = parseTernaryExpression();
    
    if (isAssignmentOperator()) {
        Token op = advance();
        ExprPtr value = parseAssignmentExpression();
        
        if (op.type == TokenType::ASSIGN) {
            auto assign = std::make_shared<AssignExpr>();
            assign->target = expr;
            assign->value = value;
            return assign;
        } else {
            auto compound = std::make_shared<CompoundAssignExpr>();
            compound->target = expr;
            compound->value = value;
            compound->op = getAssignmentOp();
            return compound;
        }
    }
    
    return expr;
}

bool Parser::isAssignmentOperator() const {
    return check(TokenType::ASSIGN) || check(TokenType::PLUS_ASSIGN) ||
           check(TokenType::MINUS_ASSIGN) || check(TokenType::STAR_ASSIGN) ||
           check(TokenType::SLASH_ASSIGN) || check(TokenType::PERCENT_ASSIGN) ||
           check(TokenType::AND_ASSIGN) || check(TokenType::OR_ASSIGN) ||
           check(TokenType::XOR_ASSIGN) || check(TokenType::LSHIFT_ASSIGN) ||
           check(TokenType::RSHIFT_ASSIGN) || check(TokenType::URSHIFT_ASSIGN);
}

BinaryExpr::Op Parser::getAssignmentOp() const {
    switch (previous().type) {
        case TokenType::PLUS_ASSIGN: return BinaryExpr::Op::Add;
        case TokenType::MINUS_ASSIGN: return BinaryExpr::Op::Sub;
        case TokenType::STAR_ASSIGN: return BinaryExpr::Op::Mul;
        case TokenType::SLASH_ASSIGN: return BinaryExpr::Op::Div;
        case TokenType::PERCENT_ASSIGN: return BinaryExpr::Op::Mod;
        case TokenType::AND_ASSIGN: return BinaryExpr::Op::BitAnd;
        case TokenType::OR_ASSIGN: return BinaryExpr::Op::BitOr;
        case TokenType::XOR_ASSIGN: return BinaryExpr::Op::BitXor;
        case TokenType::LSHIFT_ASSIGN: return BinaryExpr::Op::LeftShift;
        case TokenType::RSHIFT_ASSIGN: return BinaryExpr::Op::RightShift;
        case TokenType::URSHIFT_ASSIGN: return BinaryExpr::Op::UnsignedRightShift;
        default: return BinaryExpr::Op::Add;
    }
}

ExprPtr Parser::parseTernaryExpression() {
    ExprPtr condition = parseLogicalOrExpression();
    
    if (match(TokenType::QUESTION)) {
        auto ternary = std::make_shared<TernaryExpr>();
        ternary->condition = condition;
        ternary->thenExpr = parseExpression();
        consume(TokenType::COLON, "Esperado ':' em expressão ternária");
        ternary->elseExpr = parseTernaryExpression();
        return ternary;
    }
    
    return condition;
}

ExprPtr Parser::parseLogicalOrExpression() {
    ExprPtr left = parseLogicalAndExpression();
    
    while (match(TokenType::OR)) {
        auto binary = std::make_shared<BinaryExpr>();
        binary->op = BinaryExpr::Op::Or;
        binary->left = left;
        binary->right = parseLogicalAndExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseLogicalAndExpression() {
    ExprPtr left = parseBitwiseOrExpression();
    
    while (match(TokenType::AND)) {
        auto binary = std::make_shared<BinaryExpr>();
        binary->op = BinaryExpr::Op::And;
        binary->left = left;
        binary->right = parseBitwiseOrExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseBitwiseOrExpression() {
    ExprPtr left = parseBitwiseXorExpression();
    
    while (match(TokenType::PIPE)) {
        auto binary = std::make_shared<BinaryExpr>();
        binary->op = BinaryExpr::Op::BitOr;
        binary->left = left;
        binary->right = parseBitwiseXorExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseBitwiseXorExpression() {
    ExprPtr left = parseBitwiseAndExpression();
    
    while (match(TokenType::CARET)) {
        auto binary = std::make_shared<BinaryExpr>();
        binary->op = BinaryExpr::Op::BitXor;
        binary->left = left;
        binary->right = parseBitwiseAndExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseBitwiseAndExpression() {
    ExprPtr left = parseEqualityExpression();
    
    while (match(TokenType::AMPERSAND)) {
        auto binary = std::make_shared<BinaryExpr>();
        binary->op = BinaryExpr::Op::BitAnd;
        binary->left = left;
        binary->right = parseEqualityExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseEqualityExpression() {
    ExprPtr left = parseRelationalExpression();
    
    while (match({TokenType::EQ, TokenType::NE})) {
        auto binary = std::make_shared<BinaryExpr>();
        binary->op = previous().type == TokenType::EQ ? BinaryExpr::Op::Eq : BinaryExpr::Op::NotEq;
        binary->left = left;
        binary->right = parseRelationalExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseRelationalExpression() {
    ExprPtr left = parseShiftExpression();
    
    while (true) {
        if (match({TokenType::LT, TokenType::LE, TokenType::GT, TokenType::GE})) {
            auto binary = std::make_shared<BinaryExpr>();
            switch (previous().type) {
                case TokenType::LT: binary->op = BinaryExpr::Op::Lt; break;
                case TokenType::LE: binary->op = BinaryExpr::Op::LtEq; break;
                case TokenType::GT: binary->op = BinaryExpr::Op::Gt; break;
                case TokenType::GE: binary->op = BinaryExpr::Op::GtEq; break;
                default: break;
            }
            binary->left = left;
            binary->right = parseShiftExpression();
            left = binary;
        } else if (match(TokenType::INSTANCEOF)) {
            auto instOf = std::make_shared<InstanceOfExpr>();
            instOf->operand = left;
            instOf->checkType = parseType();
            left = instOf;
        } else {
            break;
        }
    }
    
    return left;
}

ExprPtr Parser::parseShiftExpression() {
    ExprPtr left = parseAdditiveExpression();
    
    while (match({TokenType::LSHIFT, TokenType::RSHIFT, TokenType::URSHIFT})) {
        auto binary = std::make_shared<BinaryExpr>();
        switch (previous().type) {
            case TokenType::LSHIFT: binary->op = BinaryExpr::Op::LeftShift; break;
            case TokenType::RSHIFT: binary->op = BinaryExpr::Op::RightShift; break;
            case TokenType::URSHIFT: binary->op = BinaryExpr::Op::UnsignedRightShift; break;
            default: break;
        }
        binary->left = left;
        binary->right = parseAdditiveExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseAdditiveExpression() {
    ExprPtr left = parseMultiplicativeExpression();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        auto binary = std::make_shared<BinaryExpr>();
        binary->op = previous().type == TokenType::PLUS ? BinaryExpr::Op::Add : BinaryExpr::Op::Sub;
        binary->left = left;
        binary->right = parseMultiplicativeExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseMultiplicativeExpression() {
    ExprPtr left = parseUnaryExpression();
    
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        auto binary = std::make_shared<BinaryExpr>();
        switch (previous().type) {
            case TokenType::STAR: binary->op = BinaryExpr::Op::Mul; break;
            case TokenType::SLASH: binary->op = BinaryExpr::Op::Div; break;
            case TokenType::PERCENT: binary->op = BinaryExpr::Op::Mod; break;
            default: break;
        }
        binary->left = left;
        binary->right = parseUnaryExpression();
        left = binary;
    }
    
    return left;
}

ExprPtr Parser::parseUnaryExpression() {
    if (match({TokenType::NOT, TokenType::TILDE, TokenType::MINUS, 
               TokenType::PLUS_PLUS, TokenType::MINUS_MINUS})) {
        auto unary = std::make_shared<UnaryExpr>();
        switch (previous().type) {
            case TokenType::NOT: unary->op = UnaryExpr::Op::Not; break;
            case TokenType::TILDE: unary->op = UnaryExpr::Op::BitNot; break;
            case TokenType::MINUS: unary->op = UnaryExpr::Op::Negate; break;
            case TokenType::PLUS_PLUS: unary->op = UnaryExpr::Op::PreInc; break;
            case TokenType::MINUS_MINUS: unary->op = UnaryExpr::Op::PreDec; break;
            default: break;
        }
        unary->operand = parseUnaryExpression();
        return unary;
    }
    
    // Cast
    if (check(TokenType::LPAREN)) {
        size_t saved = current;
        advance();  // (
        
        bool isCast = false;
        if (check(TokenType::INT) || check(TokenType::LONG) || check(TokenType::FLOAT) ||
            check(TokenType::DOUBLE) || check(TokenType::BOOLEAN) || check(TokenType::BYTE) ||
            check(TokenType::CHAR) || check(TokenType::SHORT)) {
            isCast = true;
        } else if (check(TokenType::IDENTIFIER)) {
            // Pode ser cast para tipo de classe
            try {
                parseType();
                if (check(TokenType::RPAREN)) {
                    isCast = true;
                }
            } catch (...) {}
        }
        
        current = saved;
        
        if (isCast) {
            advance();  // (
            auto cast = std::make_shared<CastExpr>();
            cast->targetType = parseType();
            consume(TokenType::RPAREN, "Esperado ')'");
            cast->operand = parseUnaryExpression();
            return cast;
        }
    }
    
    return parsePostfixExpression();
}

ExprPtr Parser::parsePostfixExpression() {
    ExprPtr expr = parsePrimaryExpression();
    
    while (true) {
        if (match(TokenType::PLUS_PLUS)) {
            auto unary = std::make_shared<UnaryExpr>();
            unary->op = UnaryExpr::Op::PostInc;
            unary->operand = expr;
            expr = unary;
        } else if (match(TokenType::MINUS_MINUS)) {
            auto unary = std::make_shared<UnaryExpr>();
            unary->op = UnaryExpr::Op::PostDec;
            unary->operand = expr;
            expr = unary;
        } else if (match(TokenType::DOT)) {
            std::string name = consume(TokenType::IDENTIFIER, "Esperado nome do membro").lexeme;
            if (check(TokenType::LPAREN)) {
                expr = parseMethodCall(expr, name);
            } else {
                auto member = std::make_shared<MemberExpr>();
                member->object = expr;
                member->memberName = name;
                expr = member;
            }
        } else if (check(TokenType::LBRACKET)) {
            expr = parseArrayAccess(expr);
        } else if (check(TokenType::LPAREN) && expr->getType() == NodeType::Identifier) {
            auto id = std::static_pointer_cast<IdentifierExpr>(expr);
            expr = parseMethodCall(nullptr, id->name);
        } else {
            break;
        }
    }
    
    return expr;
}

ExprPtr Parser::parsePrimaryExpression() {
    // Literais
    if (match(TokenType::TRUE)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::Boolean;
        lit->value = "true";
        return lit;
    }
    
    if (match(TokenType::FALSE)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::Boolean;
        lit->value = "false";
        return lit;
    }
    
    if (match(TokenType::NULL_LITERAL)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::Null;
        return lit;
    }
    
    if (match(TokenType::INT_LITERAL)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::Int;
        lit->value = previous().lexeme;
        return lit;
    }
    
    if (match(TokenType::LONG_LITERAL)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::Long;
        lit->value = previous().lexeme;
        return lit;
    }
    
    if (match(TokenType::FLOAT_LITERAL)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::Float;
        lit->value = previous().lexeme;
        return lit;
    }
    
    if (match(TokenType::DOUBLE_LITERAL)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::Double;
        lit->value = previous().lexeme;
        return lit;
    }
    
    if (match(TokenType::CHAR_LITERAL)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::Char;
        lit->value = previous().lexeme;
        return lit;
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->litType = LiteralExpr::LitType::String;
        lit->value = previous().lexeme;
        return lit;
    }
    
    // this e super
    if (match(TokenType::THIS)) {
        if (check(TokenType::LPAREN)) {
            return parseMethodCall(nullptr, "this");
        }
        return std::make_shared<ThisExpr>();
    }
    
    if (match(TokenType::SUPER)) {
        if (check(TokenType::LPAREN)) {
            return parseMethodCall(nullptr, "super");
        }
        auto superExpr = std::make_shared<SuperExpr>();
        if (match(TokenType::DOT)) {
            std::string name = consume(TokenType::IDENTIFIER, "Esperado nome do membro").lexeme;
            if (check(TokenType::LPAREN)) {
                auto call = std::static_pointer_cast<MethodCallExpr>(parseMethodCall(superExpr, name));
                call->isSuperCall = true;
                return call;
            }
            auto member = std::make_shared<MemberExpr>();
            member->object = superExpr;
            member->memberName = name;
            return member;
        }
        return superExpr;
    }
    
    // new
    if (match(TokenType::NEW)) {
        return parseNewExpression();
    }
    
    // Parênteses
    if (match(TokenType::LPAREN)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Esperado ')'");
        return expr;
    }
    
    // Identificador
    if (match(TokenType::IDENTIFIER)) {
        auto id = std::make_shared<IdentifierExpr>();
        id->name = previous().lexeme;
        id->isLValue = true;
        return id;
    }
    
    throw error(peek(), "Esperada expressão");
}

ExprPtr Parser::parseNewExpression() {
    TypeRefPtr type = parseType();
    
    // new Type[size] ou new Type[]{ ... }
    if (type->arrayDimensions > 0 || check(TokenType::LBRACKET)) {
        auto newArray = std::make_shared<NewArrayExpr>();
        newArray->elementType = type;
        
        while (match(TokenType::LBRACKET)) {
            if (!check(TokenType::RBRACKET)) {
                newArray->dimensions.push_back(parseExpression());
            }
            consume(TokenType::RBRACKET, "Esperado ']'");
        }
        
        // Array initializer
        if (check(TokenType::LBRACE)) {
            newArray->initializer.push_back(parseArrayInitializer());
        }
        
        return newArray;
    }
    
    // new Type(args)
    auto newExpr = std::make_shared<NewExpr>();
    newExpr->classType = type;
    newExpr->arguments = parseArguments();
    
    // Anonymous inner class
    if (check(TokenType::LBRACE)) {
        consume(TokenType::LBRACE, "Esperado '{'");
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            newExpr->anonymousClassBody.push_back(parseStatement());
        }
        consume(TokenType::RBRACE, "Esperado '}'");
    }
    
    return newExpr;
}

ExprPtr Parser::parseMethodCall(ExprPtr object, const std::string& name) {
    auto call = std::make_shared<MethodCallExpr>();
    call->object = object;
    call->methodName = name;
    call->arguments = parseArguments();
    return call;
}

ExprPtr Parser::parseArrayAccess(ExprPtr array) {
    consume(TokenType::LBRACKET, "Esperado '['");
    
    auto access = std::make_shared<ArrayAccessExpr>();
    access->array = array;
    access->index = parseExpression();
    
    consume(TokenType::RBRACKET, "Esperado ']'");
    
    return access;
}

std::vector<ExprPtr> Parser::parseArguments() {
    consume(TokenType::LPAREN, "Esperado '('");
    
    std::vector<ExprPtr> args;
    
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Esperado ')'");
    
    return args;
}

ExprPtr Parser::parseArrayInitializer() {
    consume(TokenType::LBRACE, "Esperado '{'");
    
    // Retorna como NewArrayExpr com inicializador
    auto newArray = std::make_shared<NewArrayExpr>();
    
    if (!check(TokenType::RBRACE)) {
        do {
            if (check(TokenType::LBRACE)) {
                // Nested array initializer
                newArray->initializer.push_back(parseArrayInitializer());
            } else {
                newArray->initializer.push_back(parseExpression());
            }
        } while (match(TokenType::COMMA) && !check(TokenType::RBRACE));
    }
    
    consume(TokenType::RBRACE, "Esperado '}'");
    
    return newArray;
}

} // namespace Kava
