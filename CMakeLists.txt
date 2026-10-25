cmake_minimum_required(VERSION 3.16)
project(ModeratedLMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LMS_BUILD_TESTS "Build the GoogleTest suite" ON)
option(LMS_COVERAGE "Instrument for gcov line coverage (GCC/Clang)" OFF)
option(LMS_WERROR "Treat warnings as errors" OFF)

find_package(SQLite3 REQUIRED)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(SODIUM QUIET IMPORTED_TARGET libsodium)
endif()
if(TARGET PkgConfig::SODIUM)
  set(LMS_SODIUM_TARGET PkgConfig::SODIUM)
else()
  find_library(SODIUM_LIB NAMES sodium libsodium REQUIRED)
  find_path(SODIUM_INCLUDE NAMES sodium.h REQUIRED)
  add_library(lms_sodium INTERFACE)
  target_link_libraries(lms_sodium INTERFACE ${SODIUM_LIB})
  target_include_directories(lms_sodium INTERFACE ${SODIUM_INCLUDE})
  set(LMS_SODIUM_TARGET lms_sodium)
endif()

add_library(lms_core STATIC
  src/common/DateTime.cpp src/common/Validation.cpp
  src/domain/Types.cpp src/domain/User.cpp
  src/strategy/LatePenalty.cpp
  src/security/SodiumPasswordHasher.cpp
  src/data/Database.cpp src/data/SqliteRepositories.cpp
  src/services/AccessControl.cpp src/services/AuthService.cpp src/services/UserService.cpp
  src/services/AcademicService.cpp src/services/AssignmentService.cpp
  src/services/SubmissionService.cpp src/services/AssessmentService.cpp
  src/ui/ConsoleApp.cpp)
target_include_directories(lms_core PUBLIC src)
target_link_libraries(lms_core PUBLIC SQLite::SQLite3 ${LMS_SODIUM_TARGET})

if(MSVC)
  target_compile_options(lms_core PUBLIC /W4)
  if(LMS_WERROR)
    target_compile_options(lms_core PUBLIC /WX)
  endif()
else()
  target_compile_options(lms_core PUBLIC -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
  if(LMS_WERROR)
    target_compile_options(lms_core PUBLIC -Werror)
  endif()
endif()

if(LMS_COVERAGE AND NOT MSVC)
  target_compile_options(lms_core PUBLIC --coverage -O0 -g)
  target_link_options(lms_core PUBLIC --coverage)
endif()

add_executable(lms src/main.cpp)
target_link_libraries(lms PRIVATE lms_core)
add_executable(lms_seed tools/seed_demo.cpp)
target_link_libraries(lms_seed PRIVATE lms_core)

if(LMS_BUILD_TESTS)
  enable_testing()
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()
  file(GLOB LMS_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
  add_executable(lms_tests ${LMS_TEST_SOURCES})
  target_link_libraries(lms_tests PRIVATE lms_core GTest::gtest GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(lms_tests DISCOVERY_TIMEOUT 120)
endif()
