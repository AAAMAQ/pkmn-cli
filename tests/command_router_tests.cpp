#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "app/CommandRouter.hpp"
#include "app/ExitCode.hpp"

namespace {

int failures = 0;

void Expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

struct Result {
    int code;
    std::string output;
    std::string error;
};

Result Run(std::vector<std::string> arguments) {
    std::ostringstream output;
    std::ostringstream error;
    const int code = pkmn::cli::CommandRouter{}.Run(arguments, output, error);
    return {code, output.str(), error.str()};
}

}  // namespace

int main() {
    const auto noArguments = Run({});
    Expect(noArguments.code == 0, "no arguments should show help successfully");
    Expect(noArguments.output.find("Usage:") != std::string::npos, "help should contain Usage");

    const auto help = Run({"--help"});
    Expect(help.code == 0, "--help should succeed");
    Expect(help.output.find("physicalImage") != std::string::npos,
           "help should state the physicalImage safety boundary");

    const auto version = Run({"--version"});
    Expect(version.code == 0, "--version should succeed");
    Expect(version.output == "pkmn 0.1.0\n", "version output should be stable");

    const auto unknown = Run({"wat"});
    Expect(unknown.code == pkmn::cli::ToInt(pkmn::cli::ExitCode::InvalidArguments),
           "unknown command should use invalid-arguments exit code");

    const auto planned = Run({"red", "decode", "sample.sav"});
    Expect(planned.code == pkmn::cli::ToInt(pkmn::cli::ExitCode::UnsupportedOperation),
           "planned domain should fail honestly as unsupported");

    const auto future = Run({"fred", "decode", "sample.sav"});
    Expect(future.error.find("emulator-proven") != std::string::npos,
           "FireRed placeholder should state its verification gate");

    const auto doctorHelp = Run({"doctor", "--help"});
    Expect(doctorHelp.code == 0, "doctor help should succeed without dependencies");
    Expect(doctorHelp.output.find("PKMN_RED_SAVE_GENIE") != std::string::npos,
           "doctor help should document environment overrides");

    if (failures == 0) {
        std::cout << "All command-router tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}

