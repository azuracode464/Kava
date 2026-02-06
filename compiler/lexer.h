/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.5 - Lexer Completo
 * Suporte a Java 6 + Lambdas, Streams, Async/Await, Functional Interfaces
 */

#ifndef KAVA_LEXER_H
#define KAVA_LEXER_H

#include <string>
#include <vector>
#include <map>

namespace Kava {

// ============================================================
// TIPOS DE TOKEN
// ============================================================
enum class TokenType {
    // Literais
    INT_LITERAL,
    LONG_LITERAL,
    FLOAT_LITERAL,
    DOUBLE_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,
    TRUE,
    FALSE,
    NULL_LITERAL,
    
    // Identificador
    IDENTIFIER,
    
    // Keywords - Declarações
    PACKAGE,
    IMPORT,
    CLASS,
    INTERFACE,
    ENUM,
    EXTENDS,
    IMPLEMENTS,
    
    // Keywords - Modificadores
    PUBLIC,
    PROTECTED,
    PRIVATE,
    STATIC,
    FINAL,
    ABSTRACT,
    NATIVE,
    SYNCHRONIZED,
    VOLATILE,
    TRANSIENT,
    STRICTFP,
    
    // Keywords - Tipos primitivos
    VOID,
    BOOLEAN,
    BYTE,
    CHAR,
    SHORT,
    INT,
    LONG,
    FLOAT,
    DOUBLE,
    
    // Keywords - Controle de fluxo
    IF,
    ELSE,
    SWITCH,
    CASE,
    DEFAULT,
    WHILE,
    DO,
    FOR,
    BREAK,
    CONTINUE,
    RETURN,
    
    // Keywords - Exceções
    TRY,
    CATCH,
    FINALLY,
    THROW,
    THROWS,
    
    // Keywords - OOP
    NEW,
    THIS,
    SUPER,
    INSTANCEOF,
    
    // Keywords - Outros
    ASSERT,
    LET,          // Extensão KAVA para variável local
    FUNC,         // Extensão KAVA para função (alias para método)
    PRINT,        // Extensão KAVA para saída simples
    STRUCT,       // Extensão KAVA (mapeia para classe)
    
    // === KAVA 2.5 Keywords ===
    ASYNC,        // Extensão KAVA 2.5 - async functions
    AWAIT,        // Extensão KAVA 2.5 - await expressions
    STREAM,       // Extensão KAVA 2.5 - stream operations
    YIELD,        // Extensão KAVA 2.5 - yield in generators
    DEFAULT_METHOD, // Extensão KAVA 2.5 - default interface methods
    PIPE_OP,      // Extensão KAVA 2.5 - |> pipe operator
    
    // Operadores Aritméticos
    PLUS,         // +
    MINUS,        // -
    STAR,         // *
    SLASH,        // /
    PERCENT,      // %
    
    // Operadores de Incremento/Decremento
    PLUS_PLUS,    // ++
    MINUS_MINUS,  // --
    
    // Operadores Relacionais
    EQ,           // ==
    NE,           // !=
    LT,           // <
    LE,           // <=
    GT,           // >
    GE,           // >=
    
    // Operadores Lógicos
    AND,          // &&
    OR,           // ||
    NOT,          // !
    
    // Operadores Bitwise
    AMPERSAND,    // &
    PIPE,         // |
    CARET,        // ^
    TILDE,        // ~
    LSHIFT,       // <<
    RSHIFT,       // >>
    URSHIFT,      // >>>
    
    // Operadores de Atribuição
    ASSIGN,       // =
    PLUS_ASSIGN,  // +=
    MINUS_ASSIGN, // -=
    STAR_ASSIGN,  // *=
    SLASH_ASSIGN, // /=
    PERCENT_ASSIGN, // %=
    AND_ASSIGN,   // &=
    OR_ASSIGN,    // |=
    XOR_ASSIGN,   // ^=
    LSHIFT_ASSIGN, // <<=
    RSHIFT_ASSIGN, // >>=
    URSHIFT_ASSIGN, // >>>=
    
    // Delimitadores
    LPAREN,       // (
    RPAREN,       // )
    LBRACE,       // {
    RBRACE,       // }
    LBRACKET,     // [
    RBRACKET,     // ]
    SEMICOLON,    // ;
    COMMA,        // ,
    DOT,          // .
    COLON,        // :
    QUESTION,     // ?
    AT,           // @
    ELLIPSIS,     // ...
    ARROW,        // ->
    COLONCOLON,   // ::
    
    // Especiais
    EOF_TOKEN,
    ERROR
};

// ============================================================
// TOKEN
// ============================================================
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    
    // Para valores literais
    union {
        int64_t intValue;
        double doubleValue;
        char charValue;
    };
    
    Token() : type(TokenType::ERROR), line(0), column(0), intValue(0) {}
    Token(TokenType t, const std::string& lex, int ln, int col)
        : type(t), lexeme(lex), line(ln), column(col), intValue(0) {}
    
    bool is(TokenType t) const { return type == t; }
    bool isKeyword() const { return type >= TokenType::PACKAGE && type <= TokenType::STRUCT; }
    bool isLiteral() const { return type >= TokenType::INT_LITERAL && type <= TokenType::NULL_LITERAL; }
    bool isOperator() const { return type >= TokenType::PLUS && type <= TokenType::COLONCOLON; }
    bool isAssignOp() const { return type >= TokenType::ASSIGN && type <= TokenType::URSHIFT_ASSIGN; }
    
    std::string toString() const;
};

// ============================================================
// LEXER
// ============================================================
class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> scanTokens();
    
    // Para parsing incremental
    Token nextToken();
    Token peekToken();
    bool hasError() const { return errorCount > 0; }
    int getErrorCount() const { return errorCount; }
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    std::string source;
    std::vector<Token> tokens;
    size_t start = 0;
    size_t current = 0;
    int line = 1;
    int column = 1;
    int startColumn = 1;
    
    int errorCount = 0;
    std::vector<std::string> errors;
    
    static std::map<std::string, TokenType> keywords;
    static void initKeywords();
    
    // Helpers de navegação
    bool isAtEnd() const { return current >= source.length(); }
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    
    // Scanner
    void scanToken();
    Token makeToken(TokenType type);
    Token errorToken(const std::string& message);
    
    // Específicos
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    Token identifier();
    Token number();
    Token string();
    Token character();
    
    // Verificações
    bool isDigit(char c) const { return c >= '0' && c <= '9'; }
    bool isHexDigit(char c) const { 
        return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); 
    }
    bool isAlpha(char c) const { 
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$'; 
    }
    bool isAlphaNumeric(char c) const { return isAlpha(c) || isDigit(c); }
};

} // namespace Kava

#endif // KAVA_LEXER_H
