#include <iostream> 
#include "parser.hpp"
#include "solver.hpp"

void sat_solve( cnf_t cnf )
{
    solver s( std::move( cnf ) );
    sat_t res = s.solve();

    if ( res == UNSAT )
    {
        std::cout << "UNSAT" << std::endl;
        return;
    }

    if ( res == SAT ) 
    {
        std::cout << "SAT" << std::endl;

        std::cout << "[";
        for ( idx_t v = 0; v < s.var_count; v++ )
        {
            std::cout << s.values[ v + 1 ] << ", ";
        }
        std::cout << "]\n";

        return;
    }

    if ( res == UNKNOWN )
    {
        std::cout << "UNKNOWN" << std::endl;
        return;
    }
}

int main()
{
    cnf_t cnf = parse_dimacs();

    std::cout << "PROBLEM" << std::endl;
    show_dimacs( cnf );

    std::cout << "SOLUTION" << std::endl;
    sat_solve( std::move( cnf ) );
}

