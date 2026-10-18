#include "utils/MatchingEngine.h"
#include "database/Database.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <cmath>
#include <iostream>

namespace back2you {

namespace {

// Helper to clean and lowercase words
std::string toLower(const std::string& str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return out;
}

std::vector<std::string> extractTokens(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            current += std::tolower(static_cast<unsigned char>(c));
        } else if (!current.empty()) {
            if (current.length() >= 3) {
                // Ignore small stop words
                if (current != "the" && current != "and" && current != "for" &&
                    current != "with" && current != "this" && current != "that") {
                    tokens.push_back(current);
                }
            }
            current.clear();
        }
    }
    if (current.length() >= 3) {
        tokens.push_back(current);
    }
    return tokens;
}

} // anonymous namespace

MatchResult MatchingEngine::calculateSimilarity(const Item& a, const Item& b) {
    MatchResult result;
    result.candidate_item = b;

    double score = 0.0;
    std::vector<std::string> reasons;

    // same category gives highest baseline confidence (35 pts)
    if (a.category_id == b.category_id && a.category_id > 0) {
        score += 35.0;
        reasons.push_back("Exact category match (" + a.category_name + ")");
    }

    // compare token overlaps between titles and notes
    auto tokensA = extractTokens(a.title + " " + a.description);
    auto tokensB = extractTokens(b.title + " " + b.description);

    std::unordered_set<std::string> setB(tokensB.begin(), tokensB.end());
    int sharedCount = 0;
    std::vector<std::string> sharedWords;

    for (const auto& token : tokensA) {
        if (setB.count(token)) {
            sharedCount++;
            if (sharedWords.size() < 3) {
                sharedWords.push_back(token);
            }
        }
    }

    if (!tokensA.empty() && !tokensB.empty()) {
        double maxTokens = std::max(tokensA.size(), tokensB.size());
        double overlapRatio = static_cast<double>(sharedCount) / maxTokens;
        double textScore = std::min(35.0, overlapRatio * 70.0 + (sharedCount * 5.0));
        if (textScore > 0) {
            score += textScore;
            std::string reason = "Keyword overlap in title/description";
            if (!sharedWords.empty()) {
                reason += " (e.g. '" + sharedWords[0] + "')";
            }
            reasons.push_back(reason);
        }
    }

    // check if reported locations match or reference the same building zone
    std::string locA = toLower(a.location);
    std::string locB = toLower(b.location);
    if (!locA.empty() && !locB.empty()) {
        if (locA == locB) {
            score += 20.0;
            reasons.push_back("Identical location reported: " + a.location);
        } else if (locA.find(locB) != std::string::npos || locB.find(locA) != std::string::npos) {
            score += 15.0;
            reasons.push_back("Close campus location proximity");
        } else {
            auto locTokensA = extractTokens(locA);
            auto locTokensB = extractTokens(locB);
            std::unordered_set<std::string> locSetB(locTokensB.begin(), locTokensB.end());
            for (const auto& lt : locTokensA) {
                if (locSetB.count(lt)) {
                    score += 10.0;
                    reasons.push_back("Common building/zone: " + lt);
                    break;
                }
            }
        }
    }

    // small boost if the event date aligns closely
    if (!a.date_event.empty() && !b.date_event.empty()) {
        if (a.date_event == b.date_event) {
            score += 10.0;
            reasons.push_back("Same event date (" + a.date_event + ")");
        } else {
            // Check if year-month matches
            if (a.date_event.substr(0, 7) == b.date_event.substr(0, 7)) {
                score += 5.0;
                reasons.push_back("Reported within the same month");
            }
        }
    }

    result.score = std::min(100.0, score);
    result.match_reasons = reasons;
    return result;
}

std::vector<MatchResult> MatchingEngine::findMatches(const Item& targetItem, double minConfidence) {
    std::vector<MatchResult> matches;
    auto& db = Database::getInstance();

    // Look for opposite type that is not returned yet
    std::string oppositeType = (targetItem.type == ItemType::Lost) ? "FOUND" : "LOST";
    auto candidates = db.getItems("", 0, "", "", "", oppositeType, 100, 0);

    for (const auto& candidate : candidates) {
        if (candidate.id == targetItem.id) continue;
        if (candidate.status == ItemStatus::Returned) continue;

        auto result = calculateSimilarity(targetItem, candidate);
        if (result.score >= minConfidence) {
            matches.push_back(result);
        }
    }

    // Sort descending by match score
    std::sort(matches.begin(), matches.end(), [](const MatchResult& x, const MatchResult& y) {
        return x.score > y.score;
    });

    return matches;
}

void MatchingEngine::notifyPotentialMatches(const Item& newItem) {
    auto matches = findMatches(newItem, 45.0); // 45%+ confidence
    auto& db = Database::getInstance();

    for (const auto& match : matches) {
        // Notify the reporter of the new item
        Notification notifForNew;
        notifForNew.user_id = newItem.user_id;
        notifForNew.title = "Potential Match Found!";
        notifForNew.message = "We found a possible match (" + std::to_string(static_cast<int>(match.score)) +
                              "% confidence) for your item '" + newItem.title + "': '" + match.candidate_item.title + "'.";
        notifForNew.type = "MATCH";
        notifForNew.item_id = match.candidate_item.id;
        db.createNotification(notifForNew);

        // Also notify the owner/reporter of the candidate item
        Notification notifForCandidate;
        notifForCandidate.user_id = match.candidate_item.user_id;
        notifForCandidate.title = "Potential Match Reported!";
        notifForCandidate.message = "A new " + itemTypeToString(newItem.type) + " item '" + newItem.title +
                                    "' matches your report for '" + match.candidate_item.title + "' (" +
                                    std::to_string(static_cast<int>(match.score)) + "% confidence).";
        notifForCandidate.type = "MATCH";
        notifForCandidate.item_id = newItem.id;
        db.createNotification(notifForCandidate);
    }
}

} // namespace back2you
