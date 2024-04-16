#pragma once

#include <cassert>

struct luby 
{
    unsigned int iteration = 2; 
    unsigned int current = 1;
    unsigned int acc = 1;

    int next() 
    {
        if ( current == 0 )
        {
            current++;
            return 1;
        }
        current++; 
        acc *= 2;
        if ( current >= iteration )
        {
            current = 0;
            acc = 1;
            iteration++;
        } 
        return acc;
    }
};
