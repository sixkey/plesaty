#include "solver.hpp" 
#include <cassert>
#include "logger.hpp"

logger_t logger;

solver::solver( cnf_t cnf ) : var_count( cnf.var_count )
                            , clauses( std::move( cnf.clauses ) )
                            , var_counter( 1 )
                            , watched_in( cnf.var_count )
                            , reason( cnf.var_count, idx_undef )
                            , lit_level( cnf.var_count, -1 )
                            , to_resolve( cnf.var_count, 0 )
                            , learnt_lit( cnf.var_count, 0 )
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

val_t solver::eval_lit( lit_t l )
{
    if ( l > 0 ) return values[ var_of_lit( l ) ];
    if ( l < 0 ) return 1 - values[ var_of_lit( l ) ];
    assert( false );
}


sidx_t solver::decide( lit_t l )
{
    decisions.push_back( trail.size() );  
    decision_level += 1;
    return assign( l );
}

sidx_t solver::assign( lit_t l )
{
    logger.log( "assign", "%d@%d", l, decision_level );
    values[ var_of_lit( l ) ] = val_of_lit( l );
    trail.push_back( l );
    lit_level[ l ] = decision_level;
    return update_watches( l );
}

/** Solver assigned l */
sidx_t solver::update_watches( lit_t l )
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
            return i_c;

        // wlog: l is in the 1 position.
        
        if ( c[ 0 ] == -l ) 
            std::swap( c[ 0 ], c[ 1 ] );

        assert( -l == c[ 1 ] );

        /** State: c[0] != false, c[1] = false */

        #ifdef CHECKED
        assert( eval_lit( c[ 0 ] ) != val_ff );
        assert( eval_lit( c[ 1 ] ) == val_ff );
        #endif

        // Check if first is solved
        
        if ( eval_lit( c[ 0 ] ) == val_tt ) {
            i_w_l++;
            continue; 
        }

        /** State: c[0] = unk, c[1] = false */

        #ifdef CHECKED
        assert( eval_lit( c[ 0 ] ) == val_un );
        assert( eval_lit( c[ 1 ] ) == val_ff );
        #endif

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
            #ifdef CHECKED
            assert( eval_lit( c[ 0 ] ) == val_un );
            for ( idx_t i = 1; i < c.size(); i++ )
                assert( eval_lit( c[ i ] ) == val_ff );
            #endif
            logger.log( "unitprop", "%d", i_c );
            unit_queue.push_back( i_c );
            i_w_l++;
        }
        // Else: c[0] = true or c[0] = unk, c[1] = unk
    }

    return idx_undef;
}


sidx_t solver::unit_propagation()
{
    while ( ! unit_queue.empty() )
    {
        //logger.log( "unit queue", unit_queue );
        //logger.log( "unit trail", trail );
        //logger.log( "unit decs", decisions );
        idx_t i_c = unit_queue.front();
        unit_queue.pop_front();
        auto &c = clauses[ i_c ];

        // Condition: all except first literal are false
        
        #ifdef CHECKED
        for ( idx_t i = 1; i < c.size(); i++ )
        {
            if ( eval_lit( c[ i ] ) != val_ff )
            {
                logger.log( "PROBLEM", c );
                for ( auto &l : c )
                    logger.log( "PROBLEM", "%d@%d -> %f", l, lit_level[ l ], eval_lit( l ) );
            }
            assert( eval_lit( c[ i ] ) == val_ff );
        }
        #endif
        
        // First is false
        val_t v = eval_lit( c[ 0 ] );

        //logger.log( "lol", "%d, %d = %d", c[ 0 ], c[ 1 ], v );

        if ( v == val_tt ) 
            continue;
        else if ( v == val_ff ) 
            return i_c;
        else if ( v == val_un )
        {
            assert( reason[ c[ 0 ] ] == idx_undef );
            reason[ c[ 0 ] ] = i_c;

            if ( sidx_t a_i = assign( c[ 0 ] ); a_i != idx_undef ) 
                return a_i;
        }
    }
    return idx_undef;
}

void solver::learn_part( clause_t& learnt_clause, idx_t i_c, lit_t r )
{
    auto &c = clauses[ i_c ];

    if ( r != 0 )
        to_resolve[ r ] = 0;

    #ifdef CHECKED
        bool found = false;
        for ( lit_t l : c ) 
            if ( l == r ) found = true;
        assert( found || r == 0 );
    #endif

    for ( lit_t l : c ) 
    {
        if ( l == r ) continue;

        if ( lit_level[ -l ] == decision_level ) 
            to_resolve[ -l ] = 1;
        else 
        {
            if ( ! learnt_lit[ l ] )
                learnt_clause.push_back( l );
            learnt_lit[ l ] = 1;
        }
    }

}

