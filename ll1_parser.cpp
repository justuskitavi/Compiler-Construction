/*
 * parser_ll1.cpp — LL(1) Table-Driven Parser for the mini-language.
 *
 * Reads tokens from "tokens.out" (produced by the scanner), performs
 * syntax analysis using an explicit parse stack + LL(1) parse table,
 * and prints a parse trace to stdout.
 *
 * Grammar (from LL_1__Grammar_and_Parse_Table.xlsx):
 *   1.  program       → BEGIN stmt_list END
 *   2.  stmt_list     → stmt stmt_list'
 *   3.  stmt_list'    → stmt stmt_list'
 *   4.  stmt_list'    → ε
 *   5.  stmt          → assignment
 *   6.  stmt          → print_stmt
 *   7.  stmt          → if_stmt
 *   8.  stmt          → while_stmt
 *   9.  assignment    → type id = expr ;
 *  10.  assignment    → id = expr ;
 *  11.  print_stmt    → cout << expr ;
 *  12.  if_stmt       → if ( expr ) { stmt_list } else_part
 *  13.  else_part     → else { stmt_list }
 *  14.  else_part     → ε
 *  15.  while_stmt    → while ( expr ) { stmt_list }
 *  16.  expr          → arith_expr expr'
 *  17.  expr'         → == arith_expr
 *  18.  expr'         → < arith_expr
 *  19.  expr'         → > arith_expr
 *  20.  expr'         → ε
 *  21.  arith_expr    → term arith_expr'
 *  22.  arith_expr'   → + term arith_expr'
 *  23.  arith_expr'   → - term arith_expr'
 *  24.  arith_expr'   → ε
 *  25.  term          → id
 *  26.  term          → integer
 *  27.  term          → string_literal
 *  28.  term          → ( expr )
 *  29.  type          → int
 *  30.  type          → string
 *
 * Build:
 *   g++ -std=c++17 -o parser_ll1 parser_ll1.cpp
 *
 * Run:
 *   ./parser_ll1              (reads tokens.out by default)
 *   ./parser_ll1 my.tokens    (reads a custom token file)
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <map>
#include <stdexcept>
#include <cassert>

// ─────────────────────────────────────────────
//  Terminals  (token types)
// ─────────────────────────────────────────────

enum Terminal {
    // Keywords
    T_BEGIN, T_END,
    T_INT, T_STRING_KW,       // "int" and "string" as type keywords
    T_WHILE, T_IF, T_ELSE, T_COUT,
    // Values
    T_ID, T_INTEGER, T_STRING_LITERAL,
    // Operators / punctuation
    T_ASSIGN,     // =
    T_EQ,         // ==
    T_GT,         // >
    T_LT,         // <
    T_PLUS,       // +
    T_MINUS,      // -
    T_OP_OUT,     // <<
    T_SCOLON,     // ;
    T_LBRACE,     // {
    T_RBRACE,     // }
    T_LPAREN,     // (
    T_RPAREN,     // )
    T_EOF         // $
};

// ─────────────────────────────────────────────
//  Non-terminals
// ─────────────────────────────────────────────

enum NonTerminal {
    NT_PROGRAM,
    NT_STMT_LIST,
    NT_STMT_LIST_PRIME,
    NT_STMT,
    NT_ASSIGNMENT,
    NT_PRINT_STMT,
    NT_IF_STMT,
    NT_ELSE_PART,
    NT_WHILE_STMT,
    NT_EXPR,
    NT_EXPR_PRIME,
    NT_ARITH_EXPR,
    NT_ARITH_EXPR_PRIME,
    NT_TERM,
    NT_TYPE,
    _NT_COUNT
};

// ─────────────────────────────────────────────
//  Grammar symbol (terminal OR non-terminal)
// ─────────────────────────────────────────────

struct Symbol {
    bool isTerminal;
    union { Terminal t; NonTerminal nt; };

    static Symbol term(Terminal t)       { Symbol s; s.isTerminal = true;  s.t  = t;  return s; }
    static Symbol nonterm(NonTerminal n) { Symbol s; s.isTerminal = false; s.nt = n;  return s; }
};

// Convenience aliases
static Symbol T(Terminal t)       { return Symbol::term(t); }
static Symbol NT(NonTerminal nt)  { return Symbol::nonterm(nt); }

// ─────────────────────────────────────────────
//  Token (from scanner output)
// ─────────────────────────────────────────────

struct Token {
    Terminal    type;
    std::string lexeme;
};

// ─────────────────────────────────────────────
//  Names (for trace output)
// ─────────────────────────────────────────────

static const char* termName(Terminal t) {
    switch (t) {
        case T_BEGIN:          return "BEGIN";
        case T_END:            return "END";
        case T_INT:            return "int";
        case T_STRING_KW:      return "string";
        case T_WHILE:          return "while";
        case T_IF:             return "if";
        case T_ELSE:           return "else";
        case T_COUT:           return "cout";
        case T_ID:             return "id";
        case T_INTEGER:        return "integer";
        case T_STRING_LITERAL: return "string_literal";
        case T_ASSIGN:         return "=";
        case T_EQ:             return "==";
        case T_GT:             return ">";
        case T_LT:             return "<";
        case T_PLUS:           return "+";
        case T_MINUS:          return "-";
        case T_OP_OUT:         return "<<";
        case T_SCOLON:         return ";";
        case T_LBRACE:         return "{";
        case T_RBRACE:         return "}";
        case T_LPAREN:         return "(";
        case T_RPAREN:         return ")";
        case T_EOF:            return "$";
    }
    return "?";
}

static const char* ntName(NonTerminal nt) {
    switch (nt) {
        case NT_PROGRAM:         return "<program>";
        case NT_STMT_LIST:       return "<stmt_list>";
        case NT_STMT_LIST_PRIME: return "<stmt_list'>";
        case NT_STMT:            return "<stmt>";
        case NT_ASSIGNMENT:      return "<assignment>";
        case NT_PRINT_STMT:      return "<print_stmt>";
        case NT_IF_STMT:         return "<if_stmt>";
        case NT_ELSE_PART:       return "<else_part>";
        case NT_WHILE_STMT:      return "<while_stmt>";
        case NT_EXPR:            return "<expr>";
        case NT_EXPR_PRIME:      return "<expr'>";
        case NT_ARITH_EXPR:      return "<arith_expr>";
        case NT_ARITH_EXPR_PRIME:return "<arith_expr'>";
        case NT_TERM:            return "<term>";
        case NT_TYPE:            return "<type>";
        default:                 return "<??>";
    }
    return "<??>";
}

// ─────────────────────────────────────────────
//  Production rules
// ─────────────────────────────────────────────

struct Production {
    int                 number;          // rule number (1-30)
    std::string         description;     // human-readable RHS
    std::vector<Symbol> rhs;            // symbols to push (right-to-left on stack)
};

// Build the full production table indexed 1..30 (index 0 unused)
static std::vector<Production> makeProductions() {
    std::vector<Production> P(31);  // 0-indexed, 1-30 used

    auto rule = [&](int n, std::string desc, std::vector<Symbol> rhs) {
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
    rule(30, "<type> → string", {T(T_STRING_KW)});

    return P;
}

// ─────────────────────────────────────────────
//  LL(1) Parse Table
//  table[NT][terminal] = production number (0 = no entry / error)
// ─────────────────────────────────────────────

static std::map<NonTerminal, std::map<Terminal, int>> buildParseTable() {
    std::map<NonTerminal, std::map<Terminal, int>> T;

    // Helper lambda
    auto add = [&](NonTerminal nt, Terminal tok, int prod) {
        T[nt][tok] = prod;
    };

    // ── program ──────────────────────────────────────────────────────
    add(NT_PROGRAM, T_BEGIN, 1);

    // ── stmt_list ────────────────────────────────────────────────────
    add(NT_STMT_LIST, T_INT,      2);
    add(NT_STMT_LIST, T_STRING_KW,2);
    add(NT_STMT_LIST, T_ID,       2);
    add(NT_STMT_LIST, T_COUT,     2);
    add(NT_STMT_LIST, T_IF,       2);
    add(NT_STMT_LIST, T_WHILE,    2);

    // ── stmt_list' ───────────────────────────────────────────────────
    add(NT_STMT_LIST_PRIME, T_INT,      3);
    add(NT_STMT_LIST_PRIME, T_STRING_KW,3);
    add(NT_STMT_LIST_PRIME, T_ID,       3);
    add(NT_STMT_LIST_PRIME, T_COUT,     3);
    add(NT_STMT_LIST_PRIME, T_IF,       3);
    add(NT_STMT_LIST_PRIME, T_WHILE,    3);
    add(NT_STMT_LIST_PRIME, T_END,      4);   // FOLLOW: END
    add(NT_STMT_LIST_PRIME, T_RBRACE,   4);   // FOLLOW: }

    // ── stmt ─────────────────────────────────────────────────────────
    add(NT_STMT, T_INT,      5);
    add(NT_STMT, T_STRING_KW,5);
    add(NT_STMT, T_ID,       5);   // → assignment (rule 10: id = ...)
    add(NT_STMT, T_COUT,     6);
    add(NT_STMT, T_IF,       7);
    add(NT_STMT, T_WHILE,    8);

    // ── assignment ───────────────────────────────────────────────────
    add(NT_ASSIGNMENT, T_INT,      9);
    add(NT_ASSIGNMENT, T_STRING_KW,9);
    add(NT_ASSIGNMENT, T_ID,      10);

    // ── print_stmt ───────────────────────────────────────────────────
    add(NT_PRINT_STMT, T_COUT, 11);

    // ── if_stmt ──────────────────────────────────────────────────────
    add(NT_IF_STMT, T_IF, 12);

    // ── else_part ────────────────────────────────────────────────────
    add(NT_ELSE_PART, T_ELSE,     13);
    // FOLLOW(else_part): BEGIN excluded; tokens after '}' that close an if body
    add(NT_ELSE_PART, T_END,      14);
    add(NT_ELSE_PART, T_INT,      14);
    add(NT_ELSE_PART, T_STRING_KW,14);
    add(NT_ELSE_PART, T_ID,       14);
    add(NT_ELSE_PART, T_COUT,     14);
    add(NT_ELSE_PART, T_IF,       14);
    add(NT_ELSE_PART, T_WHILE,    14);
    add(NT_ELSE_PART, T_RBRACE,   14);

    // ── while_stmt ───────────────────────────────────────────────────
    add(NT_WHILE_STMT, T_WHILE, 15);

    // ── expr ─────────────────────────────────────────────────────────
    add(NT_EXPR, T_ID,             16);
    add(NT_EXPR, T_INTEGER,        16);
    add(NT_EXPR, T_STRING_LITERAL, 16);
    add(NT_EXPR, T_LPAREN,         16);

    // ── expr' ────────────────────────────────────────────────────────
    add(NT_EXPR_PRIME, T_EQ,      17);
    add(NT_EXPR_PRIME, T_LT,      18);
    add(NT_EXPR_PRIME, T_GT,      19);
    // FOLLOW(expr'): ; ) }
    add(NT_EXPR_PRIME, T_SCOLON,  20);
    add(NT_EXPR_PRIME, T_RPAREN,  20);
    add(NT_EXPR_PRIME, T_RBRACE,  20);

    // ── arith_expr ───────────────────────────────────────────────────
    add(NT_ARITH_EXPR, T_ID,             21);
    add(NT_ARITH_EXPR, T_INTEGER,        21);
    add(NT_ARITH_EXPR, T_STRING_LITERAL, 21);
    add(NT_ARITH_EXPR, T_LPAREN,         21);

    // ── arith_expr' ──────────────────────────────────────────────────
    add(NT_ARITH_EXPR_PRIME, T_PLUS,   22);
    add(NT_ARITH_EXPR_PRIME, T_MINUS,  23);
    // FOLLOW(arith_expr'): == < > ; ) }
    add(NT_ARITH_EXPR_PRIME, T_EQ,     24);
    add(NT_ARITH_EXPR_PRIME, T_LT,     24);
    add(NT_ARITH_EXPR_PRIME, T_GT,     24);
    add(NT_ARITH_EXPR_PRIME, T_SCOLON, 24);
    add(NT_ARITH_EXPR_PRIME, T_RPAREN, 24);
    add(NT_ARITH_EXPR_PRIME, T_RBRACE, 24);

    // ── term ─────────────────────────────────────────────────────────
    add(NT_TERM, T_ID,             25);
    add(NT_TERM, T_INTEGER,        26);
    add(NT_TERM, T_STRING_LITERAL, 27);
    add(NT_TERM, T_LPAREN,         28);

    // ── type ─────────────────────────────────────────────────────────
    add(NT_TYPE, T_INT,      29);
    add(NT_TYPE, T_STRING_KW,30);

    return T;
}

// ─────────────────────────────────────────────
//  Token file loader
// ─────────────────────────────────────────────

static Terminal nameToTerminal(const std::string& name) {
    if (name == "T_BEGIN")          return T_BEGIN;
    if (name == "T_END")            return T_END;
    if (name == "T_INT")            return T_INT;
    if (name == "T_STRING")         return T_STRING_KW;
    if (name == "T_WHILE")          return T_WHILE;
    if (name == "T_IF")             return T_IF;
    if (name == "T_ELSE")           return T_ELSE;
    if (name == "T_COUT")           return T_COUT;
    if (name == "T_ID")             return T_ID;
    if (name == "T_INTEGER")        return T_INTEGER;
    if (name == "T_STRING_LITERAL") return T_STRING_LITERAL;
    if (name == "T_ASSIGN")         return T_ASSIGN;
    if (name == "T_EQ")             return T_EQ;
    if (name == "T_GT")             return T_GT;
    if (name == "T_LT")             return T_LT;
    if (name == "T_PLUS")           return T_PLUS;
    if (name == "T_MINUS")          return T_MINUS;
    if (name == "T_OP_OUT")         return T_OP_OUT;
    if (name == "T_SCOLON")         return T_SCOLON;
    if (name == "T_LBRACE")         return T_LBRACE;
    if (name == "T_RBRACE")         return T_RBRACE;
    if (name == "T_LPAREN")         return T_LPAREN;
    if (name == "T_RPAREN")         return T_RPAREN;
    // T_ERROR → sentinel for "skip"
    return static_cast<Terminal>(-1);
}

static std::vector<Token> loadTokens(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open())
        throw std::runtime_error("Cannot open token file: " + path);

    std::vector<Token> tokens;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        auto paren = line.find('(');
        std::string typeName = (paren != std::string::npos)
                               ? line.substr(0, paren) : line;

        while (!typeName.empty() &&
               (typeName.back() == '\r' || typeName.back() == ' '))
            typeName.pop_back();

        Terminal tt = nameToTerminal(typeName);
        if (static_cast<int>(tt) == -1) continue;   // skip T_ERROR etc.

        std::string lexeme;
        if (paren != std::string::npos) {
            auto close = line.rfind(')');
            if (close != std::string::npos && close > paren)
                lexeme = line.substr(paren + 1, close - paren - 1);
        }
        tokens.push_back({tt, lexeme});
    }
    tokens.push_back({T_EOF, "$"});
    return tokens;
}

// ─────────────────────────────────────────────
//  Table-Driven LL(1) Parser
// ─────────────────────────────────────────────

class LL1Parser {
public:
    LL1Parser(std::vector<Token> toks,
              std::vector<Production> prods,
              std::map<NonTerminal, std::map<Terminal, int>> table)
        : tokens_(std::move(toks)),
          prods_(std::move(prods)),
          table_(std::move(table)),
          pos_(0) {}

    void parse() {
        // Initialise stack: push $ then the start symbol
        stack_.push(Symbol::term(T_EOF));
        stack_.push(Symbol::nonterm(NT_PROGRAM));

        while (true) {
            Symbol top = stack_.top();
            const Token& cur = tokens_[pos_];

            // ── Both are $: accept ───────────────────────────────────
            if (top.isTerminal && top.t == T_EOF && cur.type == T_EOF) {
                std::cout << "\n[Parser] SUCCESS — program is syntactically valid.\n";
                return;
            }

            // ── Top is terminal ──────────────────────────────────────
            if (top.isTerminal) {
                if (top.t == cur.type) {
                    // Match: consume token
                    std::string lex = cur.lexeme.empty()
                                      ? termName(cur.type)
                                      : termName(cur.type) + std::string("(") + cur.lexeme + ")";
                    std::cout << "  match   " << lex << "\n";
                    stack_.pop();
                    ++pos_;
                } else {
                    std::string got = cur.lexeme.empty()
                                      ? termName(cur.type)
                                      : std::string(termName(cur.type)) + "(" + cur.lexeme + ")";
                    throw std::runtime_error(
                        std::string("[Syntax error] expected '") + termName(top.t) +
                        "', got '" + got + "'");
                }
                continue;
            }

            // ── Top is non-terminal ──────────────────────────────────
            NonTerminal nt = top.nt;
            Terminal    a  = cur.type;

            auto ntIt = table_.find(nt);
            if (ntIt == table_.end() || ntIt->second.find(a) == ntIt->second.end()) {
                std::string got = cur.lexeme.empty()
                                  ? termName(a)
                                  : std::string(termName(a)) + "(" + cur.lexeme + ")";
                throw std::runtime_error(
                    std::string("[Syntax error] no rule for ") + ntName(nt) +
                    " on input '" + got + "'");
            }

            int prodNum = ntIt->second.at(a);
            const Production& prod = prods_[prodNum];

            // Trace the production being applied
            std::cout << "  rule " << prodNum
                      << (prodNum < 10 ? " " : "")   // align single-digit numbers
                      << "  " << prod.description << "\n";

            stack_.pop();

            // Push RHS symbols in reverse order (so left-most is on top)
            for (int i = static_cast<int>(prod.rhs.size()) - 1; i >= 0; --i)
                stack_.push(prod.rhs[i]);
        }
    }

private:
    std::vector<Token>                               tokens_;
    std::vector<Production>                          prods_;
    std::map<NonTerminal, std::map<Terminal, int>>   table_;
    std::stack<Symbol>                               stack_;
    size_t                                           pos_;
};

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::string tokenFile = (argc > 1) ? argv[1] : "tokens.out";

    std::vector<Token> tokens;
    try {
        tokens = loadTokens(tokenFile);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::cout << "[Parser] Loaded " << tokens.size() - 1
              << " token(s) from '" << tokenFile << "'\n\n";
    std::cout << "=== Parse Trace ===\n";

    auto prods = makeProductions();
    auto table = buildParseTable();

    try {
        LL1Parser parser(std::move(tokens), std::move(prods), std::move(table));
        parser.parse();
    } catch (const std::exception& e) {
        std::cerr << "\n" << e.what() << "\n";
        return 1;
    }

    return 0;
}