cmake_minimum_required(VERSION 3.22)
project(bsdiff C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-DBSDIFF_EXECUTABLE -DBSPATCH_EXECUTABLE)

add_executable(bsdiff bsdiff.c)
add_executable(bspatch bspatch.c)

find_package (LibLZMA)
if (LIBLZMA_FOUND)
    include_directories(${LIBLZMA_INCLUDE_DIRS})
    target_link_libraries (bsdiff ${LIBLZMA_LIBRARIES})
    target_link_libraries (bspatch ${LIBLZMA_LIBRARIES})
endif (LIBLZMA_FOUND)