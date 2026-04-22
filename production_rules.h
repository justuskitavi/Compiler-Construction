#ifndef COMPILER_CONSTRUCTION_PRODUCTION_RULES_H
#define COMPILER_CONSTRUCTION_PRODUCTION_RULES_H

#include <string>
#include <vector>
#include "terminals.h"

struct Production
{
    int number;              // rule number (1-30)
    std::string description; // human-readable RHS
    std::vector<Symbol> rhs; // symbols to push (right-to-left on stack)
};

std::vector<Production> makeProductions();

#endif //COMPILER_CONSTRUCTION_PRODUCTION_RULES_H
