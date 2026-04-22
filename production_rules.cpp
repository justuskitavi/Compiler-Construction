#include <string>
#include <vector>

#include "terminals.h"
#include "production_rules.h"

std::vector<Production> makeProductions()
{
    std::vector<Production> P(31); // 0-indexed, 1-30 used

    auto rule = [&](int n, std::string desc, std::vector<Symbol> rhs)
    {
        P[n] = {n, std::move(desc), std::move(rhs)};
    };

    //  1. program → BEGIN stmt_list END
    rule(1, "<program> → BEGIN <stmt_list> END",
         {T(T_BEGIN), NT(NT_STMT_LIST), T(T_END)});

    //  2. stmt_list → stmt stmt_list'
    rule(2, "<stmt_list> → <stmt> <stmt_list'>",
         {NT(NT_STMT), NT(NT_STMT_LIST_PRIME)});

    //  3. stmt_list' → stmt stmt_list'
    rule(3, "<stmt_list'> → <stmt> <stmt_list'>",
         {NT(NT_STMT), NT(NT_STMT_LIST_PRIME)});

    //  4. stmt_list' → ε
    rule(4, "<stmt_list'> → ε", {});

    //  5. stmt → assignment
    rule(5, "<stmt> → <assignment>", {NT(NT_ASSIGNMENT)});

    //  6. stmt → print_stmt
    rule(6, "<stmt> → <print_stmt>", {NT(NT_PRINT_STMT)});

    //  7. stmt → if_stmt
    rule(7, "<stmt> → <if_stmt>", {NT(NT_IF_STMT)});

    //  8. stmt → while_stmt
    rule(8, "<stmt> → <while_stmt>", {NT(NT_WHILE_STMT)});

    //  9. assignment → type id = expr ;
    rule(9, "<assignment> → <type> id = <expr> ;",
         {NT(NT_TYPE), T(T_ID), T(T_ASSIGN), NT(NT_EXPR), T(T_SCOLON)});

    // 10. assignment → id = expr ;
    rule(10, "<assignment> → id = <expr> ;",
         {T(T_ID), T(T_ASSIGN), NT(NT_EXPR), T(T_SCOLON)});

    // 11. print_stmt → cout << expr ;
    rule(11, "<print_stmt> → cout << <expr> ;",
         {T(T_COUT), T(T_OP_OUT), NT(NT_EXPR), T(T_SCOLON)});

    // 12. if_stmt → if ( expr ) { stmt_list } else_part
    rule(12, "<if_stmt> → if ( <expr> ) { <stmt_list> } <else_part>",
         {T(T_IF), T(T_LPAREN), NT(NT_EXPR), T(T_RPAREN),
          T(T_LBRACE), NT(NT_STMT_LIST), T(T_RBRACE), NT(NT_ELSE_PART)});

    // 13. else_part → else { stmt_list }
    rule(13, "<else_part> → else { <stmt_list> }",
         {T(T_ELSE), T(T_LBRACE), NT(NT_STMT_LIST), T(T_RBRACE)});

    // 14. else_part → ε
    rule(14, "<else_part> → ε", {});

    // 15. while_stmt → while ( expr ) { stmt_list }
    rule(15, "<while_stmt> → while ( <expr> ) { <stmt_list> }",
         {T(T_WHILE), T(T_LPAREN), NT(NT_EXPR), T(T_RPAREN),
          T(T_LBRACE), NT(NT_STMT_LIST), T(T_RBRACE)});

    // 16. expr → arith_expr expr'
    rule(16, "<expr> → <arith_expr> <expr'>",
         {NT(NT_ARITH_EXPR), NT(NT_EXPR_PRIME)});

    // 17. expr' → == arith_expr
    rule(17, "<expr'> → == <arith_expr>",
         {T(T_EQ), NT(NT_ARITH_EXPR)});

    // 18. expr' → < arith_expr
    rule(18, "<expr'> → < <arith_expr>",
         {T(T_LT), NT(NT_ARITH_EXPR)});

    // 19. expr' → > arith_expr
    rule(19, "<expr'> → > <arith_expr>",
         {T(T_GT), NT(NT_ARITH_EXPR)});

    // 20. expr' → ε
    rule(20, "<expr'> → ε", {});

    // 21. arith_expr → term arith_expr'
    rule(21, "<arith_expr> → <term> <arith_expr'>",
         {NT(NT_TERM), NT(NT_ARITH_EXPR_PRIME)});

    // 22. arith_expr' → + term arith_expr'
    rule(22, "<arith_expr'> → + <term> <arith_expr'>",
         {T(T_PLUS), NT(NT_TERM), NT(NT_ARITH_EXPR_PRIME)});

    // 23. arith_expr' → - term arith_expr'
    rule(23, "<arith_expr'> → - <term> <arith_expr'>",
         {T(T_MINUS), NT(NT_TERM), NT(NT_ARITH_EXPR_PRIME)});

    // 24. arith_expr' → ε
    rule(24, "<arith_expr'> → ε", {});

    // 25. term → id
    rule(25, "<term> → id", {T(T_ID)});

    // 26. term → integer
    rule(26, "<term> → integer", {T(T_INTEGER)});

    // 27. term → string_literal
    rule(27, "<term> → string_literal", {T(T_STRING_LITERAL)});

    // 28. term → ( expr )
    rule(28, "<term> → ( <expr> )",
         {T(T_LPAREN), NT(NT_EXPR), T(T_RPAREN)});

    // 29. type → int
    rule(29, "<type> → int", {T(T_INT)});

    // 30. type → string
    rule(30, "<type> → string", {T(T_STRING)});

    return P;
}
