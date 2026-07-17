cmake_minimum_required(VERSION 3.20)

project(pkmn-cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CTest)

add_library(pkmn_cli_core
    src/app/CommandRouter.cpp
    src/commands/completion/CompletionCommand.cpp
    src/commands/config/ConfigCommand.cpp
    src/commands/doctor/DoctorCommand.cpp
    src/commands/compare/CompareCommand.cpp
    src/commands/proof/ProofCommand.cpp
    src/commands/red/EditCommand.cpp
    src/commands/red/RedCommand.cpp
    src/commands/rjson/RjsonCommand.cpp
    src/red/codec/Gen1Codec.cpp
    src/red/comparison/Comparison.cpp
    src/red/editing/EditSession.cpp
    src/red/generation/SemanticGenerator.cpp
    src/red/json/RedDecoder.cpp
    src/red/json/RedJsonDocument.cpp
    src/red/proof/PostEmulator.cpp
    src/red/save/RedSave.cpp
    src/red/validation/SaveValidator.cpp
    src/util/Sha256.cpp
    src/util/OutputPath.cpp
    src/util/ZipWriter.cpp
    src/util/ResourceLocator.cpp
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/pokemon-red-usa-europe-v1.template.bin"
    "${CMAKE_CURRENT_BINARY_DIR}/resources/pokemon-red-usa-europe-v1.template.bin"
    COPYONLY
)

target_include_directories(pkmn_cli_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)
target_compile_features(pkmn_cli_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(pkmn_cli_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(pkmn_cli_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(pkmn src/main.cpp)
target_link_libraries(pkmn PRIVATE pkmn_cli_core)

install(TARGETS pkmn RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES LICENSE README.md CHANGELOG.md THIRD_PARTY_NOTICES.md
        DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(DIRECTORY docs/ DESTINATION "${CMAKE_INSTALL_DOCDIR}/docs"
        FILES_MATCHING PATTERN "*.md")
install(FILES docs/pkmn.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
install(FILES resources/pokemon-red-usa-europe-v1.template.bin
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pkmn/resources")

if(BUILD_TESTING)
    add_executable(pkmn_cli_tests tests/command_router_tests.cpp)
    target_link_libraries(pkmn_cli_tests PRIVATE pkmn_cli_core)
    add_test(NAME pkmn-cli-command-router COMMAND pkmn_cli_tests)
endif()
