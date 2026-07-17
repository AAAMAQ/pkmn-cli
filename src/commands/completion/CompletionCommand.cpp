#include "commands/completion/CompletionCommand.hpp"

#include <ostream>

#include "app/ExitCode.hpp"

namespace pkmn::cli::commands::completion {

int Run(const std::vector<std::string> &arguments, std::ostream &output,
        std::ostream &error) {
  if (arguments.size() != 1 ||
      (arguments[0] != "bash" && arguments[0] != "zsh" &&
       arguments[0] != "fish")) {
    error << "Usage: pkmn completion <bash|zsh|fish>\n";
    return ToInt(ExitCode::InvalidArguments);
  }
  if (arguments[0] == "bash") {
    output << R"(_pkmn_completion() {
  local cur="${COMP_WORDS[COMP_CWORD]}"
  local words="doctor config get-all-cmds red rjson compare proof completion fred frjson convert"
  case "${COMP_WORDS[1]}" in
    red) words="summary decode inspect validate repair-checksums events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pending-edits undo-edit edit-history annotate-edit validate-edit end-edit" ;;
    rjson) words="inspect validate generate reconstruct migrate schema generate-batch" ;;
    compare) words="progress physical semantic semantic-batch" ;;
    proof) words="red post-emulator verify" ;;
    config) words="show" ;;
  esac
  COMPREPLY=( $(compgen -W "$words" -- "$cur") )
}
complete -F _pkmn_completion pkmn
)";
  } else if (arguments[0] == "zsh") {
    output << R"(#compdef pkmn
_pkmn() {
  local -a domains
  domains=(doctor config get-all-cmds red rjson compare proof completion fred frjson convert)
  if (( CURRENT == 2 )); then
    _describe 'command' domains
    return
  fi
  case "$words[2]" in
    red) _values 'Red command' summary decode inspect validate repair-checksums events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pending-edits undo-edit edit-history annotate-edit validate-edit end-edit ;;
    rjson) _values 'Red JSON command' inspect validate generate reconstruct migrate schema generate-batch ;;
    compare) _values 'comparison command' progress physical semantic semantic-batch ;;
    proof) _values 'proof command' red post-emulator verify ;;
    config) _values 'configuration command' show ;;
  esac
}
compdef _pkmn pkmn
)";
  } else {
    output << R"(complete -c pkmn -f
complete -c pkmn -n '__fish_use_subcommand' -a 'doctor config get-all-cmds red rjson compare proof completion fred frjson convert'
complete -c pkmn -n '__fish_seen_subcommand_from red' -a 'summary decode inspect validate repair-checksums events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pending-edits undo-edit edit-history annotate-edit validate-edit end-edit'
complete -c pkmn -n '__fish_seen_subcommand_from rjson' -a 'inspect validate generate reconstruct migrate schema generate-batch'
complete -c pkmn -n '__fish_seen_subcommand_from compare' -a 'progress physical semantic semantic-batch'
complete -c pkmn -n '__fish_seen_subcommand_from proof' -a 'red post-emulator verify'
complete -c pkmn -n '__fish_seen_subcommand_from config' -a 'show'
)";
  }
  return 0;
}

} // namespace pkmn::cli::commands::completion
