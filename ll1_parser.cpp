/*
 *  LL(1) Table-Driven Parser for the mini-language.
 *
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
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <map>
#include <stdexcept>

#include "terminals.h"
#include "production_rules.h"
#include "parse_table.h"

std::vector<Terminals> loadTokens(const std::string &path)
{
    std::ifstream in(path);
    if (!in.is_open())
        throw std::runtime_error("Cannot open token file: " + path);

    std::vector<Terminals> tokens;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty())
            continue;

        auto paren = line.find('(');
        std::string typeName = (paren != std::string::npos)
                                   ? line.substr(0, paren)
                                   : line;

        while (!typeName.empty() &&
               (typeName.back() == '\r' || typeName.back() == ' '))
            typeName.pop_back();

        Terminal tt = nameToTerminal(typeName);
        if (static_cast<int>(tt) == -1)
            continue;

        std::string lexeme;
        if (paren != std::string::npos)
        {
            auto close = line.rfind(')');
            if (close != std::string::npos && close > paren)
                lexeme = line.substr(paren + 1, close - paren - 1);
        }
        tokens.push_back({tt, lexeme});
    }
    tokens.push_back({T_EOF, "$"});
    return tokens;
}

class LL1Parser
{
public:
    LL1Parser(std::vector<Terminals> toks,
              std::vector<Production> prods,
              std::map<NonTerminal, std::map<Terminal, int>> table)
        : tokens_(std::move(toks)),
          prods_(std::move(prods)),
          table_(std::move(table)),
          pos_(0) {}

    void parse()
    {
        // Initialise stack: push $ then the start symbol
        stack_.push(Symbol::term(T_EOF));
        stack_.push(Symbol::nonterm(NT_PROGRAM));

        while (true)
        {
            Symbol top = stack_.top();
            const Terminals &current = tokens_[pos_];

            if (top.isTerminal && top.terminal == T_EOF && current.type == T_EOF)
            {
                std::cout << "\n[Parser] SUCCESS — program is syntactically valid.\n";
                return;
            }

            // Top is terminal
            if (top.isTerminal)
            {
                if (top.terminal == current.type)
                {
                    // Match: consume token
                    std::string lex = current.lexeme.empty()
                                          ? termName(current.type)
                                          : termName(current.type) + std::string("(") + current.lexeme + ")";
                    std::cout << "  match   " << lex << "\n";
                    stack_.pop();
                    ++pos_;
                }
                else
                {
                    std::string got = current.lexeme.empty()
                                          ? termName(current.type)
                                          : std::string(termName(current.type)) + "(" + current.lexeme + ")";
                    throw std::runtime_error(
                        std::string("[Syntax error] expected '") + termName(top.terminal) +
                        "', got '" + got + "'");
                }
                continue;
            }

            // Top is non-terminal
            NonTerminal nt = top.nt;
            Terminal a = current.type;

            auto ntIt = table_.find(nt);
            if (ntIt == table_.end() || ntIt->second.find(a) == ntIt->second.end())
            {
                std::string got = current.lexeme.empty()
                                      ? termName(a)
                                      : std::string(termName(a)) + "(" + current.lexeme + ")";
                throw std::runtime_error(
                    std::string("[Syntax error] no rule for ") + ntName(nt) +
                    " on input '" + got + "'");
            }

            int prodNum = ntIt->second.at(a);
            const Production &prod = prods_[prodNum];

            // Trace the production being applied
            std::cout << "  rule " << prodNum
                      << (prodNum < 10 ? " " : "") // align single-digit numbers
                      << "  " << prod.description << "\n";

            stack_.pop();

            // Push RHS symbols in reverse order (so left-most is on top)
            for (int i = static_cast<int>(prod.rhs.size()) - 1; i >= 0; --i)
                stack_.push(prod.rhs[i]);
        }
    }

private:
    std::vector<Terminals> tokens_;
    std::vector<Production> prods_;
    std::map<NonTerminal, std::map<Terminal, int>> table_;
    std::stack<Symbol> stack_;
    size_t pos_;
};

int main(int argc, char *argv[])
{
    std::string tokenFile = (argc > 1) ? argv[1] : "tokens.out";

    std::vector<Terminals> tokens;
    try
    {
        tokens = loadTokens(tokenFile);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::cout << "[Parser] Loaded " << tokens.size() - 1
              << " token(s) from '" << tokenFile << "'\n\n";
    std::cout << "=== Parse Trace ===\n";

    auto prods = makeProductions();
    auto table = buildParseTable();

    try
    {
        LL1Parser parser(std::move(tokens), std::move(prods), std::move(table));
        parser.parse();
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n"
                  << e.what() << "\n";
        return 1;
    }

    return 0;
}