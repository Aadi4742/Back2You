cmake_minimum_required(VERSION 3.14)
project(Back2You VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build test suite" OFF)

# External dependencies via FetchContent
include(FetchContent)

# 1. nlohmann/json for robust JSON handling
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# 2. cpp-httplib for REST API HTTP server
message(STATUS "Fetching cpp-httplib...")
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# 3. SQLite3
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    message(STATUS "System SQLite3 not found. Fetching official SQLite3 amalgamation...")
    FetchContent_Declare(
        sqlite3_amalgamation
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
    )
    FetchContent_MakeAvailable(sqlite3_amalgamation)
    if(TARGET sqlite3_amalgamation)
        add_library(sqlite3 STATIC ${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c)
        target_include_directories(sqlite3 PUBLIC ${sqlite3_amalgamation_SOURCE_DIR})
    endif()
endif()

# Project include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/include
)

# Collect backend sources
file(GLOB_RECURSE BACKEND_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/backend/src/*.cpp"
)

# Main Executable
add_executable(back2you_server ${BACKEND_SOURCES})

# Target link libraries
target_link_libraries(back2you_server PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
)

if(TARGET SQLite::SQLite3)
    target_link_libraries(back2you_server PRIVATE SQLite::SQLite3)
elseif(TARGET sqlite3)
    target_link_libraries(back2you_server PRIVATE sqlite3)
else()
    target_link_libraries(back2you_server PRIVATE sqlite3)
endif()

# Windows specific networking dependencies for httplib
if(WIN32)
    target_link_libraries(back2you_server PRIVATE ws2_32 crypt32)
endif()

# Output directory settings
set_target_properties(back2you_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
