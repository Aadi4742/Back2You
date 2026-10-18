# Back2You REST API Documentation

Comprehensive specification of all RESTful API endpoints for the **Back2You** Lost and Found Campus Management Platform.

* **Base URL:** `http://localhost:8080/api`
* **Version:** `1.0.0`
* **Response Format:** `application/json`
* **Authors:** Sameer Chand & Aditya Rai

---

## Table of Contents
1. [Authentication & Headers](#authentication--headers)
2. [Status Codes & Errors](#status-codes--errors)
3. [Authentication Endpoints](#1-authentication-endpoints)
4. [Categories](#2-categories)
5. [Items (Lost & Found)](#3-items-lost--found)
6. [Smart Matching Engine](#4-smart-matching-engine)
7. [Claims System](#5-claims-system)
8. [In-App Notifications](#6-in-app-notifications)
9. [Admin Management & Moderation](#7-admin-management--moderation)
10. [Health & System Check](#8-health--system-check)

---

## Authentication & Headers

Protected endpoints require a Bearer token or custom auth header:
```http
Authorization: Bearer <your_session_token>
```
or
```http
X-Auth-Token: <your_session_token>
```

When authenticated, standard user permissions apply. Role `admin` is required for routes prefixed with `/api/admin`.

---

## Status Codes & Errors

| Status Code | Description |
| :--- | :--- |
| `200 OK` | Request succeeded. |
| `201 Created` | Resource created successfully. |
| `204 No Content` | Options preflight handled. |
| `400 Bad Request` | Invalid payload or missing required parameters. |
| `401 Unauthorized` | Missing, invalid, or expired session token. |
| `403 Forbidden` | Insufficient permissions (requires item owner or admin role). |
| `404 Not Found` | Requested resource ID not found. |
| `409 Conflict` | Unique constraint violation (e.g., username/email already taken). |
| `500 Server Error` | Database or internal processing error. |

---

## 1. Authentication Endpoints

### 1.1 Register User
* **Method:** `POST`
* **URL:** `/api/auth/register`
* **Auth:** Public
* **Request Body:**
```json
{
  "username": "alexrivera",
  "email": "alex.rivera@campus.edu",
  "password": "SecretPassword123",
  "full_name": "Alex Rivera",
  "phone": "+1-555-0199"
}
```
* **Response (201 Created):**
```json
{
  "success": true,
  "message": "Registration successful",
  "token": "4a7f0c19b8e24483a903c749964585cb79f1...",
  "user": {
    "id": 2,
    "username": "alexrivera",
    "email": "alex.rivera@campus.edu",
    "role": "user",
    "full_name": "Alex Rivera",
    "phone": "+1-555-0199",
    "created_at": "2026-10-18 05:40:00"
  }
}
```

---

### 1.2 Login User
* **Method:** `POST`
* **URL:** `/api/auth/login`
* **Auth:** Public
* **Request Body:**
```json
{
  "username": "alexrivera",
  "password": "SecretPassword123"
}
```
*(You can also pass `"email"` instead of `"username"`)*

* **Response (200 OK):**
```json
{
  "success": true,
  "message": "Login successful",
  "token": "4a7f0c19b8e24483a903c749964585cb79f1...",
  "user": {
    "id": 2,
    "username": "alexrivera",
    "email": "alex.rivera@campus.edu",
    "role": "user",
    "full_name": "Alex Rivera",
    "phone": "+1-555-0199",
    "created_at": "2026-10-18 05:40:00"
  }
}
```

---

### 1.3 Logout
* **Method:** `POST`
* **URL:** `/api/auth/logout`
* **Auth:** Required
* **Response (200 OK):**
```json
{
  "success": true,
  "message": "Logged out successfully"
}
```

---

### 1.4 Get Current User Profile
* **Method:** `GET`
* **URL:** `/api/auth/me`
* **Auth:** Required
* **Response (200 OK):**
```json
{
  "success": true,
  "user": {
    "id": 2,
    "username": "alexrivera",
    "email": "alex.rivera@campus.edu",
    "role": "user",
    "full_name": "Alex Rivera",
    "phone": "+1-555-0199",
    "created_at": "2026-10-18 05:40:00"
  }
}
```

---

## 2. Categories

### 2.1 List All Categories
* **Method:** `GET`
* **URL:** `/api/categories`
* **Auth:** Public
* **Response (200 OK):**
```json
{
  "success": true,
  "categories": [
    { "id": 1, "name": "Electronics", "description": "Phones, laptops, chargers, headphones", "icon": "laptop" },
    { "id": 2, "name": "Wallets & IDs", "description": "Wallets, purses, student IDs, credit cards", "icon": "credit-card" },
    { "id": 3, "name": "Keys", "description": "Dorm keys, car keys, bike lock keys", "icon": "key" },
    { "id": 4, "name": "Books & Stationery", "description": "Textbooks, notebooks, calculators", "icon": "book-open" }
  ]
}
```

---

## 3. Items (Lost & Found)

### 3.1 Search & Filter Items
* **Method:** `GET`
* **URL:** `/api/items`
* **Auth:** Public
* **Query Parameters:**
  * `q` or `keyword` *(optional)*: Search term across title, description, location.
  * `category_id` *(optional)*: Filter by category ID.
  * `location` *(optional)*: Filter by location keyword (e.g. `Library`).
  * `date` *(optional)*: Filter by exact event date `YYYY-MM-DD`.
  * `status` *(optional)*: `LOST`, `FOUND`, `CLAIM_PENDING`, `RETURNED`.
  * `type` *(optional)*: `LOST` or `FOUND`.
  * `limit` *(optional)*: Max results (default `50`, max `100`).
  * `offset` *(optional)*: Pagination offset (default `0`).
* **Response (200 OK):**
```json
{
  "success": true,
  "count": 1,
  "items": [
    {
      "id": 1,
      "user_id": 2,
      "user_name": "alexrivera",
      "type": "LOST",
      "title": "MacBook Air M2 (Space Gray)",
      "category_id": 1,
      "category_name": "Electronics",
      "description": "Left in Library 2nd floor reading room. Has a GitHub sticker on the lid.",
      "location": "Central Library - 2nd Floor",
      "date_event": "2026-10-15",
      "image_path": "https://images.unsplash.com/photo-1517336714731-489689fd1ca8?auto=format&fit=crop&w=800&q=80",
      "contact_details": "alex.rivera@campus.edu",
      "held_at": "",
      "status": "LOST",
      "created_at": "2026-10-18 05:40:00",
      "updated_at": "2026-10-18 05:40:00"
    }
  ]
}
```

---

### 3.2 Report Lost Item
* **Method:** `POST`
* **URL:** `/api/items/lost`
* **Auth:** Required
* **Request Body:**
```json
{
  "title": "AirPods Pro (2nd Gen)",
  "category_id": 1,
  "description": "White charging case with engraving 'AR'. Lost near the gym locker room.",
  "location": "Sports Complex Locker Room",
  "date_event": "2026-10-17",
  "image_path": "https://images.unsplash.com/photo-1600294037681-c80b4cb5b434?auto=format&fit=crop&w=800&q=80",
  "contact_details": "alex.rivera@campus.edu"
}
```
* **Response (201 Created):**
```json
{
  "success": true,
  "message": "Lost item reported successfully",
  "item": {
    "id": 5,
    "user_id": 2,
    "type": "LOST",
    "title": "AirPods Pro (2nd Gen)",
    "status": "LOST",
    "date_event": "2026-10-17",
    "created_at": "2026-10-18 05:45:10"
  }
}
```

---

### 3.3 Report Found Item
* **Method:** `POST`
* **URL:** `/api/items/found`
* **Auth:** Required
* **Request Body:**
```json
{
  "title": "Set of 3 Keys on Red Lanyard",
  "category_id": 3,
  "description": "Found on bench outside gym. Has bike lock key and dorm fob.",
  "location": "Sports Complex Bench",
  "date_event": "2026-10-17",
  "image_path": "https://images.unsplash.com/photo-1582139329536-e7284fece509?auto=format&fit=crop&w=800&q=80",
  "contact_details": "gym.staff@campus.edu",
  "held_at": "Gymnasium Reception Front Desk"
}
```
* **Response (201 Created):**
```json
{
  "success": true,
  "message": "Found item reported successfully",
  "item": {
    "id": 6,
    "user_id": 1,
    "type": "FOUND",
    "title": "Set of 3 Keys on Red Lanyard",
    "held_at": "Gymnasium Reception Front Desk",
    "status": "FOUND",
    "created_at": "2026-10-18 05:46:00"
  }
}
```

---

### 3.4 Get Item by ID
* **Method:** `GET`
* **URL:** `/api/items/:id`
* **Auth:** Public
* **Response (200 OK):**
```json
{
  "success": true,
  "item": {
    "id": 2,
    "title": "Brown Leather Wallet with Student ID",
    "type": "FOUND",
    "held_at": "Campus Security Central Desk (Room 102)",
    "status": "FOUND"
  }
}
```

---

### 3.5 Update Item
* **Method:** `PUT`
* **URL:** `/api/items/:id`
* **Auth:** Required (Item reporter or Admin)
* **Request Body:** Fields to update (`title`, `description`, `location`, `category_id`, `held_at`, `contact_details`, `image_path`).
* **Response (200 OK):**
```json
{
  "success": true,
  "message": "Item updated successfully",
  "item": { ... }
}
```

---

### 3.6 Delete Item
* **Method:** `DELETE`
* **URL:** `/api/items/:id`
* **Auth:** Required (Item reporter or Admin)
* **Response (200 OK):**
```json
{
  "success": true,
  "message": "Item deleted successfully"
}
```

---

## 4. Smart Matching Engine

### 4.1 Get Matches for Item
Automatically suggests potential matching items of the opposite type (Lost $\leftrightarrow$ Found) using weighted scoring across categories, keyword tokens, campus locations, and dates.
* **Method:** `GET`
* **URL:** `/api/items/:id/matches`
* **Auth:** Public
* **Response (200 OK):**
```json
{
  "success": true,
  "target_item": {
    "id": 1,
    "title": "MacBook Air M2 (Space Gray)",
    "type": "LOST"
  },
  "match_count": 1,
  "matches": [
    {
      "item": {
        "id": 7,
        "title": "Space Gray Apple Laptop",
        "type": "FOUND",
        "held_at": "Library Information Desk",
        "location": "Central Library"
      },
      "score": 85.5,
      "reasons": [
        "Exact category match (Electronics)",
        "Keyword overlap in title/description (e.g. 'gray')",
        "Close campus location proximity",
        "Reported within the same month"
      ]
    }
  ]
}
```

---

## 5. Claims System

### 5.1 Submit Claim on Found Item
* **Method:** `POST`
* **URL:** `/api/claims`
* **Auth:** Required
* **Request Body:**
```json
{
  "item_id": 2,
  "proof_details": "My name is John Doe and my student ID 2024-8891 is inside the card slot. There is also a $20 bill and a campus library card.",
  "contact_info": "john.doe@campus.edu | +1-555-0177"
}
```
* **Response (201 Created):**
```json
{
  "success": true,
  "message": "Claim submitted successfully. The finder/admin will review your proof of ownership.",
  "claim": {
    "id": 1,
    "item_id": 2,
    "claimant_id": 3,
    "status": "PENDING",
    "created_at": "2026-10-18 05:50:00"
  }
}
```

---

### 5.2 Get My Submitted Claims
* **Method:** `GET`
* **URL:** `/api/claims/my`
* **Auth:** Required
* **Response (200 OK):**
```json
{
  "success": true,
  "claims": [
    {
      "id": 1,
      "item_id": 2,
      "item_title": "Brown Leather Wallet with Student ID",
      "status": "PENDING",
      "proof_details": "Student ID 2024-8891 is inside...",
      "created_at": "2026-10-18 05:50:00"
    }
  ]
}
```

---

### 5.3 Get Claims for Specific Item
* **Method:** `GET`
* **URL:** `/api/claims/item/:id`
* **Auth:** Required (Item reporter or Admin)
* **Response (200 OK):** Returns list of all claims submitted on this item.

---

### 5.4 Review Claim (Approve or Reject)
* **Method:** `POST`
* **URL:** `/api/claims/:id/review`
* **Auth:** Required (Item reporter or Admin)
* **Request Body:**
```json
{
  "decision": "APPROVE",
  "review_notes": "Student verified ID card in person at Campus Security Central Desk."
}
```
*(Decision must be `"APPROVE"` or `"REJECT"`)*

* **Response (200 OK):**
```json
{
  "success": true,
  "message": "Claim has been APPROVED",
  "claim_id": 1,
  "decision": "APPROVE"
}
```
*Note: Approving a claim automatically transitions item status to `RETURNED` and notifies the claimant.*

---

## 6. In-App Notifications

### 6.1 Get Notifications for Logged-In User
* **Method:** `GET`
* **URL:** `/api/notifications?unread=false`
* **Auth:** Required
* **Query Parameters:** `unread` (`true` or `false`)
* **Response (200 OK):**
```json
{
  "success": true,
  "count": 2,
  "notifications": [
    {
      "id": 1,
      "user_id": 2,
      "title": "Potential Match Found!",
      "message": "We found a possible match (85% confidence) for your item 'MacBook Air M2'.",
      "type": "MATCH",
      "is_read": false,
      "item_id": 7,
      "created_at": "2026-10-18 05:46:10"
    }
  ]
}
```

---

### 6.2 Mark Notification as Read
* **Method:** `POST`
* **URL:** `/api/notifications/:id/read`
* **Auth:** Required
* **Response (200 OK):**
```json
{
  "success": true,
  "message": "Notification marked as read"
}
```

---

### 6.3 Mark All Notifications as Read
* **Method:** `POST`
* **URL:** `/api/notifications/read-all`
* **Auth:** Required
* **Response (200 OK):**
```json
{
  "success": true,
  "message": "All notifications marked as read"
}
```

---

## 7. Admin Management & Moderation

All endpoints below require role `admin`.

### 7.1 List All Users
* **Method:** `GET`
* **URL:** `/api/admin/users`
* **Response (200 OK):** Returns all registered users with role and timestamps.

### 7.2 Update User Role
* **Method:** `PUT`
* **URL:** `/api/admin/users/:id/role`
* **Request Body:** `{ "role": "admin" }` or `{ "role": "user" }`

### 7.3 Delete User
* **Method:** `DELETE`
* **URL:** `/api/admin/users/:id`

### 7.4 List All Items
* **Method:** `GET`
* **URL:** `/api/admin/items`

### 7.5 Override Item Status
* **Method:** `PUT`
* **URL:** `/api/admin/items/:id/status`
* **Request Body:** `{ "status": "RETURNED" }` *(Options: `LOST`, `FOUND`, `CLAIM_PENDING`, `RETURNED`)*

### 7.6 View All Claims
* **Method:** `GET`
* **URL:** `/api/admin/claims`

### 7.7 System Metrics & Statistics
* **Method:** `GET`
* **URL:** `/api/admin/stats`
* **Response (200 OK):**
```json
{
  "success": true,
  "stats": {
    "total_lost": 12,
    "total_found": 15,
    "total_returned": 9,
    "active_claims_pending": 2,
    "registered_users": 34,
    "recovery_rate_percent": 33.3
  }
}
```

---

## 8. Health & System Check

### 8.1 Health Endpoint
* **Method:** `GET`
* **URL:** `/api/health`
* **Response (200 OK):**
```json
{
  "status": "ok",
  "service": "Back2You REST API",
  "version": "1.0.0",
  "authors": ["Sameer Chand", "Aditya Rai"]
}
```
