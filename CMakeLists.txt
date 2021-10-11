cmake_minimum_required(VERSION 3.15...3.21)

project(
  Protocon
  VERSION 2.2.0
  DESCRIPTION "A C++ implement for Protocon"
  LANGUAGES CXX)

include(FetchContent)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.10.x)
  # For Windows: Prevent overriding the parent project's compiler/linker
  # settings
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  set(INSTALL_GTEST
      OFF
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.6.0)
  set(BENCHMARK_ENABLE_TESTING
      OFF
      CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_INSTALL
      OFF
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googlebenchmark)

  enable_testing()

  add_subdirectory(tests)

  add_subdirectory(benches)

  add_subdirectory(examples)
endif()

find_package(Threads REQUIRED)

FetchContent_Declare(
  sockpp
  GIT_REPOSITORY https://github.com/fpagliughi/sockpp
  GIT_TAG v0.7)
set(SOCKPP_BUILD_SHARED
    OFF
    CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_STATIC
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sockpp)

file(GLOB HEADER_LIST include/*.h)
aux_source_directory(src SOURCE_LIST)

add_library(${PROJECT_NAME} STATIC ${SOURCE_LIST} ${HEADER_LIST})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<INSTALL_INTERFACE:include>
                         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Use BUILD_INTERFACE to avoid exporting.
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads $<BUILD_INTERFACE:sockpp-static>)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(DIRECTORY include/${PROJECT_NAME} DESTINATION include)

install(
  EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION lib/cmake/${PROJECT_NAME})

include(CMakePackageConfigHelpers)

configure_package_config_file(
  cmake/${PROJECT_NAME}Config.cmake.in
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

write_basic_package_version_file(
  ${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION lib/cmake/${PROJECT_NAME})
