#pragma once

#include "base.hpp"
#include "sequences.hpp"

#include <vector>
#include <deque>
#include <cmath>
#include <cassert>
#include <algorithm>


///////////////////////////////////////////////////////////////////////////////
// Solver /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


struct literal_set
{
    std::vector< char > content;

    size_t var_count;

    size_t size;

    literal_set( size_t var_count ) : var_count( var_count ), size( 0 )
    {
        content.resize( var_count * 2 );
    }

    bool contains( lit_t l )
    {
        return content[ var_of_lit( l ) - 1 + val_of_lit( l ) * var_count ];
    }

    void add( lit_t l )
    {
        auto &el = content[ var_of_lit( l ) - 1 + val_of_lit( l ) * var_count ];
        if ( !el )
            ++size;
        el = 1;
    }

    void remove( lit_t l )
    {
        auto &el = content[ var_of_lit( l ) - 1 + val_of_lit( l ) * var_count ];
        if ( el )
            --size;
        el = 0;
    }
};

struct var_heap
{
    std::vector< std::pair< double, var_t > > content;

    std::vector< size_t > var_idx;

    size_t var_count;

    size_t size;

    size_t parent( size_t i )
    {
        return i / 2;
    }

    size_t left( size_t i )
    {
        return 2 * i + 1;
    }

    size_t right( size_t i)
    {
        return 2 * i + 2;
    }

    var_heap( size_t var_count ) : var_count( var_count ), size( var_count )
    {
        content.resize( var_count );
        var_idx.resize( var_count + 1 );

        for ( size_t i = 0; i < var_count; i++ )
        {
            var_idx[ i + 1 ] = i;
            content[ i ] = { 0.0, i + 1 };
        }
    }

    void push( var_t var )
    {
        if ( var_idx[ var ] < size )
            return;

        swap( size, var_idx[ var ] );
        ++ size;
        move_up( size - 1 );
    }

    void set_priority( var_t var, double p )
    {
        content[ var_idx[ var ] ].first = p;

        if ( var_idx[ var ] < size )
        {
            move_up( var_idx[ var ] );
            move_down( var_idx[ var ] );
        }
    }

    void move_up( size_t i )
    {
        while ( content[ i ].first > content[ parent( i ) ].first && i != 0 )
        {
            swap( i,  parent( i ) );
            i = parent( i );
        }
    }

    void move_down( size_t i )
    {
        while ( left( i ) < size )
        {
            size_t max_child = left( i );
            if ( right ( i ) < size && content[ right( i ) ].first > content[ left(i) ].first )
                max_child = right( i );

            if ( content[ max_child ].first <= content[ i ].first )
                break;

            swap( i, max_child );
            i = max_child;
        }
    }

    var_t extract_max()
    {
        assert( size != 0 );

        var_t var = content[ 0 ].second;
        swap( 0, size - 1);
        -- size;
        if (size > 0)
            move_down( 0 );

        return var;
    }

    void swap( size_t i, size_t j )
    {
        std::swap( var_idx[ content[ i ].second ], var_idx[ content[ j ].second ] );
        std::swap( content[ i ], content [ j ] );
    }
};

///////////////////////////////////////////////////////////////////////////////
// Solver /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


struct solver
{
    solver( cnf_t cnf );
    sat_t solve();

    // Formula 

    size_t var_count;

    std::vector< std::vector< lit_t > > clauses;

    // Picking literal

    lit_t pick_literal();

    // Values

    std::vector< val_t > values;

    val_t eval_lit( lit_t l );

    // Backtracking

    std::vector< lit_t > trail;
    std::vector< idx_t > decisions;

    sidx_t decide( lit_t l );

    sidx_t assign( lit_t l );

    void kill_trail( idx_t i );

    void backtrack( sidx_t dec_level );

    // Unit propagation

    literal_map< std::vector< idx_t > > watched_in;
    std::deque< idx_t > unit_queue;

    sidx_t update_watches( lit_t l );

    sidx_t unit_propagation();

    // CDCL 
    
    literal_map< sidx_t > lit_level;
    sidx_t decision_level = 0;

    literal_map< sidx_t > reason;
    literal_set to_resolve;
    literal_set learnt_lit;

    void resolve_part( clause_t& learnt_clause
                     , idx_t i_c
                     , lit_t r );

    std::pair< clause_t, idx_t > conflict_anal( sidx_t i_c );

    idx_t learn( clause_t c );

    // EVSIDS

    var_heap heap;
    double bump_step = 1.01;
    double bump_size = 1.0;

    void increase_bump();

    void bump( var_t var );

    // Restarts
    size_t next_restart;
    size_t conflict_count;
    luby luby_gen;

    void restart();
};
