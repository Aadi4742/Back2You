// ClaimController.cpp - verification claims submission and approval logic
// team: Aditya Rai & Sameer Chand

#include "controllers/ClaimController.h"
#include "database/Database.h"
#include "utils/Auth.h"
#include "utils/Validation.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace back2you {

// Aditya: submit proof of ownership for a found item
void ClaimController::submitClaim(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized. Please log in to file a claim."}}.dump(), "application/json");
        return;
    }

    nlohmann::json reqJson;
    try {
        reqJson = nlohmann::json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid JSON payload"}}.dump(), "application/json");
        return;
    }

    int itemId = reqJson.value("item_id", 0);
    std::string proofDetails = reqJson.value("proof_details", "");
    std::string contactInfo = reqJson.value("contact_info", "");

    if (itemId <= 0 || proofDetails.empty()) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "item_id and proof_details (identifying marks, serials, description) are required"}
        }.dump(), "application/json");
        return;
    }

    auto& db = Database::getInstance();
    auto itemOpt = db.getItemById(itemId);
    if (!itemOpt.has_value()) {
        res.status = 404;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Item not found"}}.dump(), "application/json");
        return;
    }

    const auto& item = *itemOpt;
    if (item.status == ItemStatus::Returned) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "This item has already been claimed and returned."}}.dump(), "application/json");
        return;
    }

    if (item.user_id == user->id) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "You cannot claim an item that you reported yourself."}}.dump(), "application/json");
        return;
    }

    if (contactInfo.empty()) {
        contactInfo = user->email + (user->phone.empty() ? "" : " | " + user->phone);
    }

    Claim claim;
    claim.item_id = itemId;
    claim.claimant_id = user->id;
    claim.proof_details = Validation::sanitize(proofDetails);
    claim.contact_info = Validation::sanitize(contactInfo);
    claim.status = ClaimStatus::Pending;

    int claimId = db.createClaim(claim);
    if (claimId <= 0) {
        res.status = 500;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Failed to submit claim"}}.dump(), "application/json");
        return;
    }

    // Update item status to CLAIM_PENDING
    db.updateItemStatus(itemId, ItemStatus::ClaimPending);

    // Notify finder / item reporter
    Notification notif;
    notif.user_id = item.user_id;
    notif.title = "New Claim Submitted!";
    notif.message = "A user (" + user->username + ") has submitted an ownership claim for your found item '" + item.title + "'.";
    notif.type = "CLAIM";
    notif.item_id = itemId;
    db.createNotification(notif);

    auto createdClaim = db.getClaimById(claimId);

    res.status = 201;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Claim submitted successfully. The finder/admin will review your proof of ownership."},
        {"claim", createdClaim.has_value() ? createdClaim->toJson() : nlohmann::json{{"id", claimId}}}
    }.dump(), "application/json");
}

void ClaimController::getItemClaims(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized"}}.dump(), "application/json");
        return;
    }

    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Missing item ID"}}.dump(), "application/json");
        return;
    }

    int itemId = 0;
    try {
        itemId = std::stoi(req.matches[1]);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid item ID"}}.dump(), "application/json");
        return;
    }

    auto& db = Database::getInstance();
    auto itemOpt = db.getItemById(itemId);
    if (!itemOpt.has_value()) {
        res.status = 404;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Item not found"}}.dump(), "application/json");
        return;
    }

    // Only the item reporter or an admin can view all claims on this item
    if (itemOpt->user_id != user->id && user->role != UserRole::Admin) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Permission denied. Only item reporter or admin can review claims."}}.dump(), "application/json");
        return;
    }

    auto claims = db.getClaimsByItemId(itemId);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& cl : claims) {
        arr.push_back(cl.toJson());
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"item_id", itemId},
        {"claims", arr}
    }.dump(), "application/json");
}

void ClaimController::getMyClaims(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized"}}.dump(), "application/json");
        return;
    }

    auto claims = Database::getInstance().getClaimsByUserId(user->id);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& cl : claims) {
        arr.push_back(cl.toJson());
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"claims", arr}
    }.dump(), "application/json");
}

void ClaimController::reviewClaim(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized"}}.dump(), "application/json");
        return;
    }

    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Missing claim ID"}}.dump(), "application/json");
        return;
    }

    int claimId = 0;
    try {
        claimId = std::stoi(req.matches[1]);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid claim ID"}}.dump(), "application/json");
        return;
    }

    auto& db = Database::getInstance();
    auto claimOpt = db.getClaimById(claimId);
    if (!claimOpt.has_value()) {
        res.status = 404;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Claim not found"}}.dump(), "application/json");
        return;
    }

    auto claim = *claimOpt;
    auto itemOpt = db.getItemById(claim.item_id);
    if (!itemOpt.has_value()) {
        res.status = 404;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Associated item not found"}}.dump(), "application/json");
        return;
    }

    const auto& item = *itemOpt;
    // Authorized if user reported the item or is admin
    if (item.user_id != user->id && user->role != UserRole::Admin) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Permission denied. Only item reporter or admin can review this claim."}}.dump(), "application/json");
        return;
    }

    nlohmann::json reqJson;
    try {
        reqJson = nlohmann::json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid JSON payload"}}.dump(), "application/json");
        return;
    }

    std::string decision = reqJson.value("decision", ""); // "APPROVE" or "REJECT"
    std::string reviewNotes = reqJson.value("review_notes", "");

    if (decision != "APPROVE" && decision != "REJECT") {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Decision must be 'APPROVE' or 'REJECT'"}}.dump(), "application/json");
        return;
    }

    if (decision == "APPROVE") {
        db.updateClaimStatus(claimId, ClaimStatus::Approved, reviewNotes);
        // Mark item as RETURNED
        db.updateItemStatus(item.id, ItemStatus::Returned);

        // Notify claimant
        Notification notif;
        notif.user_id = claim.claimant_id;
        notif.title = "Claim Approved!";
        notif.message = "Your claim for '" + item.title + "' has been APPROVED! " +
                        (item.held_at.empty() ? "Please coordinate with the finder." : "You can collect it at: " + item.held_at) +
                        (reviewNotes.empty() ? "" : " Note: " + reviewNotes);
        notif.type = "CLAIM";
        notif.item_id = item.id;
        db.createNotification(notif);
    } else {
        db.updateClaimStatus(claimId, ClaimStatus::Rejected, reviewNotes);
        // If no other pending claims, set status back to FOUND
        auto itemClaims = db.getClaimsByItemId(item.id);
        bool hasOtherPending = false;
        for (const auto& c : itemClaims) {
            if (c.id != claimId && c.status == ClaimStatus::Pending) {
                hasOtherPending = true;
                break;
            }
        }
        if (!hasOtherPending) {
            db.updateItemStatus(item.id, ItemStatus::Found);
        }

        // Notify claimant
        Notification notif;
        notif.user_id = claim.claimant_id;
        notif.title = "Claim Update";
        notif.message = "Your claim for '" + item.title + "' was not approved." +
                        (reviewNotes.empty() ? "" : " Reason: " + reviewNotes);
        notif.type = "CLAIM";
        notif.item_id = item.id;
        db.createNotification(notif);
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Claim has been " + decision + "D"},
        {"claim_id", claimId},
        {"decision", decision}
    }.dump(), "application/json");
}

} // namespace back2you
