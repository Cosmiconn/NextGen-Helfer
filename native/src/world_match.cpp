#include "world_match.hpp"
#include <algorithm>
namespace isya {
std::vector<WorldHit> find_visible_targets(const std::vector<OcrLabel>& labels, const std::vector<Target>& targets,
                                            int ox, int oy, float min_confidence) {
  std::vector<WorldHit> hits;
  for (const auto& label : labels) {
    if (label.confidence < min_confidence || label.width < 6 || label.height < 6 || label.width > 2400 || label.height > 250)
      continue;
    auto key=normalize(label.text);
    if (key.empty()) continue;
    for (const auto& target : targets) {
      auto candidate=normalize(target.name);
      if (candidate.size() < 3 || candidate != key) continue;
      WorldHit hit{target.name, ox+label.x, oy+label.y, label.width, label.height,label.confidence,target.source};
      if (hit.x < -32768 || hit.y < -32768) continue;
      const auto duplicated=std::any_of(hits.begin(),hits.end(),[&](const WorldHit& prev){return prev.name==hit.name && std::abs(prev.x-hit.x)<8 && std::abs(prev.y-hit.y)<8;});
      if(!duplicated) hits.push_back(std::move(hit));
      break;
    }
    if(hits.size()>=100)break;
  }
  return hits;
}
}
