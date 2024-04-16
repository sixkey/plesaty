#pragma once

#include <cassert>

struct luby
{

    unsigned int iteration = 2;
    unsigned int current = 1;
    unsigned int acc;
    unsigned int base_value;

    luby( unsigned int base_value )
        : base_value( base_value )
        , acc( base_value )
    {
    }

    int next()
    {
        if ( current == 0 )
        {
            current++;
            return base_value;
        }
        current++;
        acc *= 2;
        if ( current >= iteration )
        {
            current = 0;
            acc = base_value;
            iteration++;
        }
        return acc;
    }
};
