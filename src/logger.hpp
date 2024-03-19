#pragma once

#include <cstring>
#include <iostream>
#include <string>


struct logger_t
{
    
    #if 0
    template <typename... Args>
    void log( const char *name, const char* format, Args... args) {
        std::string formatAsString = "[%s] ";
        formatAsString += format;
        formatAsString += "\n";
        printf( formatAsString.c_str(), name, args... );
    }
    #else 
    template <typename... Args>
    void log( const char *name, const char* format, Args... args) {
    }
    #endif
};

