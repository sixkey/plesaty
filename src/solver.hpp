#pragma once

#include "base.hpp"

#include <vector>
#include <deque>
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

    literal_map( size_t var_count, T def ) : var_count( var_count )
    {
        content.resize( var_count * 2, def );
    }

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
    var_t var_counter = 1;

    std::vector< std::vector< lit_t > > clauses;

    // Constructor

    solver( cnf_t cnf );  

    // Picking literal
    
    lit_t pick_literal();

    // Variable state
    
    literal_map< std::vector< idx_t > > watched_in;

    literal_map< sidx_t > reason;

    std::vector< val_t > values;
    std::vector< lit_t > trail;
    std::vector< idx_t > decisions;

    literal_map< sidx_t > lit_level;
    sidx_t decision_level = 0;

    std::deque< idx_t > unit_queue;

    literal_map< short > to_resolve;
    literal_map< short > learnt_lit;

    

    val_t eval_lit( lit_t l );

    sidx_t decide( lit_t l );

    sidx_t assign( lit_t l );

    sidx_t update_watches( lit_t l );

    sidx_t unit_propagation();

    void kill_trail( idx_t i );

    void backtrack( sidx_t dec_level );

    idx_t learn( clause_t c );

    void learn_part( clause_t& learnt_clause
                   , idx_t i_c
                   , lit_t r );

    std::pair< clause_t, idx_t > conflict_anal( sidx_t i_c );

    bool done();

    sat_t solve();
};
