cmake_minimum_required(VERSION 3.23)
project(opencode-client-cpp VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable folder organization in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(OPENCODE_CLIENT_BUILD_EXAMPLES "Build examples" ON)
option(OPENCODE_CLIENT_FETCH_DEPS "Fetch dependencies via FetchContent" ON)

# Dependencies
find_package(Threads REQUIRED)

if(OPENCODE_CLIENT_FETCH_DEPS)
    include(FetchContent)

    # nlohmann/json for JSON parsing
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(json)

    # cpp-httplib for HTTP client (header-only)
    FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.15.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(httplib)
else()
    find_package(nlohmann_json REQUIRED)
    find_package(httplib REQUIRED)
endif()

# =============================================================================
# Library
# =============================================================================

set(OPENCODE_HEADERS
    include/opencode/opencode.hpp
    include/opencode/client.hpp
    include/opencode/session.hpp
    include/opencode/server.hpp
    include/opencode/transport.hpp
    include/opencode/types.hpp
    include/opencode/events.hpp
    include/opencode/process.hpp
)

set(OPENCODE_SOURCES
    src/client.cpp
    src/session.cpp
    src/server.cpp
    src/transport.cpp
    src/types.cpp
    src/events.cpp
)

add_library(opencode-client STATIC
    ${OPENCODE_HEADERS}
    ${OPENCODE_SOURCES}
    $<$<NOT:$<PLATFORM_ID:Windows>>:src/process_posix.cpp>
    $<$<PLATFORM_ID:Windows>:src/process_win32.cpp>
)

# Organize sources in Visual Studio
source_group("Header Files" FILES ${OPENCODE_HEADERS})
source_group("Source Files" FILES ${OPENCODE_SOURCES})
source_group("Source Files\\Platform" FILES src/process_posix.cpp src/process_win32.cpp)

target_include_directories(opencode-client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(opencode-client PUBLIC cxx_std_23)

target_link_libraries(opencode-client
    PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
        httplib::httplib
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(opencode-client PUBLIC ws2_32)
endif()

# =============================================================================
# Examples
# =============================================================================

if(OPENCODE_CLIENT_BUILD_EXAMPLES)
    set(OPENCODE_EXAMPLES
        list-sessions
        multi-turn-chat
        spawn-server
        event-monitor
        interactive-chat
        permission-handler
        project-info
        model-comparison
        session-resume
        streaming-chat
        file-browser
        code-search
        config-manager
        mcp-manager
        tools-explorer
    )

    foreach(example ${OPENCODE_EXAMPLES})
        string(REPLACE "-" "_" example_file ${example})
        add_executable(opencode-${example} examples/${example_file}.cpp)
        target_link_libraries(opencode-${example} PRIVATE opencode-client)
        set_target_properties(opencode-${example} PROPERTIES FOLDER "Examples")
    endforeach()
endif()

# Installation (optional - skip export for FetchContent deps)
include(GNUInstallDirs)

install(TARGETS opencode-client
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
