#pragma once

#include <vector>

using clause_t = std::vector< int >;

enum sat_t 
{
    SAT, UNSAT, UNKNOWN
};

inline const char* sat_to_str[] = 
{
    "SAT", "UNSAT", "UNKNOWN"
};

struct cnf_t
{
    std::vector< clause_t > clauses; 
    unsigned int var_count = 0;
};
