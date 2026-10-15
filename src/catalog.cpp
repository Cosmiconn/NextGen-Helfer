#include "catalog.hpp"
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>
namespace isya {
std::vector<Entry> load_catalog(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Katalogdatei nicht lesbar");
    std::vector<Entry> items;
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> col;
        std::size_t start = 0;
        for (;;) {
            const auto pos = line.find('\t', start);
            if (pos == std::string::npos) { col.push_back(line.substr(start)); break; }
            col.push_back(line.substr(start, pos-start)); start = pos + 1;
        }
        if (col.size() != 6 || col[0].empty()) throw std::runtime_error("Ungueltige TSV-Zeile: 6 Spalten erwartet");
        if (col[0] == "Name" && col[1] == "Kategorie") continue;
        items.push_back({col[0], col[1], col[2], col[3], col[4], col[5]});
        if (items.size() > 100000) throw std::runtime_error("Katalog zu gross");
    }
    return items;
}
bool contains_normalized(std::wstring_view haystack, std::wstring_view needle) {
    auto fold = [](std::wstring_view s) { std::wstring r(s); for (auto& c : r) c = std::towlower(c); return r; };
    return fold(haystack).find(fold(needle)) != std::wstring::npos;
}
long long safe_profit(long long sale, long long ingredient, long long fees) {
    if (sale < 0 || ingredient < 0 || fees < 0) throw std::invalid_argument("Negative Geldwerte");
    if (ingredient > std::numeric_limits<long long>::max() - fees) throw std::overflow_error("Kostenueberlauf");
    const auto total = ingredient + fees;
    return sale - total;
}
}
