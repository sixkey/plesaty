#include "solver.hpp"
#include <cassert>
#include "logger.hpp"

/// Global ////////////////////////////////////////////////////////////////////


logger_t logger;


/// Misc. /////////////////////////////////////////////////////////////////////


void log_trail( const char *message, solver& s )
{
    for ( auto &l : s.trail )
    {
        logger.log( message, "%d@%d", l, s.lit_level[ l ] );
    }
}


/// Solver ////////////////////////////////////////////////////////////////////


solver::solver( cnf_t cnf ) : var_count( cnf.var_count )
                            , clauses( std::move( cnf.clauses ) )
                            , watched_in( cnf.var_count )
                            , lit_level( cnf.var_count, -1 )
                            , reason( cnf.var_count, idx_undef )
                            , to_resolve( cnf.var_count )
                            , learnt_lit( cnf.var_count )
                            , heap( cnf.var_count )
                            , luby_gen( 420 )
{
    values.resize( cnf.var_count + 1, val_un );

    for ( idx_t i = 0; i < clauses.size(); i++ )
    {
        auto &c = clauses[ i ];
        assert( ! c.empty() );
        watched_in[ c[ 0 ] ].push_back( i );
        if ( c.size() > 1 )
            watched_in[ c[ 1 ] ].push_back( i );
    }

    next_restart = luby_gen.next();
}


