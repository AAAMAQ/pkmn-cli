#include "commands/compare/CompareCommand.hpp"
#include "app/ExitCode.hpp"
#include "red/comparison/Comparison.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "red/save/RedSave.hpp"
#include <fstream>
#include <ostream>
namespace pkmn::cli::commands::compare {
int Run(const std::vector<std::string> &a, std::ostream &out,
        std::ostream &err) {
  if (a.empty() || a[0] == "help" || a[0] == "--help") {
    out << "Usage:\n  pkmn compare physical <a.sav> <b.sav>\n  pkmn compare "
           "semantic <a.red.json> <b.red.json>\n";
    return 0;
  }
  if (a.size() != 3 || (a[0] != "physical" && a[0] != "semantic")) {
    err << "pkmn compare: invalid arguments\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  try {
    if (a[0] == "physical") {
      const auto left = red::save::RedSave::Read(a[1]);
      const auto right = red::save::RedSave::Read(a[2]);
      const auto report =
          red::comparison::ComparePhysical(left.BytesView(), right.BytesView());
      out << red::comparison::PhysicalMarkdown(report);
      return 0;
    }
    const auto left = red::json::LoadAndValidate(a[1]);
    const auto right = red::json::LoadAndValidate(a[2]);
    if (!left.validation.Valid() || !right.validation.Valid())
      throw std::runtime_error("both semantic inputs must validate");
    const auto report = red::comparison::CompareSemantic(left.root, right.root);
    out << red::comparison::SemanticMarkdown(report);
    return report.at("equivalent").get<bool>()
               ? 0
               : ToInt(ExitCode::SemanticMismatch);
  } catch (const std::exception &e) {
    err << "pkmn compare: " << e.what() << '\n';
    return ToInt(ExitCode::InvalidInput);
  }
}
} // namespace pkmn::cli::commands::compare
