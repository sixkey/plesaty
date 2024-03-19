#include "parser.hpp" 

#include <vector>
#include <string>
#include <iostream>

clause_t parse_clause()
{
    clause_t clause;
    int read = -1; 
    std::cin >> read;
    while ( read != 0 ) {
        clause.push_back( read );
        std::cin >> read;
    }
    return clause;
}


cnf_t parse_dimacs()
{
    std::string s;

    std::cin >> s;

    while ( s == "c" ) 
    {
        std::getline( std::cin, s );
        std::cin >> s;
    }

    unsigned int var_count, clause_count;
    std::cin >> s >> var_count >> clause_count;

    cnf_t cnf{ {}, var_count };
    for( unsigned int i = 0; i < clause_count; i++ )
        cnf.clauses.push_back( parse_clause() );
    return cnf;
}

void show_dimacs( const cnf_t &cnf )
{
    std::cout << "p cnf " << cnf.clauses.size() << " " << cnf.var_count << std::endl;
    for ( auto &c : cnf.clauses )
    {
        for ( auto l : c )
            std::cout << l << " ";
        std::cout << std::endl;
    }
}
