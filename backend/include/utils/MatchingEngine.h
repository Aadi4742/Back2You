#pragma once

#include <vector>
#include <string>
#include <utility>
#include "models/Item.h"

namespace back2you {

struct MatchResult {
    Item candidate_item;
    double score = 0.0; // 0.0 to 100.0%
    std::vector<std::string> match_reasons;

    nlohmann::json toJson() const {
        return {
            {"item", candidate_item.toJson()},
            {"score", std::round(score * 10.0) / 10.0},
            {"reasons", match_reasons}
        };
    }
};

class MatchingEngine {
public:
    // Find matching items for a target item (opposite type: lost -> found, found -> lost)
    static std::vector<MatchResult> findMatches(const Item& targetItem, double minConfidence = 30.0);

    // Compute similarity score between two items
    static MatchResult calculateSimilarity(const Item& a, const Item& b);

    // Trigger automatic match notifications for a newly created item
    static void notifyPotentialMatches(const Item& newItem);
};

} // namespace back2you
