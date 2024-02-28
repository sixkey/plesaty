#include <iostream> 
#include "parser.hpp"
#include "solver.hpp"

int main()
{
    cnf_t cnf = parse_dimacs();

    std::cout << "PROBLEM" << std::endl;
    show_dimacs( cnf );

    std::cout << "SOLUTION" << std::endl;
    std::cout << sat_to_str[ sat( cnf ) ] << std::endl;
     
}
