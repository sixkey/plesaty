#pragma once

#include <vector>
#include <stddef.h>
#include <cmath>

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

using val_t = char;

const val_t val_ff = 0;
const val_t val_tt = 2;
const val_t val_un = 1;

inline val_t negate( val_t v ) { return 2 - v; }

inline val_t val_of_lit( lit_t l ) { return l > 0 ? val_tt : val_ff; }

inline var_t var_of_lit( lit_t l ) { return std::abs( l ); }


template < typename T >
struct literal_map
{
    std::vector< T > content;

    size_t var_count;

    literal_map( size_t var_count, T def ) : var_count( var_count )
    {
        content.resize( var_count * 2 + 1, def );
    }

    literal_map( size_t var_count ) : var_count( var_count )
    {
        content.resize( var_count * 2 + 1);
    }

    T& operator[]( lit_t l )
    {
        return content[ l + var_count ];
    }
};

