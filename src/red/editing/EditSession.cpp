#include "red/editing/EditSession.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "red/save/RedSave.hpp"
#include "red/codec/Gen1Codec.hpp"
#include "red/events/EventCatalog.hpp"
#include "red/json/RedJsonDocument.hpp"
#include "red/validation/SaveValidator.hpp"
#include "util/Sha256.hpp"

namespace pkmn::cli::red::editing {
namespace {

void RequireSession(const Json &session) {
  if (session.value("format", "") != "pkmn-red-edit-session" ||
      session.value("sessionVersion", "") != "0.1.0" ||
      !session.contains("source") || !session.contains("document") ||
      !session.contains("pendingEdits") ||
      !session.at("pendingEdits").is_array())
    throw std::runtime_error("invalid or unsupported edit session");
}

void RequireEditablePointer(const std::string &pointer) {
  if (!pointer.starts_with("/decoded/"))
    throw std::runtime_error("edits must target a /decoded field");
  if (pointer.starts_with("/decoded/location"))
    throw std::runtime_error(
        "arbitrary location editing is disabled; generation uses the verified "
        "Red's-house preset");
  const std::set<std::string> roots = {
      "trainer",         "rival",      "moneyAndCoins", "badges",
      "options",         "pokedex",    "inventory",     "party",
      "pcStorage",       "currentBoxCache", "daycare",  "hallOfFame",
      "worldStateRaw",   "playtime", "events", "trainerBattles",
      "staticBattles",   "storyProgress"};
  const auto begin = std::string("/decoded/").size();
  const auto end = pointer.find('/', begin);
  const auto root = pointer.substr(begin, end - begin);
  if (!roots.contains(root))
    throw std::runtime_error("field is not in the supported Red edit surface");
}

} // namespace

Json Begin(const std::filesystem::path &source) {
  const auto save = save::RedSave::Read(source);
  const auto integrity = validation::SaveValidator::Validate(save);
  if (!integrity.Valid())
    throw std::runtime_error("source save failed checksum validation");
  auto document = json::Decode(save, source.filename().string(), integrity,
                               {.includePhysicalImage = false});
  return {{"format", "pkmn-red-edit-session"},
          {"sessionVersion", "0.1.0"},
          {"source",
           {{"path", std::filesystem::absolute(source).lexically_normal().string()},
            {"logicalName", source.filename().string()},
            {"sha256", util::Sha256Hex(save.BytesView())}}},
          {"document", std::move(document)},
          {"pendingEdits", Json::array()},
          {"policy",
           {{"sourceOverwriteAllowed", false},
            {"physicalImageGenerationAuthority", false},
            {"locationEditing", "verified-safe-presets-only"}}}};
}

Json Load(const std::filesystem::path &sessionPath) {
  std::ifstream input(sessionPath);
  if (!input)
    throw std::runtime_error("could not open edit session");
  Json session;
  input >> session;
  RequireSession(session);
  return session;
}

void Save(const std::filesystem::path &sessionPath, const Json &session,
          bool refuseCollision) {
  RequireSession(session);
  if (refuseCollision && std::filesystem::exists(sessionPath))
    throw std::runtime_error("refusing to overwrite existing edit session");
  std::ofstream output(sessionPath, std::ios::binary | std::ios::trunc);
  output << session.dump(2) << '\n';
  if (!output)
    throw std::runtime_error("could not write complete edit session");
}

void AddEdit(Json &session, const std::string &pointer, const Json &value) {
  RequireSession(session);
  RequireEditablePointer(pointer);
  const Json::json_pointer path(pointer);
  auto &document = session.at("document");
  if (!document.contains(path))
    throw std::runtime_error("edit pointer does not identify an existing field");
  const auto previous = document.at(path);
  document[path] = value;
  if ((pointer.ends_with("/name/value") ||
       pointer.ends_with("/nickname/value") ||
       pointer.ends_with("/otName/value"))) {
    const auto losslessPointer = Json::json_pointer(
        pointer.substr(0, pointer.size() - 5) + "losslessValue");
    if (document.contains(losslessPointer))
      document[losslessPointer] = value;
  }
  auto &decoded = document.at("decoded");
  auto syncCollection = [](Json &collection) {
    const auto count = collection.at("pokemon").size();
    collection["count"] = count;
    if (collection.contains("declaredCount"))
      collection["declaredCount"] = count;
  };
  syncCollection(decoded.at("party"));
  for (auto &box : decoded.at("pcStorage").at("boxes"))
    syncCollection(box);
  syncCollection(decoded.at("currentBoxCache").at("cache"));
  for (auto *key : {"bag", "pcItems"}) {
    auto &inventory = decoded.at("inventory").at(key);
    inventory["count"] = inventory.at("items").size();
    inventory["declaredCount"] = inventory.at("items").size();
  }
  decoded.at("hallOfFame")["recordCount"] =
      decoded.at("hallOfFame").at("entries").size();
  if (pointer == "/decoded/badges/raw") {
    const auto raw = value.get<std::uint8_t>();
    decoded.at("badges")["mirrorRaw"] = raw;
    auto &entries = decoded.at("badges").at("entries");
    for (std::size_t index = 0; index < entries.size(); ++index)
      entries.at(index)["owned"] = (raw & (1U << index)) != 0;
  }
  if (pointer == "/decoded/worldStateRaw" ||
      pointer == "/decoded/worldStateRaw/eventFlagsHex") {
    const auto bytes = codec::DecodeHex(
        decoded.at("worldStateRaw").at("eventFlagsHex").get<std::string>());
    const auto named = events::DecodeNamedState(bytes);
    for (const auto &[key, namedValue] : named.items())
      decoded[key] = namedValue;
  }
  session.at("pendingEdits").push_back(
      {{"path", pointer}, {"previous", previous}, {"value", value}});
}

void AddNamedEventEdit(Json &session, const std::string &name, bool value) {
  RequireSession(session);
  auto &decoded = session.at("document").at("decoded");
  if (!decoded.contains("events"))
    throw std::runtime_error("document has no named event catalog");
  int index = -1;
  bool previous = false;
  for (auto &record : decoded.at("events").at("flags")) {
    if (record.at("name").get<std::string>() == name) {
      index = record.at("flagIndex").get<int>();
      previous = record.at("value").get<bool>();
      record["value"] = value;
      break;
    }
  }
  if (index < 0)
    throw std::runtime_error("unknown verified Pokemon Red event name: " + name);
  for (const auto &[root, records, key] :
       std::vector<std::tuple<const char *, const char *, const char *>>{
           {"trainerBattles", "records", "completed"},
           {"staticBattles", "records", "completed"},
           {"storyProgress", "storyFlags", "completed"}})
    for (auto &record : decoded.at(root).at(records))
      if (record.at("flagIndex").get<int>() == index)
        record[key] = value;
  auto bytes = codec::DecodeHex(
      decoded.at("worldStateRaw").at("eventFlagsHex").get<std::string>());
  auto &byte = bytes.at(static_cast<std::size_t>(index / 8));
  const auto mask = static_cast<std::uint8_t>(1U << (index % 8));
  byte = value ? static_cast<std::uint8_t>(byte | mask)
               : static_cast<std::uint8_t>(byte & ~mask);
  decoded.at("worldStateRaw")["eventFlagsHex"] = codec::Hex(bytes);
  decoded.at("events")["set"] = std::count_if(
      decoded.at("events").at("flags").begin(),
      decoded.at("events").at("flags").end(),
      [](const Json &record) { return record.at("value").get<bool>(); });
  for (const auto &[root, records] :
       std::vector<std::pair<const char *, const char *>>{
           {"trainerBattles", "records"}, {"staticBattles", "records"},
           {"storyProgress", "storyFlags"}})
    decoded.at(root)["complete"] = std::count_if(
        decoded.at(root).at(records).begin(), decoded.at(root).at(records).end(),
        [](const Json &record) { return record.at("completed").get<bool>(); });
  session.at("pendingEdits").push_back(
      {{"path", "/decoded/events/by-name/" + name},
       {"previous", previous}, {"value", value}, {"flagIndex", index}});
}

generation::Result Validate(const Json &session) {
  RequireSession(session);
  const auto &source = session.at("source");
  const auto current = save::RedSave::Read(source.at("path").get<std::string>());
  if (util::Sha256Hex(current.BytesView()) !=
      source.at("sha256").get<std::string>())
    throw std::runtime_error(
        "source save changed after this edit session was created");
  const auto semanticValidation =
      json::ValidateDocument(session.at("document"));
  if (!semanticValidation.Valid()) {
    const auto message = semanticValidation.errors.empty()
                             ? "edited semantic document is invalid"
                             : semanticValidation.errors.front();
    throw std::runtime_error("edit validation failed: " + message);
  }
  auto result = generation::Generate(session.at("document"));
  const auto integrity = validation::SaveValidator::Validate(
      save::RedSave(result.bytes));
  if (!integrity.Valid())
    throw std::runtime_error("edited output failed checksum validation");
  return result;
}

std::filesystem::path DefaultSessionPath(const std::filesystem::path &source) {
  auto path = source;
  if (path.extension() == ".sav" || path.extension() == ".srm")
    path.replace_extension();
  path += ".edit-session.json";
  return path;
}

std::filesystem::path DefaultOutputPath(const Json &session) {
  RequireSession(session);
  const std::filesystem::path source =
      session.at("source").at("path").get<std::string>();
  std::string category = "manual-edit";
  const auto &edits = session.at("pendingEdits");
  if (edits.size() == 1) {
    const auto pointer = edits.at(0).at("path").get<std::string>();
    if (pointer.starts_with("/decoded/moneyAndCoins/money"))
      category = "money";
    else if (pointer.starts_with("/decoded/party"))
      category = "party";
    else if (pointer.starts_with("/decoded/pcStorage") ||
             pointer.starts_with("/decoded/currentBoxCache"))
      category = "box-edit";
    else if (pointer.starts_with("/decoded/inventory"))
      category = "inventory";
    else if (pointer.starts_with("/decoded/trainer") ||
             pointer.starts_with("/decoded/rival"))
      category = "trainer";
    else if (pointer.starts_with("/decoded/badges"))
      category = "badges";
    else if (pointer.starts_with("/decoded/daycare"))
      category = "daycare";
    else if (pointer.starts_with("/decoded/hallOfFame"))
      category = "hall-of-fame";
    else if (pointer.starts_with("/decoded/worldStateRaw"))
      category = "events";
  }
  return source.parent_path() /
         (source.stem().string() + "_generated_" + category + ".sav");
}

} // namespace pkmn::cli::red::editing
