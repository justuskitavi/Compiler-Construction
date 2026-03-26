%{
/*
 * scanner.l  —  Flex lexer for the mini grammar.
 *
 * Each rule emits a Token (type + lexeme), prints it to stdout,
 * and appends it to the global token list consumed by main.cpp.
 *
 * Build:
 *   flex -o scanner.cpp scanner.l
 *   g++ -std=c++17 -o scanner main.cpp scanner.cpp
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "tokens.h"

// Token list filled by the scanner; main.cpp writes it to file.
extern std::vector<Token> g_tokens;

// Helper: record a token, print it, and return its type.
static TokenType emit(TokenType t, const char* text) {
    std::string lexeme(text);
    g_tokens.emplace_back(t, lexeme);

    // Tokens with a meaningful lexeme get it printed in parentheses.
    switch (t) {
        case T_ID:
        case T_INTEGER:
        case T_STRING_LITERAL:
        case T_ERROR:
            std::cout << tokenTypeName(t) << "(" << lexeme << ") ";
            break;
        default:
            std::cout << tokenTypeName(t) << " ";
            break;
    }
    return t;
}
%}

%option noyywrap
%option yylineno

%%

"int"                   { emit(T_INT,            yytext); }
"if"                    { emit(T_IF,             yytext); }
"else"                  { emit(T_ELSE,           yytext); }
"cout"                  { emit(T_COUT,           yytext); }
"string"                { emit(T_STRING,         yytext); }
"while"                 { emit(T_WHILE,          yytext); }
"BEGIN"                 { emit(T_BEGIN,          yytext); }
"END"                   { emit(T_END,            yytext); }

[a-zA-Z_][a-zA-Z0-9_]* { emit(T_ID,             yytext); }
[0-9]+                  { emit(T_INTEGER,        yytext); }
\"[^\"]*\"              { emit(T_STRING_LITERAL, yytext); }

"=="                    { emit(T_EQ,             yytext); }
"<<"                    { emit(T_OP_OUT,         yytext); }
"+"                     { emit(T_PLUS,           yytext); }
"-"                     { emit(T_MINUS,          yytext); }
"="                     { emit(T_ASSIGN,         yytext); }
">"                     { emit(T_GT,             yytext); }
"<"                     { emit(T_LT,             yytext); }

";"                     { emit(T_SCOLON,         yytext); }
"{"                     { emit(T_LBRACE,         yytext); }
"}"                     { emit(T_RBRACE,         yytext); }
"("                     { emit(T_LPAREN,         yytext); }
")"                     { emit(T_RPAREN,         yytext); }

[ \t\n\r]+              { /* ignore whitespace */ }
.                       { std::cerr << "Lexical error at line " << yylineno
                  << ": unexpected character '" << yytext[0]
                  << "' ";
                     emit(T_ERROR, yytext); }

%%
