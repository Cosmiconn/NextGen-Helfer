#pragma once
#include "logic.hpp"
#include <string>
#include <vector>

namespace isya {
struct OcrLabel { std::string text; int x=0, y=0, width=0, height=0; float confidence=0; };
struct WorldHit { std::string name; int x=0, y=0, width=0, height=0; float confidence=0; std::string category; };
// Exact normalized label matching; no fuzzy labels are highlighted to avoid false positives.
std::vector<WorldHit> find_visible_targets(const std::vector<OcrLabel>& labels, const std::vector<Target>& targets,
                                            int origin_x, int origin_y, float minimum_confidence = 60.f);
}
