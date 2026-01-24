/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Implementação do Lexer
 */

#include "lexer.h"
#include <sstream>
#include <iomanip>

namespace Kava {

// ============================================================
// TABELA DE KEYWORDS
// ============================================================
std::map<std::string, TokenType> Lexer::keywords;

void Lexer::initKeywords() {
    if (!keywords.empty()) return;
    
    // Declarações
    keywords["package"] = TokenType::PACKAGE;
    keywords["import"] = TokenType::IMPORT;
    keywords["class"] = TokenType::CLASS;
    keywords["interface"] = TokenType::INTERFACE;
    keywords["enum"] = TokenType::ENUM;
    keywords["extends"] = TokenType::EXTENDS;
    keywords["implements"] = TokenType::IMPLEMENTS;
    
    // Modificadores
    keywords["public"] = TokenType::PUBLIC;
    keywords["protected"] = TokenType::PROTECTED;
    keywords["private"] = TokenType::PRIVATE;
    keywords["static"] = TokenType::STATIC;
    keywords["final"] = TokenType::FINAL;
    keywords["abstract"] = TokenType::ABSTRACT;
    keywords["native"] = TokenType::NATIVE;
    keywords["synchronized"] = TokenType::SYNCHRONIZED;
    keywords["volatile"] = TokenType::VOLATILE;
    keywords["transient"] = TokenType::TRANSIENT;
    keywords["strictfp"] = TokenType::STRICTFP;
    
    // Tipos primitivos
    keywords["void"] = TokenType::VOID;
    keywords["boolean"] = TokenType::BOOLEAN;
    keywords["byte"] = TokenType::BYTE;
    keywords["char"] = TokenType::CHAR;
    keywords["short"] = TokenType::SHORT;
    keywords["int"] = TokenType::INT;
    keywords["long"] = TokenType::LONG;
    keywords["float"] = TokenType::FLOAT;
    keywords["double"] = TokenType::DOUBLE;
    
    // Controle de fluxo
    keywords["if"] = TokenType::IF;
    keywords["else"] = TokenType::ELSE;
    keywords["switch"] = TokenType::SWITCH;
    keywords["case"] = TokenType::CASE;
    keywords["default"] = TokenType::DEFAULT;
    keywords["while"] = TokenType::WHILE;
    keywords["do"] = TokenType::DO;
    keywords["for"] = TokenType::FOR;
    keywords["break"] = TokenType::BREAK;
    keywords["continue"] = TokenType::CONTINUE;
    keywords["return"] = TokenType::RETURN;
    
    // Exceções
    keywords["try"] = TokenType::TRY;
    keywords["catch"] = TokenType::CATCH;
    keywords["finally"] = TokenType::FINALLY;
    keywords["throw"] = TokenType::THROW;
    keywords["throws"] = TokenType::THROWS;
    
    // OOP
    keywords["new"] = TokenType::NEW;
    keywords["this"] = TokenType::THIS;
    keywords["super"] = TokenType::SUPER;
    keywords["instanceof"] = TokenType::INSTANCEOF;
    
    // Outros
    keywords["assert"] = TokenType::ASSERT;
    keywords["true"] = TokenType::TRUE;
    keywords["false"] = TokenType::FALSE;
    keywords["null"] = TokenType::NULL_LITERAL;
    
    // Extensões KAVA
    keywords["let"] = TokenType::LET;
    keywords["func"] = TokenType::FUNC;
    keywords["print"] = TokenType::PRINT;
    keywords["struct"] = TokenType::STRUCT;
    
    // Aliases para compatibilidade
    keywords["bool"] = TokenType::BOOLEAN;
    keywords["fn"] = TokenType::FUNC;
    keywords["var"] = TokenType::LET;
}

// ============================================================
// CONSTRUTOR
// ============================================================
Lexer::Lexer(const std::string& source) : source(source) {
    initKeywords();
}

// ============================================================
// TOKEN TO STRING
// ============================================================
std::string Token::toString() const {
    std::ostringstream oss;
    oss << "[" << line << ":" << column << "] ";
    
    switch (type) {
        case TokenType::IDENTIFIER: oss << "ID(" << lexeme << ")"; break;
        case TokenType::INT_LITERAL: oss << "INT(" << lexeme << ")"; break;
        case TokenType::LONG_LITERAL: oss << "LONG(" << lexeme << ")"; break;
        case TokenType::FLOAT_LITERAL: oss << "FLOAT(" << lexeme << ")"; break;
        case TokenType::DOUBLE_LITERAL: oss << "DOUBLE(" << lexeme << ")"; break;
        case TokenType::STRING_LITERAL: oss << "STRING(\"" << lexeme << "\")"; break;
        case TokenType::CHAR_LITERAL: oss << "CHAR('" << lexeme << "')"; break;
        case TokenType::EOF_TOKEN: oss << "EOF"; break;
        case TokenType::ERROR: oss << "ERROR(" << lexeme << ")"; break;
        default: oss << lexeme; break;
    }
    
    return oss.str();
}

// ============================================================
// SCAN ALL TOKENS
// ============================================================
std::vector<Token> Lexer::scanTokens() {
    tokens.clear();
    
    while (!isAtEnd()) {
        start = current;
        startColumn = column;
        scanToken();
    }
    
    tokens.push_back(Token(TokenType::EOF_TOKEN, "", line, column));
    return tokens;
}

// ============================================================
// HELPERS
// ============================================================
char Lexer::advance() {
    char c = source[current++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    advance();
    return true;
}

Token Lexer::makeToken(TokenType type) {
    std::string text = source.substr(start, current - start);
    Token token(type, text, line, startColumn);
    tokens.push_back(token);
    return token;
}

Token Lexer::errorToken(const std::string& message) {
    errorCount++;
    std::ostringstream oss;
    oss << "Erro léxico [" << line << ":" << column << "]: " << message;
    errors.push_back(oss.str());
    
    Token token(TokenType::ERROR, message, line, startColumn);
    tokens.push_back(token);
    return token;
}

// ============================================================
// SKIP WHITESPACE E COMENTÁRIOS
// ============================================================
void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    skipLineComment();
                } else if (peekNext() == '*') {
                    skipBlockComment();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

void Lexer::skipLineComment() {
    // Avança os //
    advance();
    advance();
    
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    // Avança os /*
    advance();
    advance();
    
    int depth = 1;
    while (!isAtEnd() && depth > 0) {
        if (peek() == '/' && peekNext() == '*') {
            advance();
            advance();
            depth++;
        } else if (peek() == '*' && peekNext() == '/') {
            advance();
            advance();
            depth--;
        } else {
            advance();
        }
    }
}

// ============================================================
// SCAN TOKEN
// ============================================================
void Lexer::scanToken() {
    skipWhitespace();
    
    if (isAtEnd()) return;
    
    start = current;
    startColumn = column;
    char c = advance();
    
    // Identificadores e keywords
    if (isAlpha(c)) {
        identifier();
        return;
    }
    
    // Números
    if (isDigit(c)) {
        number();
        return;
    }
    
    // Operadores e delimitadores
    switch (c) {
        // Delimitadores simples
        case '(': makeToken(TokenType::LPAREN); return;
        case ')': makeToken(TokenType::RPAREN); return;
        case '{': makeToken(TokenType::LBRACE); return;
        case '}': makeToken(TokenType::RBRACE); return;
        case '[': makeToken(TokenType::LBRACKET); return;
        case ']': makeToken(TokenType::RBRACKET); return;
        case ';': makeToken(TokenType::SEMICOLON); return;
        case ',': makeToken(TokenType::COMMA); return;
        case '~': makeToken(TokenType::TILDE); return;
        case '?': makeToken(TokenType::QUESTION); return;
        case '@': makeToken(TokenType::AT); return;
        
        // Ponto e varargs
        case '.':
            if (peek() == '.' && peekNext() == '.') {
                advance();
                advance();
                makeToken(TokenType::ELLIPSIS);
            } else if (isDigit(peek())) {
                number();  // .5 como número
            } else {
                makeToken(TokenType::DOT);
            }
            return;
        
        // Colon
        case ':':
            if (match(':')) {
                makeToken(TokenType::COLONCOLON);
            } else {
                makeToken(TokenType::COLON);
            }
            return;
        
        // Operadores com possível atribuição
        case '+':
            if (match('+')) makeToken(TokenType::PLUS_PLUS);
            else if (match('=')) makeToken(TokenType::PLUS_ASSIGN);
            else makeToken(TokenType::PLUS);
            return;
        
        case '-':
            if (match('-')) makeToken(TokenType::MINUS_MINUS);
            else if (match('=')) makeToken(TokenType::MINUS_ASSIGN);
            else if (match('>')) makeToken(TokenType::ARROW);
            else makeToken(TokenType::MINUS);
            return;
        
        case '*':
            if (match('=')) makeToken(TokenType::STAR_ASSIGN);
            else makeToken(TokenType::STAR);
            return;
        
        case '/':
            if (match('=')) makeToken(TokenType::SLASH_ASSIGN);
            else makeToken(TokenType::SLASH);
            return;
        
        case '%':
            if (match('=')) makeToken(TokenType::PERCENT_ASSIGN);
            else makeToken(TokenType::PERCENT);
            return;
        
        // Comparação e atribuição
        case '=':
            if (match('=')) makeToken(TokenType::EQ);
            else makeToken(TokenType::ASSIGN);
            return;
        
        case '!':
            if (match('=')) makeToken(TokenType::NE);
            else makeToken(TokenType::NOT);
            return;
        
        case '<':
            if (match('<')) {
                if (match('=')) makeToken(TokenType::LSHIFT_ASSIGN);
                else makeToken(TokenType::LSHIFT);
            } else if (match('=')) {
                makeToken(TokenType::LE);
            } else {
                makeToken(TokenType::LT);
            }
            return;
        
        case '>':
            if (match('>')) {
                if (match('>')) {
                    if (match('=')) makeToken(TokenType::URSHIFT_ASSIGN);
                    else makeToken(TokenType::URSHIFT);
                } else if (match('=')) {
                    makeToken(TokenType::RSHIFT_ASSIGN);
                } else {
                    makeToken(TokenType::RSHIFT);
                }
            } else if (match('=')) {
                makeToken(TokenType::GE);
            } else {
                makeToken(TokenType::GT);
            }
            return;
        
        // Lógicos e bitwise
        case '&':
            if (match('&')) makeToken(TokenType::AND);
            else if (match('=')) makeToken(TokenType::AND_ASSIGN);
            else makeToken(TokenType::AMPERSAND);
            return;
        
        case '|':
            if (match('|')) makeToken(TokenType::OR);
            else if (match('=')) makeToken(TokenType::OR_ASSIGN);
            else makeToken(TokenType::PIPE);
            return;
        
        case '^':
            if (match('=')) makeToken(TokenType::XOR_ASSIGN);
            else makeToken(TokenType::CARET);
            return;
        
        // Strings e caracteres
        case '"': string(); return;
        case '\'': character(); return;
        
        default:
            errorToken("Caractere inesperado: " + std::string(1, c));
            return;
    }
}

// ============================================================
// IDENTIFICADORES E KEYWORDS
// ============================================================
Token Lexer::identifier() {
    while (isAlphaNumeric(peek())) advance();
    
    std::string text = source.substr(start, current - start);
    
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return makeToken(it->second);
    }
    
    return makeToken(TokenType::IDENTIFIER);
}

// ============================================================
// NÚMEROS
// ============================================================
Token Lexer::number() {
    bool isLong = false;
    bool isFloat = false;
    bool isDouble = false;
    bool isHex = false;
    bool isBinary = false;
    
    // Verifica prefixo 0x ou 0b
    if (source[start] == '0' && current < source.length()) {
        char next = peek();
        if (next == 'x' || next == 'X') {
            advance();
            isHex = true;
            while (isHexDigit(peek())) advance();
        } else if (next == 'b' || next == 'B') {
            advance();
            isBinary = true;
            while (peek() == '0' || peek() == '1') advance();
        }
    }
    
    if (!isHex && !isBinary) {
        // Parte inteira
        while (isDigit(peek())) advance();
        
        // Parte decimal
        if (peek() == '.' && isDigit(peekNext())) {
            isDouble = true;
            advance();  // .
            while (isDigit(peek())) advance();
        }
        
        // Expoente
        if (peek() == 'e' || peek() == 'E') {
            isDouble = true;
            advance();
            if (peek() == '+' || peek() == '-') advance();
            while (isDigit(peek())) advance();
        }
    }
    
    // Sufixo
    char suffix = peek();
    if (suffix == 'l' || suffix == 'L') {
        advance();
        isLong = true;
    } else if (suffix == 'f' || suffix == 'F') {
        advance();
        isFloat = true;
    } else if (suffix == 'd' || suffix == 'D') {
        advance();
        isDouble = true;
    }
    
    if (isFloat) return makeToken(TokenType::FLOAT_LITERAL);
    if (isDouble) return makeToken(TokenType::DOUBLE_LITERAL);
    if (isLong) return makeToken(TokenType::LONG_LITERAL);
    return makeToken(TokenType::INT_LITERAL);
}

// ============================================================
// STRINGS
// ============================================================
Token Lexer::string() {
    std::string value;
    
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            if (isAtEnd()) break;
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                case '0': value += '\0'; break;
                case 'u': {
                    // Unicode escape \uXXXX
                    std::string hex;
                    for (int i = 0; i < 4 && !isAtEnd() && isHexDigit(peek()); i++) {
                        hex += advance();
                    }
                    if (hex.length() == 4) {
                        int codePoint = std::stoi(hex, nullptr, 16);
                        value += static_cast<char>(codePoint);  // Simplificado
                    }
                    break;
                }
                default: value += escaped; break;
            }
        } else if (peek() == '\n') {
            return errorToken("String não terminada");
        } else {
            value += advance();
        }
    }
    
    if (isAtEnd()) {
        return errorToken("String não terminada");
    }
    
    advance();  // Fecha aspas
    
    Token token = makeToken(TokenType::STRING_LITERAL);
    tokens.back().lexeme = value;  // Usa valor processado
    return token;
}