sat_t solver::solve()
{
    for ( idx_t i_c = 0; i_c < clauses.size(); i_c++ )
        if ( clauses[ i_c ].size() == 1 )
            unit_queue.push_back( i_c );

    while ( true )
    {
        logger.log( "new cycle" );
        log_trail( "new cycle", *this );
        // logger.log( "decisions", "%d", decisions.size() );

        if ( sidx_t i_c = unit_propagation(); i_c != idx_undef )
        {
            logger.log( "conflict", clauses[ i_c ] );
            ++conflict_count;

            if ( decisions.empty() ) return UNSAT;
            unit_queue.clear();

            auto [ new_clause, target_level ] = conflict_anal( i_c );

            idx_t i_new_clause = learn( std::move( new_clause ) );

            logger.log( "new_clause", clauses[ i_new_clause ] );
            if ( conflict_count >= next_restart )
            {
                logger.log( "restart" );
                if ( target_level == 0 )
                    unit_queue.push_back( i_new_clause );
                restart();
            }
            else
            {
                backtrack( target_level );
                unit_queue.push_back( i_new_clause );
            }
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


/// Picking literal ///////////////////////////////////////////////////////////


lit_t solver::pick_literal()
{
    while ( heap.size > 0 )
    {
        var_t v = heap.extract_max();
        if ( values[ v ] == val_un )
            return - v;
    }
    return 0;
}


/// Values ////////////////////////////////////////////////////////////////////


val_t solver::eval_lit( lit_t l )
{
    if ( l > 0 ) return values[ var_of_lit( l ) ];
    if ( l < 0 ) return 1 - values[ var_of_lit( l ) ];
    assert( false );
}


/// Backtracking //////////////////////////////////////////////////////////////


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


void solver::kill_trail( idx_t i )
{
    // Assignments above decision.
    for ( idx_t j = i; j < trail.size(); j++ )
    {
        auto &t_j = trail[ j ];
        values[ var_of_lit( t_j ) ] = val_un;
        reason[ t_j ] = idx_undef;
        heap.push( var_of_lit( t_j ) );
        lit_level[ t_j ] = -1;
    }
    trail.resize( i );
}


void solver::backtrack( sidx_t target_level )
{
    logger.log( "backtrack", "%d", target_level  );

    log_trail( "b", *this );


    idx_t last_dec = decisions.back();
    while ( decision_level > target_level )
    {
        last_dec = decisions.back();
        decisions.pop_back();
        decision_level--;
    }

    kill_trail( last_dec );
}


/// Unit propagation //////////////////////////////////////////////////////////


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
        {
#ifdef CHECKED
            for ( auto &l : c )
                assert( eval_lit( l ) == val_ff );
#endif
            return i_c;
        }


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
                    logger.log( "PROBLEM", "%d@%d -> %f", -l, lit_level[ -l ], eval_lit( l ) );
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


/// CDCL //////////////////////////////////////////////////////////////////////


void solver::resolve_part( clause_t& learnt_clause, idx_t i_c, lit_t r )
{
    auto &c = clauses[ i_c ];

    if ( r != 0 )
        to_resolve.remove( r );

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
            to_resolve.add( -l );
        else
        {
            if ( ! learnt_lit.contains( l ) )
                learnt_clause.push_back( l );
            learnt_lit.add( l );
        }
    }
}


std::pair< clause_t, idx_t > solver::conflict_anal( sidx_t i_c )
{
    idx_t last_d_i = decisions.back();

    clause_t learnt_clause;
    resolve_part( learnt_clause, i_c, 0 );

    #ifdef CHECKED
    bool found = false;
    for ( var_t v = 1; v <= var_count; v++ )
    {
        assert( ! to_resolve.contains( v ) || ! to_resolve.contains( -v ) );
        found = found || to_resolve.contains( v ) || to_resolve.contains( -v );
    }
    assert( found );
    assert( to_resolve.size >= 1 );
    #endif

    idx_t j = trail.size() - 1;

    while ( to_resolve.size > 1 )
    {
        assert( j > last_d_i );
        auto &t_j = trail[ j ];
        if ( to_resolve.contains( t_j ) )
        {
            assert( reason[ t_j ] != idx_undef );
            resolve_part( learnt_clause, reason[ t_j ], t_j );
            bump( var_of_lit( t_j ) );
        }
        j--;
    }

    while ( ! to_resolve.contains( trail[ j ] ) )
        j--;

    to_resolve.remove( trail[ j ] );
    learnt_clause.push_back( - trail[ j ] );
    std::swap( learnt_clause[ 0 ], learnt_clause.back() );

    for ( auto l : learnt_clause )
        bump( var_of_lit( l ) );
    increase_bump();

    /** Find the next dec level to which we are jumping. The level
     *  has to be witnessed by some literal. The literal will
     *  be the second in the clause.
     *
     *  Rational: In the next unit propagation, the first literal
     *  is the one which will be asserted, the second will
     *  be the first to be forgotten, thus if the second is false
     *  the rest are also.
     */

    sidx_t next_dec_level = -1;
    sidx_t next_dec_lit = -1;

    assert( learnt_clause.size() != 0 );

    if ( learnt_clause.size() == 1 )
        next_dec_level = 0;
    else
    {
        for ( idx_t i_l = 1; i_l < learnt_clause.size(); i_l++ )
        {
            auto &l = learnt_clause[ i_l ];
            //logger.log( "wow", "%d@%d", -l, lit_level[ -l ] );
            learnt_lit.remove( l );
            assert( lit_level[ -l ] >= 0 );
            if ( lit_level[ -l ] > next_dec_level )
            {
                next_dec_level = lit_level[ -l ];
                assert( next_dec_level >= 0 );
                next_dec_lit = i_l;
            }
        }
    }

    assert( next_dec_level >= 0 );

    // If there are more than two elements, make sure to make
    // the dec. level literal the second.
    if ( learnt_clause.size() > 2 )
    {
        assert( next_dec_lit > 0 );
        std::swap( learnt_clause[ 1 ], learnt_clause[ next_dec_lit ] );
    }


    #ifdef CHECKED
    for ( var_t v = 1; v <= var_count; v++ )
    {
        assert( ! to_resolve.contains( v ) && ! to_resolve.contains( -v ) );
        assert( ! learnt_lit.contains( v ) && ! learnt_lit.contains( -v ) );
    }
    #endif

    assert( next_dec_level >= 0 );

    // Condition: the first literal is the first unique
    //            the second is from
    #ifdef CHECKED
        if ( learnt_clause.size() > 1 )
            assert( lit_level[ - learnt_clause[ 1 ] ] == next_dec_level );
    #endif

    return std::pair( learnt_clause, next_dec_level );
}


idx_t solver::learn( clause_t c )
{
    logger.log( "learn", c );

    //log_trail( "l", *this );

    idx_t i = clauses.size();
    clauses.push_back( c );

    assert( ! c.empty() );
    watched_in[ c[ 0 ] ].push_back( i );
    if ( c.size() > 1 )
        watched_in[ c[ 1 ] ].push_back( i );

    return i;
}

// EVSIDS

void solver::increase_bump()
{
    bump_size *= bump_step;

    double max_bump = 42069420.1337;
    if ( bump_size > max_bump )
    {
        for ( size_t i = 0; i < var_count; i ++ )
        {
            heap.content[ i ].first /= bump_size;
        }
        bump_size = 1.0;
    }
}

void solver::bump( var_t var )
{
    heap.set_priority( var, heap.content[ heap.var_idx[ var ] ].first + bump_size );
}

// Restarts
void solver::restart()
{
    backtrack(0);
    for ( idx_t i_c = 0; i_c < clauses.size(); i_c++ )
        if ( clauses[ i_c ].size() == 1 )
            unit_queue.push_back( i_c );
    conflict_count = 0;
    next_restart = luby_gen.next();
}
