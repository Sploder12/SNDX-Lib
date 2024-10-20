file(GLOB_RECURSE SOURCES *.cpp)

add_executable(tests ${SOURCES} )

target_compile_features(tests PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(tests PUBLIC /W4 /WX /utf-8)
else()
  target_compile_options(tests PUBLIC -Wall -Wextra -Wpedantic)
endif()

# GTest 1.15.2
# CLion has issues linking GTest when using vcpkg
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
        DOWNLOAD_EXTRACT_TIMESTAMP ON
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

target_link_libraries(tests PUBLIC GTest::gmock_main GTest::gtest)

include(GoogleTest)
gtest_discover_tests(tests)

file(COPY ./../../test_data DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")