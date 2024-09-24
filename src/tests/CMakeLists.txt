set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
find_package(GTest CONFIG REQUIRED)

link_libraries(GTest::gtest GTest::gtest_main)

file(GLOB_RECURSE SOURCES *.cpp)

add_executable(tests ${SOURCES} "../ogg.c")

target_compile_features(tests PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(tests PUBLIC /W4 /WX /utf-8)
else()
  target_compile_options(tests PUBLIC -Wall -Werror -Wextra -Wpedantic)
endif()

enable_testing()

include(GoogleTest)
gtest_discover_tests(tests)

file(COPY ./../../test_data DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")