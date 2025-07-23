include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Function to register test modules
function(register_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE NeonFSLib GTest::gtest_main)
    gtest_discover_tests(${name})
endfunction()

# Register test files
register_test(core_result_tests core/result_tests.cpp)
register_test(secure_allocator_tests core/secure_allocator_tests.cpp)
register_test(aes_gcm_ctx_tests security/aes_gcm_ctx_tests.cpp)
register_test(aes_gcm_ctx_pool_tests security/aes_gcm_ctx_pool_tests.cpp)