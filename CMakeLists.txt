cmake_minimum_required(VERSION 3.31)
project(NeonFSLib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenSSL configuration
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_library(NeonFSLib STATIC NeonFSLib.cpp)

# Include directories
target_include_directories(NeonFSLib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(NeonFSLib
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

option(PRODUCTION "Production Use" OFF)

if (PRODUCTION)
    if(MSVC)
        target_compile_options(NeonFSLib PRIVATE /W4 /WX)
    else()
        target_compile_options(NeonFSLib PRIVATE -Wall -Wextra -Werror)
    endif()
endif ()