#include "sequences.hpp"
#include <iostream>

int main()
{
    luby l;
    for ( size_t i = 0; i < 50; i++ )
    {
        std::cout << l.next() << std::endl;
    }
}
