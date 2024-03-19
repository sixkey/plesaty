#pragma once

#include "base.hpp"

#include <vector>
#include <queue>
#include <cmath>

using val_t = float;

const val_t val_ff = 0;
const val_t val_tt = 1;
const val_t val_un = 0.5;

inline val_t val_of_lit( lit_t l ) { return l > 0 ? val_tt : val_ff; }

inline var_t var_of_lit( lit_t l ) { return std::abs( l ); }

template < typename T >  
struct literal_map
{
    std::vector< T > content;

    size_t var_count;

    literal_map( size_t var_count ) : var_count( var_count )
    {
        content.resize( var_count * 2 );
    }

    T& operator[]( lit_t l )
    {
        return content[ var_of_lit( l ) - 1 
                      + val_of_lit( l ) * var_count ];
    }
};

struct solver
{
    size_t var_count;
    std::vector< std::vector< lit_t > > clauses;

    // Constructor

    solver( cnf_t cnf );  

    // Picking literal

    var_t var_counter = 1;
    
    lit_t pick_literal();

    // Variable state
    
    literal_map< std::vector< idx_t > > watched_in;

    std::vector< val_t > values;
    std::vector< lit_t > trail;

    std::vector< idx_t > decisions;

    std::queue< idx_t > unit_queue;

    val_t eval_lit( lit_t l );

    bool decide( lit_t l );

    bool update_watches( lit_t l );

    bool assign( lit_t l );

    void backtrack();

    bool unit_propagation();

    bool done();

    sat_t solve();
};
