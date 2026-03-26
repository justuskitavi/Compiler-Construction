#ifndef TOKENS_H
#define TOKENS_H

#include <string>

// Enum of all token types recognized by the scanner.
// Used by both the scanner (scanner.l) and the parser (future).
enum TokenType {
    // Keywords
    T_INT,
    T_IF,
    T_ELSE,
    T_COUT,
    T_STRING,
    T_WHILE,
    T_BEGIN,
    T_END,

    // Identifiers and literals
    T_ID,
    T_INTEGER,
    T_STRING_LITERAL,

    // Operators
    T_EQ,         // ==
    T_OP_OUT,     // <<
    T_PLUS,       // +
    T_MINUS,      // -
    T_ASSIGN,     // =
    T_GT,         // >
    T_LT,         // <

    // Punctuation
    T_SCOLON,     // ;
    T_LBRACE,     // {
    T_RBRACE,     // }
    T_LPAREN,     // (
    T_RPAREN,     // )

    // Special
    T_ERROR,
    T_EOF
};

// Returns a human-readable name for a given token type.
// Useful for both debug output and the parser's error messages.
inline std::string tokenTypeName(TokenType t) {
    switch (t) {
        case T_INT:            return "T_INT";
        case T_IF:             return "T_IF";
        case T_ELSE:           return "T_ELSE";
        case T_COUT:           return "T_COUT";
        case T_STRING:         return "T_STRING";
        case T_WHILE:          return "T_WHILE";
        case T_BEGIN:          return "T_BEGIN";
        case T_END:            return "T_END";
        case T_ID:             return "T_ID";
        case T_INTEGER:        return "T_INTEGER";
        case T_STRING_LITERAL: return "T_STRING_LITERAL";
        case T_EQ:             return "T_EQ";
        case T_OP_OUT:         return "T_OP_OUT";
        case T_PLUS:           return "T_PLUS";
        case T_MINUS:          return "T_MINUS";
        case T_ASSIGN:         return "T_ASSIGN";
        case T_GT:             return "T_GT";
        case T_LT:             return "T_LT";
        case T_SCOLON:         return "T_SCOLON";
        case T_LBRACE:         return "T_LBRACE";
        case T_RBRACE:         return "T_RBRACE";
        case T_LPAREN:         return "T_LPAREN";
        case T_RPAREN:         return "T_RPAREN";
        case T_ERROR:        return "T_ERROR";
        case T_EOF:            return "T_EOF";
        default:               return "T_INVALID";
    }
}

// A token bundles a type with its lexeme (the raw matched text).
// The parser will consume a stream of these.
struct Token {
    TokenType   type;
    std::string lexeme;

    Token(TokenType t, const std::string& l) : type(t), lexeme(l) {}
};

#endif // TOKENS_H