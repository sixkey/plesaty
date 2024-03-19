#pragma once

#include <cstring>
#include <iostream>
#include <string>


struct logger_t
{

    void log( const char *message ) {
#ifdef LOG
        std::cout << message << std::endl;
#endif
    }

    template <typename... Args>
    void log( const char *name, const char* format, Args... args) {
        #ifdef LOG
        std::string formatAsString = "[%s] ";
        formatAsString += format;
        formatAsString += "\n";
        printf( formatAsString.c_str(), name, args... );
        #endif
    }
    template < typename T >
    void log( const char *name, const std::vector< T > &values )
    {
        #ifdef LOG
        std::cout << "[" << name << "] " << "[";
        for ( auto &v : values )
            std::cout << v << ", ";
        std::cout << "]" << std::endl;
        #endif
    }
    template < typename T >
    void log( const char *name, const std::deque< T > &values )
    {
        #ifdef LOG
        std::cout << "[" << name << "] " << "<";
        for ( auto &v : values )
            std::cout << v << ", ";
        std::cout << ">" << std::endl;
        #endif
    }
};

