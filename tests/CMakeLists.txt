# Create test executable
add_executable(result_tests result_tests.cpp)

# Link with main library and any test dependencies
target_link_libraries(result_tests PRIVATE
        NeonFSLib
)

# Add test to CTest
add_test(NAME ResultTests COMMAND result_tests)

# Optional: Add Google Test if you want more advanced testing
option(ENABLE_GTEST "Enable Google Test" OFF)
if(ENABLE_GTEST)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)

    target_link_libraries(result_tests PRIVATE GTest::GTest GTest::Main)
    gtest_discover_tests(result_tests)
endif()