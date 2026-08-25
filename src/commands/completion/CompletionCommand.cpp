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
  local words="doctor config get-all-cmds interactive red blue rjson bjson compare proof completion fred leafgreen frjson lgjson convert"
  case "${COMP_WORDS[1]}" in
    red) words="summary decode inspect validate repair-checksums events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit" ;;
    blue) words="summary decode inspect validate repair-checksums events validate-batch decode-batch edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit convert" ;;
    rjson) words="inspect validate generate reconstruct migrate schema update_schema generate-batch convert convert_to_frjson convert_to_lgjson" ;;
    bjson) words="inspect validate generate reconstruct migrate schema update_schema generate-batch convert convert_to_lgjson convert_to_frjson" ;;
    fred) words="summary inspect validate repair-checksums decode events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit" ;;
    leafgreen) words="summary inspect validate repair-checksums decode events validate-batch decode-batch edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit" ;;
    frjson) words="inspect validate generate reconstruct migrate schema update_schema generate-batch" ;;
    lgjson) words="inspect validate generate reconstruct migrate schema update_schema generate-batch" ;;
    convert) words="red-firered red-leafgreen blue-firered blue-leafgreen red-to-firered routes inspect explain validate-manifest batch" ;;
    compare) words="progress physical semantic semantic-batch firered-semantic firered-progress firered-pokemon firered-events firered-trainers firered-items firered-fly firered-hall-of-fame bridge" ;;
    proof) words="red fred red-to-firered post-emulator verify" ;;
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
  domains=(doctor config get-all-cmds interactive red blue rjson bjson compare proof completion fred leafgreen frjson lgjson convert)
  if (( CURRENT == 2 )); then
    _describe 'command' domains
    return
  fi
  case "$words[2]" in
    red) _values 'Red command' summary decode inspect validate repair-checksums events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit ;;
    blue) _values 'Blue command' summary decode inspect validate repair-checksums events validate-batch decode-batch edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit convert ;;
    rjson) _values 'Red JSON command' inspect validate generate reconstruct migrate schema update_schema generate-batch convert convert_to_frjson convert_to_lgjson ;;
    bjson) _values 'Blue JSON command' inspect validate generate reconstruct migrate schema update_schema generate-batch convert convert_to_lgjson convert_to_frjson ;;
    fred) _values 'FireRed command' summary inspect validate repair-checksums decode events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit ;;
    leafgreen) _values 'LeafGreen command' summary inspect validate repair-checksums decode events validate-batch decode-batch edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit ;;
    frjson) _values 'FireRed JSON command' inspect validate generate reconstruct migrate schema update_schema generate-batch ;;
    lgjson) _values 'LeafGreen JSON command' inspect validate generate reconstruct migrate schema update_schema generate-batch ;;
    convert) _values 'conversion command' red-firered red-leafgreen blue-firered blue-leafgreen red-to-firered routes inspect explain validate-manifest batch ;;
    compare) _values 'comparison command' progress physical semantic semantic-batch firered-semantic firered-progress firered-pokemon firered-events firered-trainers firered-items firered-fly firered-hall-of-fame bridge ;;
    proof) _values 'proof command' red fred red-to-firered post-emulator verify ;;
    config) _values 'configuration command' show ;;
  esac
}
compdef _pkmn pkmn
)";
  } else {
    output << R"(complete -c pkmn -f
complete -c pkmn -n '__fish_use_subcommand' -a 'doctor config get-all-cmds interactive red blue rjson bjson compare proof completion fred leafgreen frjson lgjson convert'
complete -c pkmn -n '__fish_seen_subcommand_from red' -a 'summary decode inspect validate repair-checksums events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit'
complete -c pkmn -n '__fish_seen_subcommand_from blue' -a 'summary decode inspect validate repair-checksums events validate-batch decode-batch edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit convert'
complete -c pkmn -n '__fish_seen_subcommand_from rjson' -a 'inspect validate generate reconstruct migrate schema update_schema generate-batch convert convert_to_frjson convert_to_lgjson'
complete -c pkmn -n '__fish_seen_subcommand_from bjson' -a 'inspect validate generate reconstruct migrate schema update_schema generate-batch convert convert_to_lgjson convert_to_frjson'
complete -c pkmn -n '__fish_seen_subcommand_from fred' -a 'summary inspect validate repair-checksums decode events validate-batch decode-batch validate-post-emulator edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit'
complete -c pkmn -n '__fish_seen_subcommand_from leafgreen' -a 'summary inspect validate repair-checksums decode events validate-batch decode-batch edit begin-edit edit-session pokemon bag progress pending-edits undo-edit edit-history annotate-edit validate-edit end-edit'
complete -c pkmn -n '__fish_seen_subcommand_from frjson' -a 'inspect validate generate reconstruct migrate schema update_schema generate-batch'
complete -c pkmn -n '__fish_seen_subcommand_from lgjson' -a 'inspect validate generate reconstruct migrate schema update_schema generate-batch'
complete -c pkmn -n '__fish_seen_subcommand_from convert' -a 'red-firered red-leafgreen blue-firered blue-leafgreen red-to-firered routes inspect explain validate-manifest batch'
complete -c pkmn -n '__fish_seen_subcommand_from compare' -a 'progress physical semantic semantic-batch firered-semantic firered-progress firered-pokemon firered-events firered-trainers firered-items firered-fly firered-hall-of-fame bridge'
complete -c pkmn -n '__fish_seen_subcommand_from proof' -a 'red fred red-to-firered post-emulator verify'
complete -c pkmn -n '__fish_seen_subcommand_from config' -a 'show'
)";
  }
  return 0;
}

} // namespace pkmn::cli::commands::completion
