#include "red/comparison/Comparison.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <sstream>

#include "util/Sha256.hpp"

namespace pkmn::cli::red::comparison {
namespace {
using Json = json::OrderedJson;

std::string SaveRegion(std::size_t offset) {
  if (offset < 0x2000)
    return "bank0-hall-of-fame-and-runtime";
  if (offset < 0x4000)
    return "bank1-main-save";
  if (offset < 0x6000)
    return "bank2-boxes-1-6";
  return "bank3-boxes-7-12";
}

bool EndsWith(const std::string &value, const std::string &suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
             0;
}

bool GameplayMutable(const std::string &path) {
  for (const auto *root : {"/moneyAndCoins/", "/badges/", "/playtime/",
                           "/location/", "/options/", "/pokedex/",
                           "/inventory/", "/party/", "/pcStorage/",
                           "/currentBoxCache/", "/daycare/", "/hallOfFame/",
                           "/worldStateRaw/", "/summaryCounts/"})
    if (path.rfind(root, 0) == 0)
      return true;
  return false;
}

std::string Classification(const std::string &path,
                           const SemanticOptions &options) {
  if (EndsWith(path, "/rawHex") || EndsWith(path, "/rawRecordHex") ||
      EndsWith(path, "/rawBlockHex") || EndsWith(path, "/rawSlotHex") ||
      EndsWith(path, "/speciesListHex") ||
      path.rfind("/summaryCounts/", 0) == 0)
    return "derived_match";
  if (EndsWith(path, "/losslessValue"))
    return "normalized_match";
  if (EndsWith(path, "/mirrorRaw"))
    return "synchronized_mirror_match";
  if (path.rfind("/location/", 0) == 0)
    return options.postEmulator ? "expected_runtime_drift"
                                : "permitted_canonical_difference";
  if (path.rfind("/currentBoxCache/", 0) == 0)
    return options.postEmulator ? "expected_runtime_drift"
                                : "expected_cache_difference";
  if (options.postEmulator && GameplayMutable(path))
    return "expected_runtime_drift";
  return "unexpected_mismatch";
}

void Differences(const Json &a, const Json &b, const std::string &path,
                 const SemanticOptions &options, Json &rows,
                 std::size_t &exactLeaves) {
  if (a.type() != b.type()) {
    rows.push_back({{"path", path},
                    {"classification", Classification(path, options)},
                    {"left", a},
                    {"right", b}});
    return;
  }
  if (a.is_object()) {
    for (auto it = a.begin(); it != a.end(); ++it) {
      if (!b.contains(it.key()))
        rows.push_back({{"path", path + "/" + it.key()},
                        {"classification",
                         Classification(path + "/" + it.key(), options)},
                        {"left", it.value()},
                        {"right", nullptr}});
      else
        Differences(it.value(), b.at(it.key()), path + "/" + it.key(),
                    options, rows, exactLeaves);
    }
    for (auto it = b.begin(); it != b.end(); ++it)
      if (!a.contains(it.key()))
        rows.push_back({{"path", path + "/" + it.key()},
                        {"classification",
                         Classification(path + "/" + it.key(), options)},
                        {"left", nullptr},
                        {"right", it.value()}});
    return;
  }
  if (a.is_array()) {
    const auto common = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < common; ++i)
      Differences(a.at(i), b.at(i), path + "/" + std::to_string(i), options,
                  rows, exactLeaves);
    if (a.size() != b.size())
      rows.push_back({{"path", path},
                      {"classification", Classification(path, options)},
                      {"leftSize", a.size()},
                      {"rightSize", b.size()}});
    return;
  }
  if (a != b)
    rows.push_back({{"path", path},
                    {"classification", Classification(path, options)},
                    {"left", a},
                    {"right", b}});
  else
    ++exactLeaves;
}
} // namespace

