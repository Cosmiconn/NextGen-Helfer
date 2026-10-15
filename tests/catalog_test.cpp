#include "catalog.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <limits>
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Katalogpfad als Argument fehlt");
        const auto records = isya::load_catalog(argv[1]);
        require(records.size() >= 8, "Katalog ist zu klein");
        bool found = false, found_recipe = false;
        for (const auto& r : records) {
            if (r.name == "Kupfererz" && r.source_place.find("Echo") != std::string::npos) found = true;
            if (r.name == "Elrue [1]" && r.category == "Rezept") found_recipe = true;
            require(r.source_url.rfind("https://",0) == 0, "Ungueltige Belegquelle");
            require(!r.quality.empty(), "Qualitaetsmarkierung fehlt");
        }
        require(found && found_recipe, "Zentrale Wiki-Eintraege fehlen");
        require(isya::contains_normalized(L"SilberERZ",L"ERZ"), "Ungueltiger Namensabgleich");
        require(!isya::contains_normalized(L"Topas",L"Silber"), "Falschpositiver Namensabgleich");
        require(isya::safe_profit(300,100,15) == 185, "Gewinnrechnung fehlerhaft");
        require(isya::safe_profit(100,125,0) == -25, "Verlustrechnung fehlerhaft");
        bool rejected_negative = false;
        try { (void)isya::safe_profit(0,-1,0); } catch (const std::invalid_argument&) { rejected_negative = true; }
        require(rejected_negative, "Negative Kosten nicht abgewiesen");
        bool rejected_overflow = false;
        try { (void)isya::safe_profit(0,std::numeric_limits<long long>::max(),1); } catch (const std::overflow_error&) { rejected_overflow = true; }
        require(rejected_overflow, "Kostenueberlauf nicht abgewiesen");
        std::cout << "Katalog und Preisregeln OK: " << records.size() << " Eintraege\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
