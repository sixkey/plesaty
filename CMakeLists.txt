cmake_minimum_required( VERSION 3.18 )

project( sat )

add_compile_options( -Wall -Wextra -fmax-errors=1 -DCHECKED )

add_subdirectory( src )

# add_subdirectory( test )
