#include "red/comparison/Comparison.hpp"

#include <algorithm>
#include <limits>
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

void Differences(const Json &a, const Json &b, const std::string &path,
                 Json &rows) {
  if (a.type() != b.type()) {
    rows.push_back({{"path", path},
                    {"classification", "unexpected_mismatch"},
                    {"left", a},
                    {"right", b}});
    return;
  }
  if (a.is_object()) {
    for (auto it = a.begin(); it != a.end(); ++it) {
      if (it.key() == "rawHex" || it.key() == "rawRecordHex" ||
          it.key() == "rawBlockHex" || it.key() == "speciesListHex")
        continue;
      if (!b.contains(it.key()))
        rows.push_back({{"path", path + "/" + it.key()},
                        {"classification", "unexpected_mismatch"},
                        {"left", it.value()},
                        {"right", nullptr}});
      else
        Differences(it.value(), b.at(it.key()), path + "/" + it.key(), rows);
    }
    for (auto it = b.begin(); it != b.end(); ++it)
      if (it.key() != "rawHex" && it.key() != "rawRecordHex" &&
          it.key() != "rawBlockHex" && it.key() != "speciesListHex" &&
          !a.contains(it.key()))
        rows.push_back({{"path", path + "/" + it.key()},
                        {"classification", "unexpected_mismatch"},
                        {"left", nullptr},
                        {"right", it.value()}});
    return;
  }
  if (a.is_array()) {
    const auto common = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < common; ++i)
      Differences(a.at(i), b.at(i), path + "/" + std::to_string(i), rows);
    if (a.size() != b.size())
      rows.push_back({{"path", path},
                      {"classification", "unexpected_mismatch"},
                      {"leftSize", a.size()},
                      {"rightSize", b.size()}});
    return;
  }
  if (a != b) {
    const bool location = path.rfind("/location/", 0) == 0;
    rows.push_back({{"path", path},
                    {"classification",
                     location ? "permitted_safe_location_canonicalization"
                              : "unexpected_mismatch"},
                    {"left", a},
                    {"right", b}});
  }
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
    ranges.push_back({{"start", rangeStart}, {"endInclusive", common - 1},
                      {"region", SaveRegion(rangeStart)}, {"classification", "physical_difference"}});
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
                                  const json::OrderedJson &b) {
  Json rows = Json::array();
  Differences(a.at("decoded"), b.at("decoded"), "", rows);
  std::size_t unexpected = 0, permitted = 0;
  for (const auto &row : rows)
    (row.at("classification") == "unexpected_mismatch" ? unexpected
                                                       : permitted)++;
  return {{"equivalent", unexpected == 0},
          {"differenceCount", rows.size()},
          {"unexpectedMismatchCount", unexpected},
          {"permittedDifferenceCount", permitted},
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
    << "\n";
  return o.str();
}
} // namespace pkmn::cli::red::comparison
