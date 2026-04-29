/*
 *  LL(1) Table-Driven Parser for the mini-language.
 *  - Parse tree is built and printed incrementally as parsing happens.
 *  - On error, the partial tree built so far is printed, then halts.
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

// ─────────────────────────────────────────────────────────────────────────────
//  Parse-tree node
// ─────────────────────────────────────────────────────────────────────────────
struct TreeNode
{
    std::string label;
    std::vector<std::shared_ptr<TreeNode>> children;

    explicit TreeNode(std::string lbl) : label(std::move(lbl)) {}
    void addChild(std::shared_ptr<TreeNode> c) { children.push_back(std::move(c)); }
};

// Prints the tree with box-drawing connectors, incrementally safe to call
// at any point — even mid-parse on a partial tree.
void printTree(const std::shared_ptr<TreeNode> &node,
               const std::string &prefix = "",
               bool isLast = true)
{
    if (prefix.empty())
    {
        std::cout << node->label << "\n";
    }
    else
    {
        std::cout << prefix << (isLast ? "`-- " : "|-- ") << node->label << "\n";
    }
    const std::string childPfx = prefix + (isLast ? "    " : "|   ");
    for (size_t i = 0; i < node->children.size(); ++i)
        printTree(node->children[i], childPfx, i == node->children.size() - 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Token loader
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
//  LL(1) Parser
//  Builds the parse tree live as each production is applied / each terminal
//  matched. On any error the partial tree is printed before halting.
// ─────────────────────────────────────────────────────────────────────────────
class LL1Parser
{
public:
    LL1Parser(std::vector<Terminals> tokens,
              std::vector<Production> productions,
              std::map<NonTerminal, std::map<Terminal, int>> table)
        : tokens_(std::move(tokens)),
          productions_(std::move(productions)),
          table_(std::move(table)),
          pos_(0) {}

    // Parses the token stream. Returns the root of the (possibly partial) tree.
    // Prints the parse trace to stdout as it goes.
    // On a syntax error: prints the partial tree, then throws.
    std::shared_ptr<TreeNode> parse()
    {
        root_ = std::make_shared<TreeNode>("Program");

        struct StackEntry
        {
            Symbol symbol;
            std::shared_ptr<TreeNode> node; // tree node this symbol owns
        };

        std::stack<StackEntry> stk;
        stk.push({Symbol::term(T_EOF), nullptr});
        stk.push({Symbol::nonterm(NT_PROGRAM), root_});

        while (true)
        {
            StackEntry top = stk.top();
            const Terminals &current = tokens_[pos_];

            // ── Accept ───────────────────────────────────────────────────────
            if (top.symbol.isTerminal &&
                top.symbol.terminal == T_EOF &&
                current.type == T_EOF)
            {
                std::cout << "\n[Parser] SUCCESS — program is syntactically valid.\n";
                return root_;
            }

            // ── Terminal on top ──────────────────────────────────────────────
            if (top.symbol.isTerminal)
            {
                if (top.symbol.terminal == current.type)
                {
                    // Build leaf label with lexeme when present
                    std::string lbl = termName(current.type);
                    if (!current.lexeme.empty())
                        lbl += "(" + current.lexeme + ")";

                    // Update the placeholder node that was already attached
                    // to the tree when this terminal was pushed.
                    if (top.node)
                        top.node->label = lbl;

                    std::cout << "  match   " << lbl << "\n";
                    stk.pop();
                    ++pos_;
                }
                else
                {
                    // ── Terminal mismatch error ──────────────────────────────
                    std::string got = termName(current.type);
                    if (!current.lexeme.empty())
                        got += "(" + current.lexeme + ")";

                    std::string msg =
                        std::string("[Syntax error] expected '") +
                        termName(top.symbol.terminal) +
                        "', got '" + got + "'";

                    printPartialTreeAndError(msg);
                    throw std::runtime_error(msg);
                }
                continue;
            }

            // ── Non-terminal on top ──────────────────────────────────────────
            NonTerminal nt = top.symbol.non_terminal;
            Terminal la = current.type;

            auto rowIt = table_.find(nt);
            if (rowIt == table_.end() ||
                rowIt->second.find(la) == rowIt->second.end())
            {
                // ── No-rule error ────────────────────────────────────────────
                std::string got = termName(la);
                if (!current.lexeme.empty())
                    got += "(" + current.lexeme + ")";

                std::string msg =
                    std::string("[Syntax error] no rule for ") +
                    ntName(nt) + " on input '" + got + "'";

                printPartialTreeAndError(msg);
                throw std::runtime_error(msg);
            }

            int ruleNum = rowIt->second.at(la);
            const Production &prod = productions_[ruleNum];

            std::cout << "  rule " << ruleNum
                      << (ruleNum < 10 ? " " : "")
                      << "  " << prod.description << "\n";

            stk.pop();
            auto parentNode = top.node;

            if (prod.right_hand_side.empty())
            {
                // ε production: attach epsilon leaf immediately
                parentNode->addChild(std::make_shared<TreeNode>("epsilon"));
            }
            else
            {
                // Create all child nodes and attach them to the tree NOW,
                // before pushing anything. This way the tree always reflects
                // every production that has been applied so far.
                std::vector<std::shared_ptr<TreeNode>> childNodes;
                childNodes.reserve(prod.right_hand_side.size());

                for (const Symbol &sym : prod.right_hand_side)
                {
                    std::string lbl = sym.isTerminal
                                          ? termName(sym.terminal)
                                          : ntName(sym.non_terminal);
                    auto child = std::make_shared<TreeNode>(lbl);
                    childNodes.push_back(child);
                    parentNode->addChild(child); // ← attached to tree here
                }

                // Push right-to-left so left-most symbol is processed first
                for (int i = (int)prod.right_hand_side.size() - 1; i >= 0; --i)
                    stk.push(StackEntry{prod.right_hand_side[i], childNodes[i]});
            }
        }
    }

private:
    std::vector<Terminals> tokens_;
    std::vector<Production> productions_;
    std::map<NonTerminal, std::map<Terminal, int>> table_;
    size_t pos_;
    std::shared_ptr<TreeNode> root_;

    // Called on any error: prints the partial tree then the error message.
    void printPartialTreeAndError(const std::string &msg) const
    {
        std::cout << "\n=== Parse Tree (partial — stopped at error) ===\n";
        if (root_)
            printTree(root_);
        std::cerr << "\n"
                  << msg << "\n";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
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
              << " token(s) from '" << tokenFile << "'\n\n"
              << "=== Parse Trace ===\n";

    auto prods = makeProductions();
    auto table = buildParseTable();

    try
    {
        LL1Parser parser(std::move(tokens), std::move(prods), std::move(table));
        auto tree = parser.parse();

        std::cout << "\n=== Parse Tree ===\n";
        printTree(tree);
    }
    catch (const std::exception &)
    {
        // Error already printed inside parser; just signal failure.
        return 1;
    }

    return 0;
}