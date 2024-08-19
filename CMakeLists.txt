# CMakeList.txt : CMake project for SNDXlib, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.12)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project ("SNDXlib")

find_package(OpenAL CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_path(MINIMP3_INCLUDE_DIRS "minimp3/minimp3.h")
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)

include_directories(lib/include src/include/sndx)

include_directories(${Stb_INCLUDE_DIR} ${MINIMP3_INCLUDE_DIRS})
link_libraries(OpenAL::OpenAL glm::glm glfw GLEW::GLEW Freetype::Freetype)

add_subdirectory(src/tests)
