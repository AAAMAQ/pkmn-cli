#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

#include "app/CommandRouter.hpp"

int main(int argc, char* argv[]) {
    std::vector<std::string> arguments;
    arguments.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }

#if defined(_WIN32)
    // Preserve byte-for-byte save data when a pipeline uses standard input or
    // standard output. Windows otherwise performs text-mode newline rewriting.
    for (const auto& argument : arguments) {
        if (argument == "-") {
            _setmode(_fileno(stdin), _O_BINARY);
            _setmode(_fileno(stdout), _O_BINARY);
            break;
        }
    }
#endif

    return pkmn::cli::CommandRouter{}.Run(arguments, std::cout, std::cerr);
}
