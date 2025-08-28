# CMakeList.txt : CMake project for Shooter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Shooter")

find_package(SDL3 REQUIRED)

find_package(SDL3_image REQUIRED)

# Add source to this project's executable.
add_executable (Shooter "Shooter.cpp" "Shooter.h" "timer.h" "animation.h" "gameobject.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Shooter PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
target_link_libraries(Shooter PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)
target_include_directories(Shooter PRIVATE "ext/")