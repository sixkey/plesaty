#include <iostream> 
#include "parser.hpp"
#include "solver.hpp"

void sat_solve( cnf_t cnf )
{
    solver s( std::move( cnf ) );
    sat_t res = s.solve();

    if ( res == UNSAT )
    {
        std::cout << "s UNSATISFIABLE" << std::endl;
        return;
    }

    if ( res == SAT ) 
    {
        std::cout << "s SATISFIABLE" << std::endl;

        // print model
        std::cout << "v ";
        for ( sidx_t v = 1; v <= s.var_count; v++ )
        {
            std::cout << (s.values[ v ] == val_tt ? v : -v) << " ";
        }
        std::cout << "0\n";

        return;
    }

    if ( res == UNKNOWN )
    {
        std::cout << "s UNKNOWN" << std::endl;
        return;
    }
}

int main()
{
    cnf_t cnf = parse_dimacs();

    // std::cout << "PROBLEM" << std::endl;
    // show_dimacs( cnf );

    // std::cout << "SOLUTION" << std::endl;
    sat_solve( std::move( cnf ) );
}

