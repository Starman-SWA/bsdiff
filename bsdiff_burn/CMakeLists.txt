cmake_minimum_required(VERSION 3.22)
project(bsdiff C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-DBSDIFF_EXECUTABLE -DBSPATCH_EXECUTABLE)

add_executable(bsdiff bsdiff.c graph.c)
add_executable(bspatch bspatch.c)

find_package (BZip2)
if (BZIP2_FOUND)
    include_directories(${BZIP_INCLUDE_DIRS})
    target_link_libraries (bsdiff ${BZIP2_LIBRARIES})
    target_link_libraries (bspatch ${BZIP2_LIBRARIES})
endif (BZIP2_FOUND)