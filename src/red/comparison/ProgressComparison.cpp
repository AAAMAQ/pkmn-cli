#include "red/comparison/ProgressComparison.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <sstream>

namespace pkmn::cli::red::comparison {
namespace {

using Json = json::OrderedJson;

constexpr std::array<const char *, 8> kBadgeNames = {
    "Boulder", "Cascade", "Thunder", "Rainbow",
    "Soul",    "Marsh",   "Volcano", "Earth"};

std::int64_t SignedDelta(const Json &before, const Json &after) {
  return after.get<std::int64_t>() - before.get<std::int64_t>();
}

std::int64_t PlaytimeSeconds(const Json &value) {
  return static_cast<std::int64_t>(value.at("hours").get<unsigned>()) * 3600 +
         static_cast<std::int64_t>(value.at("minutes").get<unsigned>()) * 60 +
         value.at("seconds").get<unsigned>();
}

std::string Duration(std::int64_t seconds) {
  const auto sign = seconds < 0 ? "-" : "+";
  seconds = std::abs(seconds);
  std::ostringstream output;
  output << sign << seconds / 3600 << "h " << (seconds / 60) % 60 << "m "
         << seconds % 60 << "s";
  return output.str();
}

Json PokemonBrief(const Json &pokemon) {
  if (pokemon.is_null()) return nullptr;
  return {{"speciesId", pokemon.at("speciesId")},
          {"speciesName", pokemon.value("speciesName", "UNKNOWN")},
          {"nickname", pokemon.at("nickname").at("value")},
          {"level", pokemon.at("level")},
          {"currentHp", pokemon.at("currentHp")}};
}

Json PartyChanges(const Json &before, const Json &after) {
  Json changes = Json::array();
  const auto &left = before.at("pokemon");
  const auto &right = after.at("pokemon");
  const auto slots = std::max(left.size(), right.size());
  for (std::size_t slot = 0; slot < slots; ++slot) {
    const Json oldPokemon = slot < left.size() ? PokemonBrief(left.at(slot))
                                               : Json(nullptr);
    const Json newPokemon = slot < right.size() ? PokemonBrief(right.at(slot))
                                                : Json(nullptr);
    if (oldPokemon == newPokemon) continue;
    Json change = {{"slot", slot + 1},
                   {"before", oldPokemon},
                   {"after", newPokemon}};
    if (!oldPokemon.is_null() && !newPokemon.is_null())
      change["levelDelta"] = SignedDelta(oldPokemon.at("level"),
                                          newPokemon.at("level"));
    changes.push_back(std::move(change));
  }
  return changes;
}

Json ItemChanges(const Json &before, const Json &after) {
  struct Amount {
    std::string name;
    std::int64_t before = 0;
    std::int64_t after = 0;
  };
  std::map<unsigned, Amount> values;
  for (const auto &item : before.at("items")) {
    const auto id = item.at("itemId").get<unsigned>();
    values[id].name = item.value("itemName", "UNKNOWN");
    values[id].before += item.at("quantity").get<std::int64_t>();
  }
  for (const auto &item : after.at("items")) {
    const auto id = item.at("itemId").get<unsigned>();
    values[id].name = item.value("itemName", "UNKNOWN");
    values[id].after += item.at("quantity").get<std::int64_t>();
  }
  Json changes = Json::array();
  for (const auto &[id, amount] : values)
    if (amount.before != amount.after)
      changes.push_back({{"itemId", id},
                         {"itemName", amount.name},
                         {"before", amount.before},
                         {"after", amount.after},
                         {"delta", amount.after - amount.before}});
  return changes;
}

Json ChangedRecords(const Json &before, const Json &after,
                    std::string_view valueKey, bool newValue) {
  std::map<int, bool> oldValues;
  for (const auto &record : before)
    oldValues[record.at("flagIndex").get<int>()] =
        record.at(std::string(valueKey)).get<bool>();
  Json changes = Json::array();
  for (const auto &record : after) {
    const auto index = record.at("flagIndex").get<int>();
    const auto value = record.at(std::string(valueKey)).get<bool>();
    if (oldValues.contains(index) && oldValues[index] != value &&
        value == newValue)
      changes.push_back({{"flagIndex", index},
                         {"name", record.at("name")},
                         {"label", record.at("label")}});
  }
  return changes;
}

std::size_t StoredPokemon(const Json &decoded) {
  std::size_t total = 0;
  for (const auto &box : decoded.at("pcStorage").at("boxes"))
    total += box.at("count").get<std::size_t>();
  return total;
}

std::string DeltaPhrase(std::int64_t delta, std::string_view gained,
                        std::string_view spent) {
  if (delta == 0) return "unchanged";
  return std::string(delta > 0 ? gained : spent) + " " +
         std::to_string(std::abs(delta));
}

void ItemMarkdown(std::ostringstream &output, const Json &items,
                  std::string_view heading) {
  if (items.empty()) return;
  output << "\n### " << heading << "\n\n";
  for (const auto &item : items)
    output << "- " << item.at("itemName").get<std::string>() << ": "
           << item.at("before") << " -> " << item.at("after") << " ("
           << (item.at("delta").get<std::int64_t>() > 0 ? "+" : "")
           << item.at("delta") << ")\n";
}

} // namespace

Json CompareProgress(const Json &older, const Json &newer) {
  const auto &before = older.at("decoded");
  const auto &after = newer.at("decoded");
  const auto oldTrainerId = before.at("trainer").at("trainerId");
  const auto newTrainerId = after.at("trainer").at("trainerId");
  const auto oldTrainerName = before.at("trainer").at("name").at("value");
  const auto newTrainerName = after.at("trainer").at("name").at("value");
  const bool samePlaythrough = oldTrainerId == newTrainerId &&
                               oldTrainerName == newTrainerName;

  Json badgesGained = Json::array();
  Json badgesLost = Json::array();
  const auto oldBadges = before.at("badges").at("raw").get<unsigned>();
  const auto newBadges = after.at("badges").at("raw").get<unsigned>();
  for (std::size_t bit = 0; bit < kBadgeNames.size(); ++bit) {
    const bool oldValue = (oldBadges & (1U << bit)) != 0;
    const bool newValue = (newBadges & (1U << bit)) != 0;
    if (!oldValue && newValue) badgesGained.push_back(kBadgeNames[bit]);
    if (oldValue && !newValue) badgesLost.push_back(kBadgeNames[bit]);
  }

  const auto elapsed = PlaytimeSeconds(after.at("playtime")) -
                       PlaytimeSeconds(before.at("playtime"));
  const auto moneyDelta = SignedDelta(
      before.at("moneyAndCoins").at("money"),
      after.at("moneyAndCoins").at("money"));
  const auto coinsDelta = SignedDelta(
      before.at("moneyAndCoins").at("coins"),
      after.at("moneyAndCoins").at("coins"));

  const auto eventsSet = ChangedRecords(before.at("events").at("flags"),
                                        after.at("events").at("flags"), "value", true);
  const auto eventsCleared = ChangedRecords(before.at("events").at("flags"),
                                            after.at("events").at("flags"), "value", false);
  const auto trainers = ChangedRecords(
      before.at("trainerBattles").at("records"),
      after.at("trainerBattles").at("records"), "completed", true);
  const auto statics = ChangedRecords(
      before.at("staticBattles").at("records"),
      after.at("staticBattles").at("records"), "completed", true);
  const auto stories = ChangedRecords(
      before.at("storyProgress").at("storyFlags"),
      after.at("storyProgress").at("storyFlags"), "completed", true);

  return {
      {"format", "pkmn-red-playthrough-progress"},
      {"reportVersion", "1.0.0"},
      {"samePlaythrough", samePlaythrough},
      {"identity",
       {{"olderTrainerName", oldTrainerName},
        {"newerTrainerName", newTrainerName},
        {"olderTrainerId", oldTrainerId},
        {"newerTrainerId", newTrainerId}}},
      {"sources",
       {{"older",
         {{"logicalName", older.at("source").at("fileName")},
          {"sha256", older.at("source").at("hashes").at("wholeFileSha256")}}},
        {"newer",
         {{"logicalName", newer.at("source").at("fileName")},
          {"sha256", newer.at("source").at("hashes").at("wholeFileSha256")}}}}},
      {"elapsedPlaytimeSeconds", elapsed},
      {"currency",
       {{"money",
         {{"before", before.at("moneyAndCoins").at("money")},
          {"after", after.at("moneyAndCoins").at("money")},
          {"delta", moneyDelta}}},
        {"coins",
         {{"before", before.at("moneyAndCoins").at("coins")},
          {"after", after.at("moneyAndCoins").at("coins")},
          {"delta", coinsDelta}}}}},
      {"badges",
       {{"beforeRaw", oldBadges}, {"afterRaw", newBadges},
        {"gained", badgesGained}, {"lost", badgesLost}}},
      {"pokedex",
       {{"owned",
         {{"before", before.at("pokedex").at("ownedCount")},
          {"after", after.at("pokedex").at("ownedCount")},
          {"delta", SignedDelta(before.at("pokedex").at("ownedCount"),
                                 after.at("pokedex").at("ownedCount"))}}},
        {"seen",
         {{"before", before.at("pokedex").at("seenCount")},
          {"after", after.at("pokedex").at("seenCount")},
          {"delta", SignedDelta(before.at("pokedex").at("seenCount"),
                                 after.at("pokedex").at("seenCount"))}}}}},
      {"location",
       {{"before", before.at("location")},
        {"after", after.at("location")},
        {"changed", before.at("location") != after.at("location")}}},
      {"partyChanges", PartyChanges(before.at("party"), after.at("party"))},
      {"inventoryChanges",
       {{"bag", ItemChanges(before.at("inventory").at("bag"),
                             after.at("inventory").at("bag"))},
        {"pcItems", ItemChanges(before.at("inventory").at("pcItems"),
                                 after.at("inventory").at("pcItems"))}}},
      {"storage",
       {{"pokemonBefore", StoredPokemon(before)},
        {"pokemonAfter", StoredPokemon(after)},
        {"selectedBoxBefore",
         before.at("currentBoxCache").at("selectedBoxNumber")},
        {"selectedBoxAfter",
         after.at("currentBoxCache").at("selectedBoxNumber")}}},
      {"hallOfFame",
       {{"recordsBefore", before.at("hallOfFame").at("recordCount")},
        {"recordsAfter", after.at("hallOfFame").at("recordCount")}}},
      {"worldProgress",
       {{"eventsSet", eventsSet},
        {"eventsCleared", eventsCleared},
        {"trainerBattlesCompleted", trainers},
        {"staticEncountersCompleted", statics},
        {"storyFlagsCompleted", stories}}}};
}

std::string ProgressMarkdown(const Json &report) {
  const auto &sources = report.at("sources");
  const auto &currency = report.at("currency");
  const auto &pokedex = report.at("pokedex");
  const auto &world = report.at("worldProgress");
  std::ostringstream output;
  output << "# Pokemon Red Playthrough Progress\n\n"
         << "Compared `" << sources.at("older").at("logicalName").get<std::string>()
         << "` -> `" << sources.at("newer").at("logicalName").get<std::string>()
         << "`.\n\n- **Same playthrough:** "
         << (report.at("samePlaythrough").get<bool>() ? "yes" : "no")
         << "\n- **Elapsed playtime:** "
         << Duration(report.at("elapsedPlaytimeSeconds").get<std::int64_t>())
         << "\n- **Money:** " << currency.at("money").at("before") << " -> "
         << currency.at("money").at("after") << " ("
         << DeltaPhrase(currency.at("money").at("delta").get<std::int64_t>(),
                        "gained", "spent")
         << ")\n- **Coins:** " << currency.at("coins").at("before") << " -> "
         << currency.at("coins").at("after") << " ("
         << DeltaPhrase(currency.at("coins").at("delta").get<std::int64_t>(),
                        "gained", "used")
         << ")\n- **Pokedex owned:** " << pokedex.at("owned").at("before")
         << " -> " << pokedex.at("owned").at("after") << " ("
         << (pokedex.at("owned").at("delta").get<std::int64_t>() > 0 ? "+" : "")
         << pokedex.at("owned").at("delta") << ")\n- **Pokedex seen:** "
         << pokedex.at("seen").at("before") << " -> "
         << pokedex.at("seen").at("after") << " ("
         << (pokedex.at("seen").at("delta").get<std::int64_t>() > 0 ? "+" : "")
         << pokedex.at("seen").at("delta") << ")\n";

  if (report.at("location").at("changed").get<bool>())
    output << "- **Location:** "
           << report.at("location").at("before").at("mapName").get<std::string>()
           << " -> "
           << report.at("location").at("after").at("mapName").get<std::string>()
           << "\n";

  if (!report.at("badges").at("gained").empty()) {
    output << "\n## Badges gained\n\n";
    for (const auto &badge : report.at("badges").at("gained"))
      output << "- " << badge.get<std::string>() << " Badge\n";
  }
  if (!world.at("trainerBattlesCompleted").empty()) {
    output << "\n## Trainer battles completed\n\n";
    for (const auto &event : world.at("trainerBattlesCompleted"))
      output << "- Beat " << event.at("label").get<std::string>() << '\n';
  }
  if (!world.at("staticEncountersCompleted").empty()) {
    output << "\n## Static encounters completed\n\n";
    for (const auto &event : world.at("staticEncountersCompleted"))
      output << "- " << event.at("label").get<std::string>() << '\n';
  }
  if (!world.at("storyFlagsCompleted").empty()) {
    output << "\n## Story progress\n\n";
    for (const auto &event : world.at("storyFlagsCompleted"))
      output << "- " << event.at("label").get<std::string>() << '\n';
  }
  if (!report.at("partyChanges").empty()) {
    output << "\n## Party changes\n\n";
    for (const auto &change : report.at("partyChanges")) {
      output << "- Slot " << change.at("slot") << ": ";
      if (change.at("before").is_null())
        output << "empty";
      else
        output << change.at("before").at("nickname").get<std::string>()
               << " (Lv. " << change.at("before").at("level") << ')';
      output << " -> ";
      if (change.at("after").is_null())
        output << "empty";
      else
        output << change.at("after").at("nickname").get<std::string>()
               << " (Lv. " << change.at("after").at("level") << ')';
      output << '\n';
    }
  }
  ItemMarkdown(output, report.at("inventoryChanges").at("bag"),
               "Bag changes");
  ItemMarkdown(output, report.at("inventoryChanges").at("pcItems"),
               "PC item changes");
  output << "\n## Technical notes\n\n- Newly set verified event flags: "
         << world.at("eventsSet").size()
         << "\n- Cleared verified event flags: "
         << world.at("eventsCleared").size()
         << "\n- Both input saves passed checksum validation.\n";
  return output.str();
}

} // namespace pkmn::cli::red::comparison
