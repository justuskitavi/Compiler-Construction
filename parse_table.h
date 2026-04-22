#ifndef COMPILER_CONSTRUCTION_PARSE_TABLE_H
#define COMPILER_CONSTRUCTION_PARSE_TABLE_H

#include <map>
#include "terminals.h"

std::map<NonTerminal, std::map<Terminal, int>> buildParseTable();

#endif //COMPILER_CONSTRUCTION_PARSE_TABLE_H
