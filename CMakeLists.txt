# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)

add_library( k2pdfoptjni

             SHARED

             src/main/cpp/jni.cpp )

include_directories(src/main/cpp/)

add_definitions(-DUSE_CMAKE)

# one day we will be able to replace those lines with find_library(k2pdfopt-lib k2pdfopt)
add_library(k2pdfopt SHARED IMPORTED)
string(REPLACE "/android-k2pdfopt/intermediates/" "/libk2pdfopt/intermediates/" K2PDFOPT_TARGET "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libk2pdfopt.so")
include_directories( libk2pdfopt/src/main/cpp/include_mod libk2pdfopt/src/main/cpp/k2pdfopt )
set_target_properties(k2pdfopt PROPERTIES IMPORTED_LOCATION ${K2PDFOPT_TARGET})

add_library(willus SHARED IMPORTED)
string(REPLACE "/android-k2pdfopt/intermediates/" "/libk2pdfopt/intermediates/" WILLUS_TARGET "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwillus.so")
include_directories( libk2pdfopt/src/main/cpp/include_mod libk2pdfopt/src/main/cpp/willus )
set_target_properties(willus PROPERTIES IMPORTED_LOCATION ${WILLUS_TARGET})

find_library( log-lib log )
find_library( android-lib android )
find_library( jnigraphics-lib jnigraphics )

target_link_libraries( k2pdfoptjni
                       ${log-lib} ${android-lib} ${jnigraphics-lib} k2pdfopt willus )
