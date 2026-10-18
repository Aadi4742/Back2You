#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace back2you {

enum class ClaimStatus {
    Pending,
    Approved,
    Rejected
};

inline std::string claimStatusToString(ClaimStatus status) {
    switch (status) {
        case ClaimStatus::Pending: return "PENDING";
        case ClaimStatus::Approved: return "APPROVED";
        case ClaimStatus::Rejected: return "REJECTED";
    }
    return "PENDING";
}

inline ClaimStatus stringToClaimStatus(const std::string& str) {
    if (str == "APPROVED") return ClaimStatus::Approved;
    if (str == "REJECTED") return ClaimStatus::Rejected;
    return ClaimStatus::Pending;
}

struct Claim {
    int id = 0;
    int item_id = 0;
    std::string item_title;
    int claimant_id = 0;
    std::string claimant_name;
    std::string proof_details;
    std::string contact_info;
    ClaimStatus status = ClaimStatus::Pending;
    std::string review_notes;
    std::string created_at;
    std::string updated_at;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"item_id", item_id},
            {"item_title", item_title},
            {"claimant_id", claimant_id},
            {"claimant_name", claimant_name},
            {"proof_details", proof_details},
            {"contact_info", contact_info},
            {"status", claimStatusToString(status)},
            {"review_notes", review_notes},
            {"created_at", created_at},
            {"updated_at", updated_at}
        };
    }
};

} // namespace back2you
