cmake_minimum_required(VERSION 3.20)

project(pkmn-cli VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PKMN_REPRODUCIBLE_BUILD "Remove local source/build paths from compiler output" ON)

include(GNUInstallDirs)
include(CTest)
find_package(Python3 3 REQUIRED COMPONENTS Interpreter)

add_library(pkmn_cli_core
    src/app/CommandCatalog.cpp
    src/app/CommandRouter.cpp
    src/commands/catalog/CatalogCommand.cpp
    src/commands/completion/CompletionCommand.cpp
    src/commands/config/ConfigCommand.cpp
    src/commands/doctor/DoctorCommand.cpp
    src/commands/fred/FredCommand.cpp
    src/commands/compare/CompareCommand.cpp
    src/commands/conversion/ConversionCommand.cpp
    src/commands/proof/ProofCommand.cpp
    src/commands/red/EditCommand.cpp
    src/commands/red/RedCommand.cpp
    src/commands/rjson/RjsonCommand.cpp
    src/red/codec/Gen1Codec.cpp
    src/red/comparison/Comparison.cpp
    src/red/comparison/ProgressComparison.cpp
    src/red/data/Gen1Names.cpp
    src/red/data/Gen1PokemonMath.cpp
    src/red/editing/EditSession.cpp
    src/red/events/EventCatalog.cpp
    src/red/generation/SemanticGenerator.cpp
    src/red/json/RedDecoder.cpp
    src/red/json/RedJsonDocument.cpp
    src/red/proof/PostEmulator.cpp
    src/red/reporting/SaveSummary.cpp
    src/red/save/RedSave.cpp
    src/red/validation/SaveValidator.cpp
    src/util/Sha256.cpp
    src/util/AtomicOutput.cpp
    src/util/OutputPath.cpp
    src/util/ZipWriter.cpp
    src/util/ResourceLocator.cpp
)

file(GLOB FIRERED_NATIVE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/firered/native/*.cpp")
target_sources(pkmn_cli_core PRIVATE ${FIRERED_NATIVE_SOURCES})

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/pokemon-red-usa-europe-v1.template.bin"
    "${CMAKE_CURRENT_BINARY_DIR}/resources/pokemon-red-usa-europe-v1.template.bin"
    COPYONLY
)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/runtime/"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/runtime")

target_include_directories(pkmn_cli_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/firered/native"
)
target_compile_features(pkmn_cli_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(pkmn_cli_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(pkmn_cli_core PRIVATE -Wall -Wextra -Wpedantic)
    if(PKMN_REPRODUCIBLE_BUILD)
        target_compile_options(pkmn_cli_core PRIVATE
            "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=."
            "-ffile-prefix-map=${CMAKE_BINARY_DIR}=./build")
    endif()
endif()

add_executable(pkmn src/main.cpp)
target_link_libraries(pkmn PRIVATE pkmn_cli_core)

install(TARGETS pkmn RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES LICENSE README.md CHANGELOG.md CONTRIBUTING.md SECURITY.md
        THIRD_PARTY_NOTICES.md
        DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(DIRECTORY docs/ DESTINATION "${CMAKE_INSTALL_DOCDIR}/docs"
        FILES_MATCHING PATTERN "*.md")
install(FILES docs/pkmn.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
install(FILES resources/pokemon-red-usa-europe-v1.template.bin
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pkmn/resources")
install(DIRECTORY runtime/ DESTINATION "${CMAKE_INSTALL_DATADIR}/pkmn/runtime"
        PATTERN "__pycache__" EXCLUDE PATTERN "*.pyc" EXCLUDE)
install(FILES packaging/sbom/pkmn-cli.spdx.json
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/packaging")

set(CPACK_PACKAGE_NAME "pkmn-cli")
set(CPACK_PACKAGE_VENDOR "MAQ / BiG MAQ Studios")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Pokemon Red to FireRed conversion and save workflow CLI")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "TGZ")
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MAQ / BiG MAQ Studios")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
endif()
include(CPack)

if(BUILD_TESTING)
    add_executable(pkmn_cli_tests tests/command_router_tests.cpp)
    target_link_libraries(pkmn_cli_tests PRIVATE pkmn_cli_core)
    add_test(NAME pkmn-cli-command-router COMMAND pkmn_cli_tests)
    add_test(NAME pkmn-v2-python-runtime
             COMMAND "${Python3_EXECUTABLE}"
                     "${CMAKE_CURRENT_SOURCE_DIR}/runtime/pkmn_v2_runtime.py"
                     --help)
endif()
