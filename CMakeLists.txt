cmake_minimum_required(VERSION 3.31)
project(NeonFSLib)

set(CMAKE_CXX_STANDARD 17)

# Tell CMake where to look for OpenSSL
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
set(OPENSSL_USE_STATIC_LIBS TRUE)

find_package(OpenSSL REQUIRED)

# Define source files
set(SOURCES
        NeonFSLib.cpp
)

# Define header files 
set(HEADERS
        include/NeonFS/core/result.hpp
)

# Create library target
add_library(NeonFSLib STATIC ${SOURCES} ${HEADERS})

# Link dependencies
target_link_libraries(NeonFSLib
        OpenSSL::SSL
        OpenSSL::Crypto
)