cmake_minimum_required(VERSION 3.20)

file(STRINGS VERSION APP_VERSION LIMIT_COUNT 1)
project(trrispo25 VERSION ${APP_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)
pkg_check_modules(MHD REQUIRED IMPORTED_TARGET libmicrohttpd)

option(ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ENABLE_COVERAGE   "Build with gcov coverage instrumentation"               OFF)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

add_library(trrispo25core STATIC src/core.c src/server.c)
target_link_libraries(trrispo25core PUBLIC PkgConfig::ZSTD PkgConfig::MHD)
target_include_directories(trrispo25core PUBLIC src)

add_executable(trrispo25 src/main.c)
target_compile_definitions(trrispo25 PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(trrispo25 PRIVATE trrispo25core)

include(CTest)
if(BUILD_TESTING)
    # --- unit-тесты core ---
    add_executable(test_core tests/test_core.c)
    target_link_libraries(test_core PRIVATE trrispo25core)
    add_test(NAME core COMMAND test_core)

    # --- CLI-тест 1: --version печатает версию проекта ---
    add_test(NAME cli_version COMMAND trrispo25 --version)
    set_tests_properties(cli_version PROPERTIES
        PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

    # --- CLI-тест 2: неизвестный ключ — ненулевой код ---
    add_test(NAME cli_bad_args COMMAND trrispo25 --no-such-option)
    set_tests_properties(cli_bad_args PROPERTIES WILL_FAIL TRUE)
endif()

install(TARGETS trrispo25 RUNTIME DESTINATION bin)
install(FILES packaging/trrispo25.conf DESTINATION /etc/trrispo25)