size_t count_pos( literal_map< short > m, var_t var_count )
{
    size_t count = 0;
    for ( var_t v = 1; v <= var_count; v++ )
    {
        if ( m[ v ] ) count += 1;
        if ( m[ -v ] ) count += 1;
    }
    return count;
}

std::pair< clause_t, idx_t > solver::conflict_anal( sidx_t i_c ) 
{
    idx_t last_d_i = decisions.back(); 

    clause_t learnt_clause;
    learn_part( learnt_clause, i_c, 0 );

    #ifdef CHECKED
    bool found = false;
    for ( var_t v = 1; v <= var_count; v++ )
    {
        assert( ! to_resolve[ v ] || ! to_resolve[ -v ] );
        found = found || to_resolve[ v ] || to_resolve[ -v ];
    }
    assert( found );
    assert( count_pos( to_resolve, var_count ) >= 1 );
    #endif

    idx_t j = trail.size() - 1;
    while ( count_pos( to_resolve, var_count ) > 1 )
    {
        assert( j > last_d_i );
        auto &t_j = trail[ j ];
        if ( to_resolve[ t_j ] )
        {
            assert( reason[ t_j ] != idx_undef );
            learn_part( learnt_clause, reason[ t_j ], t_j );
        }
        j--;
    }
    
    while ( ! to_resolve[ trail[ j ] ] )
        j--;

    sidx_t next_dec_level = 0;
    for ( auto &l : learnt_clause )
    {
        //logger.log( "wow", "%d@%d", -l, lit_level[ -l ] );

        learnt_lit[ l ] = 0;
        next_dec_level = std::max( next_dec_level, lit_level[ -l ] );
    }
    to_resolve[ trail[ j ] ] = 0;
    
    learnt_clause.push_back( - trail[ j ] );
    std::swap( learnt_clause[ 0 ], learnt_clause.back() );

    #ifdef CHECKED
    for ( var_t v = 1; v <= var_count; v++ )
    {
        assert( to_resolve[ v ] == 0 && to_resolve[ -v ] == 0 );
        assert( learnt_lit[ v ] == 0 && learnt_lit[ -v ] == 0 );
    }
    #endif

    assert( next_dec_level >= 0 );

    return std::pair( learnt_clause, next_dec_level );
}

void solver::kill_trail( idx_t i )
{
    // Assignments above decision.
    for ( idx_t j = i; j < trail.size(); j++ )
    {
        auto &t_j = trail[ j ];
        values[ var_of_lit( t_j ) ] = val_un;
        reason[ t_j ] = idx_undef;
        lit_level[ t_j ] = -1;
    }
    trail.resize( i );
}

void solver::backtrack( sidx_t target_level )
{
    logger.log( "backtrack", "%d", target_level  );


    idx_t last_dec = decisions.back();
    while ( decision_level > target_level )
    {
        last_dec = decisions.back();
        decisions.pop_back();
        decision_level--;
    }

    kill_trail( last_dec );

    //logger.log( "back queue", unit_queue );
    //logger.log( "back trail", trail );
    //logger.log( "back decs", decisions );
}

idx_t solver::learn( clause_t c )
{
    logger.log( "learn", c ); 

    idx_t i = clauses.size();
    clauses.push_back( c );
    
    assert( ! c.empty() );
    watched_in[ c[ 0 ] ].push_back( i );
    if ( c.size() > 1 )
        watched_in[ c[ 1 ] ].push_back( i );

    return i;
}

sat_t solver::solve()
{
    for ( idx_t i_c = 0; i_c < clauses.size(); i_c++ )
        if ( clauses[ i_c ].size() == 1 ) 
            unit_queue.push_back( i_c );

    while ( true )
    {
        // logger.log( "decisions", "%d", decisions.size() );
        
        if ( sidx_t i_c = unit_propagation(); i_c != idx_undef )
        {
            if ( decisions.empty() ) return UNSAT;

            unit_queue.clear();

            auto [ new_clause, target_level ] = conflict_anal( i_c );

            idx_t i_new_clause = learn( std::move( new_clause ) );
            backtrack( target_level );
            unit_queue.push_back( i_new_clause );
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
