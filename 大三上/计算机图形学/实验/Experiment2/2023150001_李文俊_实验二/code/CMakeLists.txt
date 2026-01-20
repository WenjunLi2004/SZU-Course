# cmake version
cmake_minimum_required(VERSION 3.20)

# Export compile commands for IDE tooling (clangd/cpptools)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy shaders and models to build dir for runtime
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/Models DESTINATION ${CMAKE_BINARY_DIR})

# c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# vcpkg (optional)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

project(Experiment2_OFF_Display)

# target
aux_source_directory("./" PROJECT_SOURCES)
add_executable(main ${PROJECT_SOURCES})
target_include_directories(main PRIVATE include)

# Keep a copy of compile_commands.json at project root for editor integration
add_custom_command(TARGET main POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/compile_commands.json
          ${CMAKE_SOURCE_DIR}/compile_commands.json)

if(APPLE)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    target_link_libraries(main PRIVATE glfw)
    target_link_libraries(main PRIVATE glm::glm)
else()
    find_package(glad CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    target_link_libraries(main PRIVATE glad::glad)
    target_link_libraries(main PRIVATE glfw)
    target_link_libraries(main PRIVATE glm::glm)
endif()