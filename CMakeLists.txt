cmake_minimum_required(VERSION 3.20)

file(STRINGS VERSION APP_VERSION LIMIT_COUNT 1)
project(trrispo25 VERSION ${APP_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)

add_library(trrispo25core STATIC src/core.c)
target_include_directories(trrispo25core PUBLIC src)
target_link_libraries(trrispo25core PUBLIC PkgConfig::ZSTD)

add_executable(trrispo25 src/main.c)
target_compile_definitions(trrispo25 PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(trrispo25 PRIVATE trrispo25core)

install(TARGETS trrispo25 RUNTIME DESTINATION bin)
install(FILES packaging/trrispo25.conf DESTINATION /etc/trrispo25)

