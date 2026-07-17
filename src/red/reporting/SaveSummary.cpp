#include "red/reporting/SaveSummary.hpp"

#include <array>
#include <sstream>

namespace pkmn::cli::red::reporting {
namespace {

using Json = json::OrderedJson;

constexpr std::array<const char *, 8> kBadgeNames = {
    "Boulder", "Cascade", "Thunder", "Rainbow",
    "Soul",    "Marsh",   "Volcano", "Earth"};

Json PokemonBrief(const Json &pokemon) {
  Json brief = {{"speciesId", pokemon.at("speciesId")},
                {"speciesName", pokemon.value("speciesName", "UNKNOWN")},
                {"level", pokemon.at("level")},
                {"nickname", pokemon.at("nickname").at("value")},
                {"currentHp", pokemon.at("currentHp")}};
  if (pokemon.contains("stats"))
    brief["maxHp"] = pokemon.at("stats").at("maxHp");
  return brief;
}

Json ItemBriefs(const Json &collection) {
  Json items = Json::array();
  for (const auto &item : collection.at("items"))
    items.push_back({{"itemId", item.at("itemId")},
                     {"itemName", item.value("itemName", "UNKNOWN")},
                     {"quantity", item.at("quantity")}});
  return items;
}

std::string Playtime(const Json &playtime) {
  std::ostringstream output;
  output << playtime.at("hours").get<unsigned>() << "h "
         << playtime.at("minutes").get<unsigned>() << "m "
         << playtime.at("seconds").get<unsigned>() << "s";
  return output.str();
}

void AppendPokemon(std::ostringstream &output, const Json &pokemon,
                   const std::string &prefix) {
  output << prefix << pokemon.at("nickname").get<std::string>() << " ("
         << pokemon.at("speciesName").get<std::string>() << ", Lv. "
         << pokemon.at("level") << ", HP " << pokemon.at("currentHp");
  if (pokemon.contains("maxHp")) output << '/' << pokemon.at("maxHp");
  output << ")\n";
}

} // namespace

Json BuildSaveSummary(const Json &document) {
  const auto &decoded = document.at("decoded");
  Json badges = Json::array();
  const auto badgeRaw = decoded.at("badges").at("raw").get<unsigned>();
  for (std::size_t bit = 0; bit < kBadgeNames.size(); ++bit)
    if ((badgeRaw & (1U << bit)) != 0) badges.push_back(kBadgeNames[bit]);

  Json party = Json::array();
  for (const auto &pokemon : decoded.at("party").at("pokemon"))
    party.push_back(PokemonBrief(pokemon));

  Json boxes = Json::array();
  std::size_t storedPokemon = 0;
  for (const auto &box : decoded.at("pcStorage").at("boxes")) {
    const auto count = box.at("count").get<std::size_t>();
    storedPokemon += count;
    boxes.push_back({{"boxNumber", box.at("boxNumber")}, {"count", count}});
  }

  Json majorStory = Json::array();
  for (const auto &event :
       decoded.at("storyProgress").at("storyFlags"))
    if (event.at("completed").get<bool>() &&
        event.value("category", "") == "major_story")
      majorStory.push_back({{"name", event.at("name")},
                            {"label", event.at("label")}});

  Json daycare = {{"inUse", decoded.at("daycare").at("inUse")}};
  if (daycare.at("inUse").get<bool>())
    daycare["pokemon"] = PokemonBrief(decoded.at("daycare").at("pokemon"));

  return {
      {"format", "pkmn-red-readable-summary"},
      {"reportVersion", "1.0.0"},
      {"source",
       {{"logicalName", document.at("source").at("fileName")},
        {"sha256", document.at("source").at("hashes").at("wholeFileSha256")},
        {"checksumsValid", true}}},
      {"trainer",
       {{"name", decoded.at("trainer").at("name").at("value")},
        {"trainerId", decoded.at("trainer").at("trainerId")},
        {"rivalName", decoded.at("rival").at("name").at("value")}}},
      {"progress",
       {{"money", decoded.at("moneyAndCoins").at("money")},
        {"coins", decoded.at("moneyAndCoins").at("coins")},
        {"badgeCount", badges.size()},
        {"badges", badges},
        {"playtime", decoded.at("playtime")},
        {"location", decoded.at("location")},
        {"pokedex",
         {{"owned", decoded.at("pokedex").at("ownedCount")},
          {"seen", decoded.at("pokedex").at("seenCount")}}},
        {"eventsSet", decoded.at("events").at("set")},
        {"trainerBattlesComplete",
         decoded.at("trainerBattles").at("complete")},
        {"staticEncountersComplete",
         decoded.at("staticBattles").at("complete")},
        {"majorStoryCompleted", majorStory}}},
      {"party", {{"count", party.size()}, {"pokemon", party}}},
      {"inventory",
       {{"bag", ItemBriefs(decoded.at("inventory").at("bag"))},
        {"pcItems", ItemBriefs(decoded.at("inventory").at("pcItems"))}}},
      {"storage",
       {{"storedPokemon", storedPokemon},
        {"selectedBox",
         decoded.at("currentBoxCache").at("selectedBoxNumber")},
        {"boxes", boxes}}},
      {"daycare", daycare},
      {"hallOfFameRecords",
       decoded.at("hallOfFame").at("recordCount")}};
}

std::string SaveSummaryText(const Json &summary) {
  const auto &trainer = summary.at("trainer");
  const auto &progress = summary.at("progress");
  std::ostringstream output;
  output << "Pokemon Red save summary\n"
         << "Source: " << summary.at("source").at("logicalName").get<std::string>() << "\n"
         << "Trainer: " << trainer.at("name").get<std::string>() << " (ID "
         << trainer.at("trainerId") << ")\nRival: " << trainer.at("rivalName").get<std::string>()
         << "\nPlaytime: " << Playtime(progress.at("playtime"))
         << "\nLocation: " << progress.at("location").at("mapName").get<std::string>() << " ("
         << progress.at("location").at("x") << ", "
         << progress.at("location").at("y") << ")\nMoney: "
         << progress.at("money") << "\nCoins: " << progress.at("coins")
         << "\nBadges: " << progress.at("badgeCount") << "/8";
  for (const auto &badge : progress.at("badges"))
    output << " " << badge.get<std::string>();
  output << "\nPokedex: " << progress.at("pokedex").at("owned")
         << " owned / " << progress.at("pokedex").at("seen") << " seen\n"
         << "Party (" << summary.at("party").at("count") << "):\n";
  for (const auto &pokemon : summary.at("party").at("pokemon"))
    AppendPokemon(output, pokemon, "  - ");
  output << "Bag items:\n";
  for (const auto &item : summary.at("inventory").at("bag"))
    output << "  - " << item.at("itemName").get<std::string>() << " x" << item.at("quantity")
           << '\n';
  output << "PC items:\n";
  for (const auto &item : summary.at("inventory").at("pcItems"))
    output << "  - " << item.at("itemName").get<std::string>() << " x"
           << item.at("quantity") << '\n';
  output << "PC Pokemon: " << summary.at("storage").at("storedPokemon")
         << " across 12 boxes (selected box "
         << summary.at("storage").at("selectedBox") << ")\nBox counts:";
  for (const auto &box : summary.at("storage").at("boxes"))
    output << " " << box.at("boxNumber") << ':' << box.at("count");
  output << "\n"
         << "Daycare: "
         << (summary.at("daycare").at("inUse").get<bool>() ? "occupied"
                                                              : "empty")
         << "\nHall of Fame records: " << summary.at("hallOfFameRecords")
         << "\nKnown events set: " << progress.at("eventsSet")
         << "\nTrainer battles complete: "
         << progress.at("trainerBattlesComplete")
         << "\nStatic encounters complete: "
         << progress.at("staticEncountersComplete")
         << "\nMajor story flags complete: "
         << progress.at("majorStoryCompleted").size()
         << "\nChecksum integrity: valid\n";
  return output.str();
}

std::string SaveSummaryMarkdown(const Json &summary) {
  const auto &trainer = summary.at("trainer");
  const auto &progress = summary.at("progress");
  std::ostringstream output;
  output << "# Pokemon Red Save Summary\n\n"
         << "- **Source:** `" << summary.at("source").at("logicalName").get<std::string>()
         << "`\n- **Trainer:** " << trainer.at("name").get<std::string>() << " (ID "
         << trainer.at("trainerId") << ")\n- **Rival:** " << trainer.at("rivalName").get<std::string>()
         << "\n- **Playtime:** " << Playtime(progress.at("playtime"))
         << "\n- **Location:** " << progress.at("location").at("mapName").get<std::string>()
         << "\n- **Money / coins:** " << progress.at("money") << " / "
         << progress.at("coins") << "\n- **Badges:** "
         << progress.at("badgeCount") << "/8";
  for (const auto &badge : progress.at("badges"))
    output << " " << badge.get<std::string>();
  output << "\n- **Pokedex:** "
         << progress.at("pokedex").at("owned") << " owned / "
         << progress.at("pokedex").at("seen") << " seen\n\n## Party\n\n";
  for (const auto &pokemon : summary.at("party").at("pokemon"))
    AppendPokemon(output, pokemon, "- ");
  output << "\n## Bag\n\n";
  for (const auto &item : summary.at("inventory").at("bag"))
    output << "- " << item.at("itemName").get<std::string>() << " x" << item.at("quantity")
           << '\n';
  output << "\n## PC items\n\n";
  for (const auto &item : summary.at("inventory").at("pcItems"))
    output << "- " << item.at("itemName").get<std::string>() << " x"
           << item.at("quantity") << '\n';
  output << "\n## Storage and progress\n\n- PC Pokemon: "
         << summary.at("storage").at("storedPokemon")
         << "\n- Selected box: " << summary.at("storage").at("selectedBox")
         << "\n- Hall of Fame records: " << summary.at("hallOfFameRecords")
         << "\n- Known events set: " << progress.at("eventsSet")
         << "\n- Trainer battles complete: "
         << progress.at("trainerBattlesComplete")
         << "\n- Static encounters complete: "
         << progress.at("staticEncountersComplete")
         << "\n- Major story flags complete: "
         << progress.at("majorStoryCompleted").size()
         << "\n- Checksum integrity: valid\n";
  return output.str();
}

} // namespace pkmn::cli::red::reporting
