#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace isya {
struct Target {
  std::string name;
  std::int64_t need = 0;
  std::optional<std::int64_t> max_unit;
  std::string source;
};
struct Sample {
  std::string name;
  std::optional<std::int64_t> price;
  std::int64_t quantity = 1;
  bool quantity_assumed = true;
  std::string tooltip_text, price_text, quantity_text, message;
};
struct Match {
  std::string status = "none";
  std::string name;
  std::int64_t need = 0;
  std::optional<std::int64_t> max_unit;
  std::optional<std::int64_t> unit;
  bool qty_assumed = true;
  std::string message;
};
std::string normalize(std::string_view text);
std::string parse_name(std::string_view raw);
std::optional<std::int64_t> parse_price(std::string_view raw);
std::optional<std::int64_t> parse_quantity(std::string_view raw);
Match match_sample(const Sample& sample, const std::vector<Target>& targets);
std::string signature(const Sample& sample);
}
