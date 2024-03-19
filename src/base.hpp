#pragma once

#include <vector>
#include <stddef.h>

using lit_t    = int;
using var_t    = unsigned int;
using clause_t = std::vector< lit_t >;
using idx_t    = size_t;
using sidx_t   = long long; 

const sidx_t idx_undef = -1;

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
