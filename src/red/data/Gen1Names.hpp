#pragma once

#include <cstdint>
#include <string_view>

namespace pkmn::cli::red::data {

std::string_view SpeciesName(std::uint8_t id);
int PokedexNumber(std::uint8_t id);
bool IsValidSpecies(std::uint8_t id);
std::string_view MoveName(std::uint8_t id);
bool IsValidMove(std::uint8_t id);
std::string_view ItemName(std::uint8_t id);
bool IsValidItem(std::uint8_t id);
std::string_view MapName(std::uint8_t id);

} // namespace pkmn::cli::red::data
