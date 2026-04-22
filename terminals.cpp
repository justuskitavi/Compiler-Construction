#include <string>
#include "terminals.h"

const char *termName(Terminal t)
{
    switch (t)
    {
        case T_BEGIN:
            return "BEGIN";
        case T_END:
            return "END";
        case T_INT:
            return "int";
        case T_STRING:
            return "string";
        case T_WHILE:
            return "while";
        case T_IF:
            return "if";
        case T_ELSE:
            return "else";
        case T_COUT:
            return "cout";
        case T_ID:
            return "id";
        case T_INTEGER:
            return "integer";
        case T_STRING_LITERAL:
            return "string_literal";
        case T_ASSIGN:
            return "=";
        case T_EQ:
            return "==";
        case T_GT:
            return ">";
        case T_LT:
            return "<";
        case T_PLUS:
            return "+";
        case T_MINUS:
            return "-";
        case T_OP_OUT:
            return "<<";
        case T_SCOLON:
            return ";";
        case T_LBRACE:
            return "{";
        case T_RBRACE:
            return "}";
        case T_LPAREN:
            return "(";
        case T_RPAREN:
            return ")";
        case T_EOF:
            return "$";
    }
    return "?";
}

const char *ntName(NonTerminal nt)
{
    switch (nt)
    {
        case NT_PROGRAM:
            return "<program>";
        case NT_STMT_LIST:
            return "<stmt_list>";
        case NT_STMT_LIST_PRIME:
            return "<stmt_list'>";
        case NT_STMT:
            return "<stmt>";
        case NT_ASSIGNMENT:
            return "<assignment>";
        case NT_PRINT_STMT:
            return "<print_stmt>";
        case NT_IF_STMT:
            return "<if_stmt>";
        case NT_ELSE_PART:
            return "<else_part>";
        case NT_WHILE_STMT:
            return "<while_stmt>";
        case NT_EXPR:
            return "<expr>";
        case NT_EXPR_PRIME:
            return "<expr'>";
        case NT_ARITH_EXPR:
            return "<arith_expr>";
        case NT_ARITH_EXPR_PRIME:
            return "<arith_expr'>";
        case NT_TERM:
            return "<term>";
        case NT_TYPE:
            return "<type>";
        default:
            return "<??>";
    }
}

Terminal nameToTerminal(const std::string &name)
{
    if (name == "T_BEGIN")
        return T_BEGIN;
    if (name == "T_END")
        return T_END;
    if (name == "T_INT")
        return T_INT;
    if (name == "T_STRING")
        return T_STRING;
    if (name == "T_WHILE")
        return T_WHILE;
    if (name == "T_IF")
        return T_IF;
    if (name == "T_ELSE")
        return T_ELSE;
    if (name == "T_COUT")
        return T_COUT;
    if (name == "T_ID")
        return T_ID;
    if (name == "T_INTEGER")
        return T_INTEGER;
    if (name == "T_STRING_LITERAL")
        return T_STRING_LITERAL;
    if (name == "T_ASSIGN")
        return T_ASSIGN;
    if (name == "T_EQ")
        return T_EQ;
    if (name == "T_GT")
        return T_GT;
    if (name == "T_LT")
        return T_LT;
    if (name == "T_PLUS")
        return T_PLUS;
    if (name == "T_MINUS")
        return T_MINUS;
    if (name == "T_OP_OUT")
        return T_OP_OUT;
    if (name == "T_SCOLON")
        return T_SCOLON;
    if (name == "T_LBRACE")
        return T_LBRACE;
    if (name == "T_RBRACE")
        return T_RBRACE;
    if (name == "T_LPAREN")
        return T_LPAREN;
    if (name == "T_RPAREN")
        return T_RPAREN;

    return static_cast<Terminal>(-1);
}