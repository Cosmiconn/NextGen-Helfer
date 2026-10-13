#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace isya::shn {
struct Column {std::string name; std::uint32_t type=0; std::int32_t length=0;};
struct Table {
  std::uint32_t header=0, record_length=0;
  std::vector<Column> columns;
  std::vector<std::vector<std::string>> rows;
};
// Offline data-only SHN table decoder. Does not write to the game installation.
Table read(const std::filesystem::path& input);
std::string csv(const Table& table);
std::string profile(const Table& table);
}
