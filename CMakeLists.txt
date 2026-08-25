cmake_minimum_required(VERSION 3.20)

project(pkmn-cli VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PKMN_REPRODUCIBLE_BUILD "Remove local source/build paths from compiler output" ON)

include(GNUInstallDirs)
include(CTest)

set(PKMN_BUNDLED_RUNTIME_EXECUTABLE "" CACHE FILEPATH
    "Prebuilt private pkmn-runtime executable installed with release packages")

add_library(pkmn_cli_core
    src/app/CommandCatalog.cpp
    src/app/CommandRouter.cpp
    src/commands/catalog/CatalogCommand.cpp
    src/commands/completion/CompletionCommand.cpp
    src/commands/config/ConfigCommand.cpp
    src/commands/doctor/DoctorCommand.cpp
    src/commands/fred/FredCommand.cpp
    src/commands/interactive/InteractiveCommand.cpp
    src/commands/paired/PairedCommand.cpp
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
    src/conversion/RouteRegistry.cpp
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
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/pokemon-firered-usa-europe-v1.template.bin"
    "${CMAKE_CURRENT_BINARY_DIR}/resources/pokemon-firered-usa-europe-v1.template.bin"
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
if(PKMN_BUNDLED_RUNTIME_EXECUTABLE)
    if(NOT EXISTS "${PKMN_BUNDLED_RUNTIME_EXECUTABLE}")
        message(FATAL_ERROR
            "PKMN_BUNDLED_RUNTIME_EXECUTABLE does not exist: ${PKMN_BUNDLED_RUNTIME_EXECUTABLE}")
    endif()
    if(WIN32)
        get_filename_component(PKMN_BUNDLED_RUNTIME_DIRECTORY
                               "${PKMN_BUNDLED_RUNTIME_EXECUTABLE}" DIRECTORY)
        install(DIRECTORY "${PKMN_BUNDLED_RUNTIME_DIRECTORY}/"
                DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/pkmn/runtime"
                USE_SOURCE_PERMISSIONS)
    else()
        get_filename_component(PKMN_BUNDLED_RUNTIME_DIRECTORY
                               "${PKMN_BUNDLED_RUNTIME_EXECUTABLE}" DIRECTORY)
        install(DIRECTORY "${PKMN_BUNDLED_RUNTIME_DIRECTORY}/"
                DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/pkmn/runtime"
                USE_SOURCE_PERMISSIONS)
    endif()
endif()
install(FILES LICENSE README.md CHANGELOG.md CONTRIBUTING.md SECURITY.md
        THIRD_PARTY_NOTICES.md
        DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(DIRECTORY docs/ DESTINATION "${CMAKE_INSTALL_DOCDIR}/docs"
        FILES_MATCHING PATTERN "*.md")
install(FILES docs/pkmn.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
install(FILES resources/pokemon-red-usa-europe-v1.template.bin
              resources/pokemon-firered-usa-europe-v1.template.bin
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pkmn/resources")
install(DIRECTORY runtime/ DESTINATION "${CMAKE_INSTALL_DATADIR}/pkmn/runtime"
        PATTERN "__pycache__" EXCLUDE PATTERN "*.pyc" EXCLUDE)
install(FILES packaging/sbom/pkmn-cli.spdx.json
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/packaging")
if(WIN32)
    install(PROGRAMS packaging/windows/pkmn-interactive.cmd
            DESTINATION "${CMAKE_INSTALL_BINDIR}")
elseif(APPLE)
    install(PROGRAMS packaging/macos/pkmn-interactive.command
            DESTINATION "${CMAKE_INSTALL_BINDIR}")
else()
    install(PROGRAMS packaging/linux/pkmn-interactive
            DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES packaging/linux/pkmn-interactive.desktop
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()

set(CPACK_PACKAGE_NAME "pkmn-cli")
set(CPACK_PACKAGE_VENDOR "MAQ / BiG MAQ Studios")
set(CPACK_PACKAGE_VERSION "3.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Pokemon Red and Blue to FireRed and LeafGreen save conversion CLI")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
               "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")
set(CPACK_GENERATOR "TGZ")
if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "pkmn-cli-${CPACK_PACKAGE_VERSION}-windows-x86_64")
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "pkmn 3.0")
    set(CPACK_NSIS_PACKAGE_NAME "pkmn 3.0")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_MENU_LINKS "bin/pkmn-interactive.cmd" "pkmn interactive")
elseif(APPLE)
    set(CPACK_PACKAGE_FILE_NAME
        "pkmn-cli-${CPACK_PACKAGE_VERSION}-macos-${CMAKE_SYSTEM_PROCESSOR}")
    # The release workflow builds the macOS installer directly with pkgbuild.
    # CPack's productbuild generator emits a distribution wrapper whose payload
    # packages are stored separately, which is unsuitable as a standalone
    # GitHub Release asset.
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_PACKAGE_FILE_NAME
        "pkmn-cli-${CPACK_PACKAGE_VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}")
endif()
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MAQ / BiG MAQ Studios")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
endif()
include(CPack)

if(BUILD_TESTING)
    find_package(Python3 3 REQUIRED COMPONENTS Interpreter)
    add_executable(pkmn_cli_tests tests/command_router_tests.cpp)
    target_link_libraries(pkmn_cli_tests PRIVATE pkmn_cli_core)
    add_test(NAME pkmn-cli-command-router COMMAND pkmn_cli_tests)
    add_test(NAME pkmn-v2-python-runtime
             COMMAND "${Python3_EXECUTABLE}"
                     "${CMAKE_CURRENT_SOURCE_DIR}/runtime/pkmn_v2_runtime.py"
                     --help)
    add_test(NAME pkmn-firered-template-policy
             COMMAND "${Python3_EXECUTABLE}"
                     "${CMAKE_CURRENT_SOURCE_DIR}/tests/firered_template_policy_test.py"
                     "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME pkmn-paired-origin-profile
             COMMAND "${Python3_EXECUTABLE}"
                     "${CMAKE_CURRENT_SOURCE_DIR}/tests/paired_origin_profile_test.py"
                     "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
