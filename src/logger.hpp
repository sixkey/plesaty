#pragma once

#include <cstring>
#include <iostream>
#include <string>


struct logger_t
{
    
    #if 1
    template <typename... Args>
    void log( const char *name, const char* format, Args... args) {
        std::string formatAsString = "[%s] ";
        formatAsString += format;
        formatAsString += "\n";
        printf( formatAsString.c_str(), name, args... );
    }

    template < typename T >
    void log( const char *name, const std::vector< T > &values )
    {
        std::cout << "[" << name << "] " << "[";
        for ( auto &v : values )
            std::cout << v << ", ";
        std::cout << "]" << std::endl;
    }
    template < typename T >
    void log( const char *name, const std::deque< T > &values )
    {
        std::cout << "[" << name << "] " << "<";
        for ( auto &v : values )
            std::cout << v << ", ";
        std::cout << ">" << std::endl;
    }
    #else 
    template <typename... Args>
    void log( const char *name, const char* format, Args... args) {
    }
    #endif
};