json::OrderedJson ComparePhysical(const save::RedSave::Bytes &a,
                                  const save::RedSave::Bytes &b) {
  Json ranges = Json::array();
  Json equalRanges = Json::array();
  std::size_t differing = 0, first = std::numeric_limits<std::size_t>::max(),
              last = 0;
  bool inRange = false;
  bool inEqualRange = false;
  std::size_t rangeStart = 0;
  std::size_t equalStart = 0;
  const auto common = std::min(a.size(), b.size());
  for (std::size_t i = 0; i < common; ++i) {
    const bool different = a[i] != b[i];
    if (different) {
      ++differing;
      if (first == std::numeric_limits<std::size_t>::max())
        first = i;
      last = i;
    }
    if (different && !inRange) {
      inRange = true;
      rangeStart = i;
    }
    if (!different && inRange) {
      ranges.push_back({{"start", rangeStart},
                        {"endInclusive", i - 1},
                        {"region", SaveRegion(rangeStart)},
                        {"classification", "physical_difference"}});
      inRange = false;
    }
    if (!different && !inEqualRange) {
      inEqualRange = true;
      equalStart = i;
    }
    if (different && inEqualRange) {
      equalRanges.push_back(
          {{"start", equalStart}, {"endInclusive", i - 1}});
      inEqualRange = false;
    }
  }
  if (inRange)
    ranges.push_back({{"start", rangeStart},
                      {"endInclusive", common - 1},
                      {"region", SaveRegion(rangeStart)},
                      {"classification", "physical_difference"}});
  if (inEqualRange)
    equalRanges.push_back(
        {{"start", equalStart}, {"endInclusive", common - 1}});
  if (a.size() != b.size()) {
    if (first == std::numeric_limits<std::size_t>::max())
      first = common;
    last = std::max(a.size(), b.size()) - 1;
    differing += std::max(a.size(), b.size()) - common;
    ranges.push_back({{"start", common}, {"endInclusive", last},
                      {"region", SaveRegion(common)},
                      {"classification", "size_difference"}});
  }
  const auto total = std::max(a.size(), b.size());
  return {{"leftSize", a.size()},
          {"rightSize", b.size()},
          {"leftSha256", util::Sha256Hex(a)},
          {"rightSha256", util::Sha256Hex(b)},
          {"identical", differing == 0},
          {"equalByteCount", total - differing},
          {"differingByteCount", differing},
          {"equalPercent",
           total == 0 ? 100.0
                      : 100.0 * static_cast<double>(total - differing) / total},
          {"firstDifference", differing == 0 ? Json(nullptr) : Json(first)},
          {"lastDifference", differing == 0 ? Json(nullptr) : Json(last)},
          {"equalRanges", equalRanges},
          {"differingRanges", ranges}};
}

json::OrderedJson CompareSemantic(const json::OrderedJson &a,
                                  const json::OrderedJson &b,
                                  const SemanticOptions &options) {
  Json rows = Json::array();
  std::size_t exactLeaves = 0;
  Differences(a.at("decoded"), b.at("decoded"), "", options, rows,
              exactLeaves);
  std::size_t unexpected = 0, permitted = 0;
  std::map<std::string, std::size_t> counts = {
      {"exact_match", exactLeaves},
      {"normalized_match", 0},
      {"derived_match", 0},
      {"synchronized_mirror_match", 0},
      {"permitted_canonical_difference", 0},
      {"expected_runtime_drift", 0},
      {"expected_cache_difference", 0},
      {"unsupported_or_deferred", 0},
      {"unexpected_mismatch", 0}};
  for (const auto &row : rows) {
    const auto classification =
        row.at("classification").get<std::string>();
    ++counts[classification];
    (classification == "unexpected_mismatch" ? unexpected : permitted)++;
  }
  Json classificationCounts = Json::object();
  for (const auto &[name, count] : counts)
    classificationCounts[name] = count;
  return {{"equivalent", unexpected == 0},
          {"policy", options.postEmulator ? "post-emulator" : "generation"},
          {"exactLeafCount", exactLeaves},
          {"differenceCount", rows.size()},
          {"unexpectedMismatchCount", unexpected},
          {"permittedDifferenceCount", permitted},
          {"classificationCounts", classificationCounts},
          {"differences", rows}};
}

std::string PhysicalMarkdown(const json::OrderedJson &r) {
  std::ostringstream o;
  o << "# Physical Comparison\n\n- Identical: "
    << (r.at("identical").get<bool>() ? "yes" : "no")
    << "\n- Differing bytes: " << r.at("differingByteCount")
    << "\n- Equal percent: " << r.at("equalPercent") << "\n- Left SHA-256: `"
    << r.at("leftSha256").get<std::string>() << "`\n- Right SHA-256: `"
    << r.at("rightSha256").get<std::string>() << "`\n";
  if (!r.at("identical").get<bool>()) {
    o << "- First difference: " << r.at("firstDifference")
      << "\n- Last difference: " << r.at("lastDifference")
      << "\n\n## Differing ranges\n\n"
         "| Start | End (inclusive) | Region |\n"
         "|---:|---:|---|\n";
    for (const auto &range : r.at("differingRanges"))
      o << "| " << range.at("start") << " | " << range.at("endInclusive")
        << " | " << range.at("region").get<std::string>() << " |\n";
  }
  return o.str();
}
std::string SemanticMarkdown(const json::OrderedJson &r) {
  std::ostringstream o;
  o << "# Semantic Comparison\n\n- Equivalent under policy: "
    << (r.at("equivalent").get<bool>() ? "yes" : "no")
    << "\n- Unexpected mismatches: " << r.at("unexpectedMismatchCount")
    << "\n- Permitted differences: " << r.at("permittedDifferenceCount")
    << "\n- Exact semantic leaves: " << r.at("exactLeafCount")
    << "\n- Policy: " << r.at("policy").get<std::string>() << "\n";
  if (!r.at("differences").empty()) {
    o << "\n## Field differences\n\n"
         "| Path | Classification | Left | Right |\n"
         "|---|---|---|---|\n";
    for (const auto &difference : r.at("differences"))
      o << "| `" << difference.at("path").get<std::string>() << "` | "
        << difference.at("classification").get<std::string>() << " | `"
        << difference.value("left", Json(nullptr)).dump() << "` | `"
        << difference.value("right", Json(nullptr)).dump() << "` |\n";
  }
  return o.str();
}
} // namespace pkmn::cli::red::comparison
