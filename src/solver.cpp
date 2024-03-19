#include "solver.hpp" 
#include <cassert>
#include "logger.hpp"

logger_t logger;

solver::solver( cnf_t cnf ) : var_count( cnf.var_count )
                            , clauses( std::move( cnf.clauses ) )
                            , var_counter( 1 )
                            , watched_in( cnf.var_count )
{ 
    values.resize( cnf.var_count + 1, val_un );

    for ( idx_t i = 0; i < var_count + 1; i++ )
    {
        logger.log( "val", "%d -> %f", i, values[ i ]  );
    }

    for ( idx_t i = 0; i < clauses.size(); i++ ) 
    {
        auto &c = clauses[ i ];
        assert( ! c.empty() );
        watched_in[ c[ 0 ] ].push_back( i );
        if ( c.size() > 1 )
            watched_in[ c[ 1 ] ].push_back( i );
    }
}

lit_t solver::pick_literal()
{
    for ( idx_t v_i = 1; v_i < var_count + 1; v_i++ )
        if ( values[ v_i ] == val_un )
            return - v_i;
    return 0;
}

bool solver::decide( lit_t l )
{
    decisions.push_back( trail.size() );  
    return assign( l );
}

val_t solver::eval_lit( lit_t l )
{
    if ( l > 0 ) return values[ var_of_lit( l ) ];
    if ( l < 0 ) return 1 - values[ var_of_lit( l ) ];
    assert( false );
}

/** Solver assigned l */
bool solver::update_watches( lit_t l )
{
    idx_t i_w_l = 0;

    auto &w_l = watched_in[ -l ];

    while ( i_w_l < w_l.size() )
    {
        idx_t i_c = w_l[ i_w_l ];
        auto &c = clauses[ i_c ];

        assert( c.size() > 1 );

        // If the clause is unit
        
        if ( eval_lit( c[ 0 ] ) == val_ff 
          && eval_lit( c[ 1 ] ) == val_ff ) 
            return true;

        // wlog: l is in the 1 position.
        
        if ( c[ 0 ] == -l ) 
            std::swap( c[ 0 ], c[ 1 ] );

        /** State: c[0] != false, c[1] = false */

        // Check if first is solved
        
        if ( eval_lit( c[ 0 ] ) == val_tt ) {
            i_w_l++;
            continue; 
        }

        /** State: c[0] = unk, c[1] = false */

        bool found = false;
        for ( idx_t i_l = 2; i_l < c.size(); i_l++ ) 
        {
            val_t v = eval_lit( c[ i_l ] );
            if ( v == val_tt || v == val_un ) {

                watched_in[ c[ i_l ] ].push_back( i_c );

                std::swap( w_l[ i_w_l ], w_l.back() );
                w_l.pop_back();

                std::swap( c[ i_l ], c[ 1 ] );
                if ( v == val_tt ) {
                    // This swap is so that watched_in[ c[ 0 ] ] is 
                    // the list through which we are iterating so 
                    // that we can kick the clause in the current list. 
                    std::swap( c[ 0 ], c[ 1 ] );
                }
                found = true;
                break;
            }
        }

        if ( ! found )
        {
            // Condition: c[0] = unk; c[1:] = false;
            logger.log( "unitprop", "%d", i_c );
            unit_queue.push( i_c );
            i_w_l++;
        }
        // Else: c[0] = true or c[0] = unk, c[1] = unk
    }

    return false;
}

bool solver::assign( lit_t l )
{
    logger.log( "assign", "%d", l );
    values[ var_of_lit( l ) ] = val_of_lit( l );
    trail.push_back( l );
    return update_watches( l );
}


void solver::backtrack()
{
    logger.log( "backtrack", "" );
    assert( ! decisions.empty() );

    idx_t i = decisions.back();
    decisions.pop_back();

    lit_t decided = trail[ i ];
    
    for ( idx_t j = i; j < trail.size(); j++ )
        values[ var_of_lit( trail[ j ] ) ] = val_un;

    trail.resize( i );


    assign( -decided );
}

bool solver::unit_propagation()
{
    while ( ! unit_queue.empty() )
    {
        idx_t i_c = unit_queue.front();
        unit_queue.pop();
        auto &c = clauses[ i_c ];

        // Condition: all except first literal are false
        
        // First is false
        val_t v = eval_lit( c[ 0 ] );

        logger.log( "lol", "%d, %d = %d", c[ 0 ], c[ 1 ], v );

        if ( v == val_tt ) 
            continue;
        else if ( v == val_ff ) 
        {
            // TODO: in cdcl more complicated
            return true;
        }
        else if ( v == val_un )
        {
            if ( assign( c[ 0 ] ) ) return true;
        }
    }
    return false;
}

sat_t solver::solve()
{
    for ( idx_t i_c = 0; i_c < clauses.size(); i_c++ )
        if ( clauses[ i_c ].size() == 1 ) 
            unit_queue.push( i_c );

    while ( true )
    {
        // logger.log( "decisions", "%d", decisions.size() );
        if ( unit_propagation() )
        {
            if ( decisions.empty() ) return UNSAT;
            backtrack();
            continue;
        } 

        lit_t l = pick_literal();   
        if ( l == 0 ) 
            break;

        logger.log( "pick", "%d", l );
        decide( l );
    }
    return SAT;
}

bool solver::done()
{
    return var_counter > var_count;
}
