cmake_minimum_required( VERSION 3.18 )

project( sat )

add_compile_options( -Wall -Wextra -fmax-errors=1 )

if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    add_compile_options( -DCHECKED )
endif()

add_subdirectory( src )

# add_subdirectory( test )
