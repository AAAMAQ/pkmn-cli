cmake_minimum_required(VERSION 3.20)

project(pkmn-cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CTest)

add_library(pkmn_cli_core
    src/app/CommandRouter.cpp
    src/commands/doctor/DoctorCommand.cpp
    src/integration/ToolDiscovery.cpp
)

target_include_directories(pkmn_cli_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(pkmn_cli_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(pkmn_cli_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(pkmn_cli_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(pkmn src/main.cpp)
target_link_libraries(pkmn PRIVATE pkmn_cli_core)

install(TARGETS pkmn RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DOCDIR}")

if(BUILD_TESTING)
    add_executable(pkmn_cli_tests tests/command_router_tests.cpp)
    target_link_libraries(pkmn_cli_tests PRIVATE pkmn_cli_core)
    add_test(NAME pkmn-cli-command-router COMMAND pkmn_cli_tests)
endif()

