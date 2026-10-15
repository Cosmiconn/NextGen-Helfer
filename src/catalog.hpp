#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
namespace isya {
struct Entry {
    std::string name;
    std::string category;
    std::string source_place;
    std::string purpose;
    std::string source_url;
    std::string quality;
};
std::vector<Entry> load_catalog(const std::filesystem::path& path);
bool contains_normalized(std::wstring_view haystack, std::wstring_view needle);
long long safe_profit(long long sale_value, long long ingredient_value, long long fees);
}