// ============================================================
// CARACTERE
// ============================================================
Token Lexer::character() {
    if (isAtEnd()) {
        return errorToken("Caractere não terminado");
    }
    
    char value;
    if (peek() == '\\') {
        advance();
        if (isAtEnd()) return errorToken("Caractere não terminado");
        char escaped = advance();
        switch (escaped) {
            case 'n': value = '\n'; break;
            case 't': value = '\t'; break;
            case 'r': value = '\r'; break;
            case '\\': value = '\\'; break;
            case '"': value = '"'; break;
            case '\'': value = '\''; break;
            case '0': value = '\0'; break;
            default: value = escaped; break;
        }
    } else {
        value = advance();
    }
    
    if (!match('\'')) {
        return errorToken("Caractere não terminado");
    }
    
    Token token = makeToken(TokenType::CHAR_LITERAL);
    tokens.back().charValue = value;
    return token;
}

// ============================================================
// NEXT TOKEN (para parsing incremental)
// ============================================================
Token Lexer::nextToken() {
    skipWhitespace();
    
    if (isAtEnd()) {
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }
    
    start = current;
    startColumn = column;
    
    // Limpa tokens anteriores e scanneia um
    size_t oldSize = tokens.size();
    scanToken();
    
    if (tokens.size() > oldSize) {
        return tokens.back();
    }
    
    return Token(TokenType::EOF_TOKEN, "", line, column);
}

Token Lexer::peekToken() {
    size_t savedCurrent = current;
    int savedLine = line;
    int savedColumn = column;
    size_t savedStart = start;
    int savedStartColumn = startColumn;
    
    Token token = nextToken();
    
    // Restaura estado
    current = savedCurrent;
    line = savedLine;
    column = savedColumn;
    start = savedStart;
    startColumn = savedStartColumn;
    
    // Remove token adicionado
    if (!tokens.empty()) tokens.pop_back();
    
    return token;
}

} // namespace Kava
