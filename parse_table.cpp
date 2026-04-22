#include <map>

#include "parse_table.h"
#include "terminals.h"

std::map<NonTerminal, std::map<Terminal, int>> buildParseTable()
{
    std::map<NonTerminal, std::map<Terminal, int>> T;

    // Helper lambda
    auto add = [&](NonTerminal nt, Terminal tok, int prod)
    {
        T[nt][tok] = prod;
    };

    // ── program ──────────────────────────────────────────────────────
    add(NT_PROGRAM, T_BEGIN, 1);

    // ── stmt_list ────────────────────────────────────────────────────
    add(NT_STMT_LIST, T_INT, 2);
    add(NT_STMT_LIST, T_STRING, 2);
    add(NT_STMT_LIST, T_ID, 2);
    add(NT_STMT_LIST, T_COUT, 2);
    add(NT_STMT_LIST, T_IF, 2);
    add(NT_STMT_LIST, T_WHILE, 2);

    // ── stmt_list' ───────────────────────────────────────────────────
    add(NT_STMT_LIST_PRIME, T_INT, 3);
    add(NT_STMT_LIST_PRIME, T_STRING, 3);
    add(NT_STMT_LIST_PRIME, T_ID, 3);
    add(NT_STMT_LIST_PRIME, T_COUT, 3);
    add(NT_STMT_LIST_PRIME, T_IF, 3);
    add(NT_STMT_LIST_PRIME, T_WHILE, 3);
    add(NT_STMT_LIST_PRIME, T_END, 4);    // FOLLOW: END
    add(NT_STMT_LIST_PRIME, T_RBRACE, 4); // FOLLOW: }

    // ── stmt ─────────────────────────────────────────────────────────
    add(NT_STMT, T_INT, 5);
    add(NT_STMT, T_STRING, 5);
    add(NT_STMT, T_ID, 5); // → assignment (rule 10: id = ...)
    add(NT_STMT, T_COUT, 6);
    add(NT_STMT, T_IF, 7);
    add(NT_STMT, T_WHILE, 8);

    // ── assignment ───────────────────────────────────────────────────
    add(NT_ASSIGNMENT, T_INT, 9);
    add(NT_ASSIGNMENT, T_STRING, 9);
    add(NT_ASSIGNMENT, T_ID, 10);

    // ── print_stmt ───────────────────────────────────────────────────
    add(NT_PRINT_STMT, T_COUT, 11);

    // ── if_stmt ──────────────────────────────────────────────────────
    add(NT_IF_STMT, T_IF, 12);

    // ── else_part ────────────────────────────────────────────────────
    add(NT_ELSE_PART, T_ELSE, 13);
    // FOLLOW(else_part): BEGIN excluded; tokens after '}' that close an if body
    add(NT_ELSE_PART, T_END, 14);
    add(NT_ELSE_PART, T_INT, 14);
    add(NT_ELSE_PART, T_STRING, 14);
    add(NT_ELSE_PART, T_ID, 14);
    add(NT_ELSE_PART, T_COUT, 14);
    add(NT_ELSE_PART, T_IF, 14);
    add(NT_ELSE_PART, T_WHILE, 14);
    add(NT_ELSE_PART, T_RBRACE, 14);

    // ── while_stmt ───────────────────────────────────────────────────
    add(NT_WHILE_STMT, T_WHILE, 15);

    // ── expr ─────────────────────────────────────────────────────────
    add(NT_EXPR, T_ID, 16);
    add(NT_EXPR, T_INTEGER, 16);
    add(NT_EXPR, T_STRING_LITERAL, 16);
    add(NT_EXPR, T_LPAREN, 16);

    // ── expr' ────────────────────────────────────────────────────────
    add(NT_EXPR_PRIME, T_EQ, 17);
    add(NT_EXPR_PRIME, T_LT, 18);
    add(NT_EXPR_PRIME, T_GT, 19);
    // FOLLOW(expr'): ; ) }
    add(NT_EXPR_PRIME, T_SCOLON, 20);
    add(NT_EXPR_PRIME, T_RPAREN, 20);
    add(NT_EXPR_PRIME, T_RBRACE, 20);

    // ── arith_expr ───────────────────────────────────────────────────
    add(NT_ARITH_EXPR, T_ID, 21);
    add(NT_ARITH_EXPR, T_INTEGER, 21);
    add(NT_ARITH_EXPR, T_STRING_LITERAL, 21);
    add(NT_ARITH_EXPR, T_LPAREN, 21);

    // ── arith_expr' ──────────────────────────────────────────────────
    add(NT_ARITH_EXPR_PRIME, T_PLUS, 22);
    add(NT_ARITH_EXPR_PRIME, T_MINUS, 23);
    // FOLLOW(arith_expr'): == < > ; ) }
    add(NT_ARITH_EXPR_PRIME, T_EQ, 24);
    add(NT_ARITH_EXPR_PRIME, T_LT, 24);
    add(NT_ARITH_EXPR_PRIME, T_GT, 24);
    add(NT_ARITH_EXPR_PRIME, T_SCOLON, 24);
    add(NT_ARITH_EXPR_PRIME, T_RPAREN, 24);
    add(NT_ARITH_EXPR_PRIME, T_RBRACE, 24);

    // ── term ─────────────────────────────────────────────────────────
    add(NT_TERM, T_ID, 25);
    add(NT_TERM, T_INTEGER, 26);
    add(NT_TERM, T_STRING_LITERAL, 27);
    add(NT_TERM, T_LPAREN, 28);

    // ── type ─────────────────────────────────────────────────────────
    add(NT_TYPE, T_INT, 29);
    add(NT_TYPE, T_STRING, 30);

    return T;
}