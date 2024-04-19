cmake_minimum_required( VERSION 3.18 )

project( sat )

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release)
endif()

add_compile_options( -Wall -Wextra -fmax-errors=1 )

if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    add_compile_options( -DCHECKED -DLOG )
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" )

add_subdirectory( src )

# add_subdirectory( test )
