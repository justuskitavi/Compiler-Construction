/*
 * parser.cpp — LL(1) Recursive Descent Parser for the mini-language.
 *
 * Reads tokens from "tokens.out" (produced by the scanner), performs
 * syntax analysis, and prints a parse trace to stdout.
 *
 * Grammar:
 *   Program    → BEGIN StmtList END
 *   StmtList   → Stmt StmtList | ε
 *   Stmt       → DeclStmt | AssignStmt | WhileStmt | CoutStmt
 *   DeclStmt   → Type id = Expr ;
 *   Type       → int | string
 *   AssignStmt → id = Expr ;
 *   WhileStmt  → while ( Condition ) { StmtList }
 *   CoutStmt   → cout << Expr ;
 *   Condition  → Expr RelOp Expr
 *   RelOp      → > | < | ==
 *   Expr       → Term ExprTail
 *   ExprTail   → + Term ExprTail | - Term ExprTail | ε
 *   Term       → id | integer | string_literal
 *
 * Build:
 *   g++ -std=c++17 -o parser parser.cpp
 *
 * Run:
 *   ./parser              (reads tokens.out by default)
 *   ./parser my.tokens    (reads a custom token file)
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

// ─────────────────────────────────────────────
//  Token representation
// ─────────────────────────────────────────────

enum TokenType {
    T_BEGIN, T_END,
    T_INT, T_STRING, T_WHILE, T_IF, T_ELSE, T_COUT,
    T_ID, T_INTEGER, T_STRING_LITERAL,
    T_ASSIGN, T_EQ, T_GT, T_LT,
    T_PLUS, T_MINUS,
    T_OP_OUT,
    T_SCOLON, T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN,
    T_ERROR,
    T_EOF
};

struct Token {
    TokenType   type;
    std::string lexeme;
};

// ─────────────────────────────────────────────
//  Token file loader
//  Parses lines like:  T_INT  or  T_ID(salary)
// ─────────────────────────────────────────────

static TokenType nameToType(const std::string& name) {
    if (name == "T_BEGIN")          return T_BEGIN;
    if (name == "T_END")            return T_END;
    if (name == "T_INT")            return T_INT;
    if (name == "T_STRING")         return T_STRING;
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
    if (name == "T_ERROR")          return T_ERROR;
    return T_ERROR;
}

static std::vector<Token> loadTokens(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open())
        throw std::runtime_error("Cannot open token file: " + path);

    std::vector<Token> tokens;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        // Skip T_ERROR tokens (lexical errors already reported by scanner)
        auto paren = line.find('(');
        std::string typeName = (paren != std::string::npos)
                                ? line.substr(0, paren)
                                : line;

        // Trim trailing whitespace/carriage-return
        while (!typeName.empty() && (typeName.back() == '\r' || typeName.back() == ' '))
            typeName.pop_back();

        TokenType tt = nameToType(typeName);
        if (tt == T_ERROR) continue;   // skip bad tokens

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
//  Parser
// ─────────────────────────────────────────────

class Parser {
public:
    explicit Parser(std::vector<Token> toks)
        : tokens_(std::move(toks)), pos_(0), depth_(0) {}

    void parse() {
        parseProgram();
        if (current().type != T_EOF)
            error("unexpected tokens after END");
        std::cout << "\n[Parser] SUCCESS — program is syntactically valid.\n";
    }

private:
    std::vector<Token> tokens_;
    size_t             pos_;
    int                depth_;   // indentation for trace

    // ── helpers ──────────────────────────────

    const Token& current() const { return tokens_[pos_]; }

    // Print a trace line with indentation
    void trace(const std::string& msg) {
        std::cout << std::string(depth_ * 2, ' ') << msg << "\n";
    }

    // Consume the current token if it matches, otherwise throw
    Token expect(TokenType t, const std::string& desc) {
        if (current().type != t) {
            std::string got = current().lexeme.empty()
                              ? "(token #" + std::to_string(pos_) + ")"
                              : "'" + current().lexeme + "'";
            error("expected " + desc + ", got " + got);
        }
        Token tok = current();
        ++pos_;
        return tok;
    }

    [[noreturn]] void error(const std::string& msg) {
        throw std::runtime_error("[Syntax error] " + msg);
    }

    // ── grammar rules ────────────────────────

    // Program → BEGIN StmtList END
    void parseProgram() {
        trace("Program → BEGIN StmtList END");
        ++depth_;
        expect(T_BEGIN, "BEGIN");
        parseStmtList();
        expect(T_END, "END");
        --depth_;
    }

    // StmtList → Stmt StmtList | ε
    void parseStmtList() {
        trace("StmtList");
        ++depth_;
        while (isStmtFirst(current().type)) {
            parseStmt();
        }
        --depth_;
    }

    // Check FIRST(Stmt)
    static bool isStmtFirst(TokenType t) {
        return t == T_INT    || t == T_STRING ||
               t == T_ID     || t == T_WHILE  || t == T_COUT;
    }

    // Stmt → DeclStmt | AssignStmt | WhileStmt | CoutStmt
    void parseStmt() {
        trace("Stmt");
        ++depth_;
        switch (current().type) {
            case T_INT:
            case T_STRING:
                parseDeclStmt();   break;
            case T_ID:
                parseAssignStmt(); break;
            case T_WHILE:
                parseWhileStmt();  break;
            case T_COUT:
                parseCoutStmt();   break;
            default:
                error("expected a statement");
        }
        --depth_;
    }

    // DeclStmt → Type id = Expr ;
    void parseDeclStmt() {
        trace("DeclStmt → Type id = Expr ;");
        ++depth_;
        parseType();
        Token id = expect(T_ID, "identifier");
        trace("id: " + id.lexeme);
        expect(T_ASSIGN, "'='");
        parseExpr();
        expect(T_SCOLON, "';'");
        --depth_;
    }

    // Type → int | string
    void parseType() {
        if (current().type == T_INT) {
            trace("Type → int");
            ++pos_;
        } else if (current().type == T_STRING) {
            trace("Type → string");
            ++pos_;
        } else {
            error("expected type keyword (int or string)");
        }
    }

    // AssignStmt → id = Expr ;
    void parseAssignStmt() {
        trace("AssignStmt → id = Expr ;");
        ++depth_;
        Token id = expect(T_ID, "identifier");
        trace("id: " + id.lexeme);
        expect(T_ASSIGN, "'='");
        parseExpr();
        expect(T_SCOLON, "';'");
        --depth_;
    }

    // WhileStmt → while ( Condition ) { StmtList }
    void parseWhileStmt() {
        trace("WhileStmt → while ( Condition ) { StmtList }");
        ++depth_;
        expect(T_WHILE,  "'while'");
        expect(T_LPAREN, "'('");
        parseCondition();
        expect(T_RPAREN, "')'");
        expect(T_LBRACE, "'{'");
        parseStmtList();
        expect(T_RBRACE, "'}'");
        --depth_;
    }

    // CoutStmt → cout << Expr ;
    void parseCoutStmt() {
        trace("CoutStmt → cout << Expr ;");
        ++depth_;
        expect(T_COUT,   "'cout'");
        expect(T_OP_OUT, "'<<'");
        parseExpr();
        expect(T_SCOLON, "';'");
        --depth_;
    }

    // Condition → Expr RelOp Expr
    void parseCondition() {
        trace("Condition → Expr RelOp Expr");
        ++depth_;
        parseExpr();
        parseRelOp();
        parseExpr();
        --depth_;
    }

    // RelOp → > | < | ==
    void parseRelOp() {
        switch (current().type) {
            case T_GT: trace("RelOp → >"); ++pos_; break;
            case T_LT: trace("RelOp → <"); ++pos_; break;
            case T_EQ: trace("RelOp → =="); ++pos_; break;
            default:   error("expected relational operator (>, <, ==)");
        }
    }

    // Expr → Term ExprTail
    void parseExpr() {
        trace("Expr → Term ExprTail");
        ++depth_;
        parseTerm();
        parseExprTail();
        --depth_;
    }

    // ExprTail → + Term ExprTail | - Term ExprTail | ε
    void parseExprTail() {
        if (current().type == T_PLUS) {
            trace("ExprTail → + Term ExprTail");
            ++depth_;
            ++pos_;
            parseTerm();
            parseExprTail();
            --depth_;
        } else if (current().type == T_MINUS) {
            trace("ExprTail → - Term ExprTail");
            ++depth_;
            ++pos_;
            parseTerm();
            parseExprTail();
            --depth_;
        } else {
            trace("ExprTail → ε");
        }
    }

    // Term → id | integer | string_literal
    void parseTerm() {
        switch (current().type) {
            case T_ID:
                trace("Term → id(" + current().lexeme + ")");
                ++pos_; break;
            case T_INTEGER:
                trace("Term → integer(" + current().lexeme + ")");
                ++pos_; break;
            case T_STRING_LITERAL:
                trace("Term → string_literal(" + current().lexeme + ")");
                ++pos_; break;
            default:
                error("expected an expression term (id, integer, or string literal)");
        }
    }
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

    try {
        Parser p(std::move(tokens));
        p.parse();
    } catch (const std::exception& e) {
        std::cerr << "\n" << e.what() << "\n";
        return 1;
    }

    return 0;
}
