#include "commands/interactive/options/Workflow.hpp"

#include <stdexcept>
#include <iterator>
#include <unordered_set>

#include "app/CommandCatalog.hpp"

namespace pkmn::cli::commands::interactive::options {

const std::vector<Workflow> &AllWorkflows() {
  static const std::vector<Workflow> all = [] {
    std::vector<Workflow> result;
    for (auto part : {SaveAndJsonWorkflows(), EditAndJapaneseWorkflows(),
                      ConversionCompareProofWorkflows()}) {
      result.insert(result.end(), std::make_move_iterator(part.begin()),
                    std::make_move_iterator(part.end()));
    }
    // The session entry point is handled by the main menu itself.
    result.push_back({"interactive", {}, false,
                      "You are already in the interactive session.", {}});
    std::unordered_set<std::string> catalog;
    for (const auto &command : AvailableCommands())
      catalog.emplace(command.path);
    std::unordered_set<std::string> covered;
    for (const auto &workflow : result) {
      if (!catalog.contains(workflow.path))
        throw std::logic_error("interactive workflow is not in the command catalog: " +
                               workflow.path);
      if (!covered.emplace(workflow.path).second)
        throw std::logic_error("duplicate interactive workflow: " + workflow.path);
      std::unordered_set<std::string> fields;
      for (const auto &field : workflow.fields) {
        if (field.key.empty() || !fields.emplace(field.key).second)
          throw std::logic_error("duplicate or empty interactive field in " +
                                 workflow.path);
        if (field.kind == InputKind::Choice &&
            (field.choices.empty() ||
             (!field.values.empty() && field.values.size() != field.choices.size())))
          throw std::logic_error("invalid interactive choices in " + workflow.path);
        if ((field.emit == EmitKind::Option || field.emit == EmitKind::Switch) &&
            field.option.empty() && !workflow.buildArguments)
          throw std::logic_error("interactive option has no CLI flag in " +
                                 workflow.path);
      }
    }
    for (const auto &command : AvailableCommands())
      if (!covered.contains(std::string(command.path)))
        throw std::logic_error("missing interactive workflow: " +
                               std::string(command.path));
    return result;
  }();
  return all;
}

} // namespace pkmn::cli::commands::interactive::options
