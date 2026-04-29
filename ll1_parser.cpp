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
#include <memory>
#include <stdexcept>

#include "terminals.h"
#include "production_rules.h"
#include "parse_table.h"

struct TreeNode {
    std::string label;
    std::vector<std::shared_ptr<TreeNode>> children;
    explicit TreeNode(std::string lbl) : label(std::move(lbl)) {}
    void addChild(std::shared_ptr<TreeNode> c) { children.push_back(std::move(c)); }
};

void printTree(const std::shared_ptr<TreeNode> &node,
               const std::string &prefix = "",
               bool isLast = true)
{
    if (prefix.empty()) {
        std::cout << node->label << "\n";
    } else {
        std::cout << prefix << (isLast ? "`-- " : "|-- ") << node->label << "\n";
    }
    const std::string childPfx = prefix + (isLast ? "    " : "|   ");
    for (size_t i = 0; i < node->children.size(); ++i)
        printTree(node->children[i], childPfx, i == node->children.size() - 1);
}

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
    LL1Parser(std::vector<Terminals> toks,std::vector<Production> prods,std::map<NonTerminal, std::map<Terminal, int>> table)
        : tokens_(std::move(toks)),
          productions_(std::move(prods)),
          table_(std::move(table)),
          pos_(0) {}

    std::shared_ptr<TreeNode> parse()
    {
        auto root = std::make_shared<TreeNode>("Program");

        struct StackEntry {
            Symbol symbol;
            std::shared_ptr<TreeNode> node;
        };

        std::stack<StackEntry> stack;
        stack.push({Symbol::term(T_EOF), nullptr});
        stack.push({Symbol::nonterm(NT_PROGRAM), root});

        while (true) {
            StackEntry stack_top = stack.top();
            const Terminals &current_token = tokens_[pos_];

            // Accept
            if (stack_top.symbol.isTerminal && stack_top.symbol.terminal == T_EOF && current_token.type == T_EOF) {
                std::cout << "\n[Parser] SUCCESS — program is syntactically valid.\n\n";
                return root;
            }

            // Terminal match
            if (stack_top.symbol.isTerminal) {
                if (stack_top.symbol.terminal == current_token.type) {
                    std::string lbl = termName(current_token.type);
                    if (!current_token.lexeme.empty()) lbl += "(" + current_token.lexeme + ")";
                    if (stack_top.node) stack_top.node->label = lbl;
                    std::cout << "  match   " << lbl << "\n";
                    stack.pop();
                    ++pos_;
                } else {
                    std::string got = termName(current_token.type);
                    if (!current_token.lexeme.empty()) got += "(" + current_token.lexeme + ")";
                    throw std::runtime_error(
                        std::string("[Syntax error] expected '") +
                        termName(stack_top.symbol.terminal) + "', got '" + got + "'");
                }
                continue;
            }

            // Non-terminal: look up production
            NonTerminal non_terminal = stack_top.symbol.non_terminal;
            Terminal look_ahead_token = current_token.type;

            auto non_terminal_row = table_.find(non_terminal);
            if (non_terminal_row == table_.end() || non_terminal_row->second.find(look_ahead_token) == non_terminal_row->second.end()) {
                std::string got = termName(look_ahead_token);
                if (!current_token.lexeme.empty()) got += "(" + current_token.lexeme + ")";
                throw std::runtime_error(
                    std::string("[Syntax error] no rule for ") +
                    ntName(non_terminal) + " on input '" + got + "'");
            }

            int production_rule = non_terminal_row->second.at(look_ahead_token);
            const Production &prod = productions_[production_rule];
            std::cout << "  rule " << production_rule << (production_rule < 10 ? " " : "")
                      << "  " << prod.description << "\n";

            stack.pop();
            auto parentNode = stack_top.node;

            if (prod.right_hand_side.empty()) {
                parentNode->addChild(std::make_shared<TreeNode>("epsilon"));
            } else {
                std::vector<std::shared_ptr<TreeNode>> childNodes;
                for (const Symbol &symbol : prod.right_hand_side) {
                    std::string lbl = symbol.isTerminal ? termName(symbol.terminal) : ntName(symbol.non_terminal);
                    childNodes.push_back(std::make_shared<TreeNode>(lbl));
                    parentNode->addChild(childNodes.back());
                }
                for (int i = (int)prod.right_hand_side.size() - 1; i >= 0; --i)
                    stack.push({prod.right_hand_side[i], childNodes[i]});
            }
        }
    }

private:
    std::vector<Terminals> tokens_;
    std::vector<Production> productions_;
    std::map<NonTerminal, std::map<Terminal, int>> table_;
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
        auto tree = parser.parse();

        std::cout << "=== Parse Tree ===\n";
        printTree(tree);
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n"
                  << e.what() << "\n";
        return 1;
    }

    return 0;
}