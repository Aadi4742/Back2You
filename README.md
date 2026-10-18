# Back2You &mdash; College Lost and Found Project

> **Course Project:** Database & Web Application Development  
> **Developed by:** Aditya Rai ([@Aadi4742](https://github.com/Aadi4742)) & Sameer Chand ([@binsamdu](https://github.com/binsamdu))  
> **Repository:** [https://github.com/Aadi4742/Back2You](https://github.com/Aadi4742/Back2You)  
> **Project Tagline:** *"Lost it? Found it? Back2You."*

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Database](https://img.shields.io/badge/Database-SQLite3-lightgrey.svg)](https://www.sqlite.org)

---

## Table of Contents
1. [Project Overview & Motivation](#why-back2you)
2. [Tech Stack Chosen](#tech-stack)
3. [Folder Structure](#repository-folder-structure)
4. [How to Build and Run](#build-and-run-instructions)
5. [API Overview](#api-documentation-summary)
6. [Team Division of Work](#team--github-workflow)
7. [Commit Plan & History](#git-commit-plan--authorship)
8. [Known Limitations & Next Steps](#next-stage-roadmap-stage-2-full-frontend)

---

## Why Back2You?

Every semester on university campuses, items like calculators, ID cards, headphones, and keys end up in security drawers or forgotten under classroom desks. The existing ways people try to get them back are pretty inefficient — posting across multiple unofficial WhatsApp groups, checking bulletin boards, or walking between different security desks hoping someone handed an item in.

We built **Back2You** as a centralized, fast lost-and-found system for campus. Instead of using a heavy framework with large dependencies, we wrote the backend in **C++17 with SQLite** and built lightweight, responsive HTML/CSS landing pages.

### Main Capabilities
* **Item Reporting:** Separate forms for reporting lost belongings or logging found items with their exact drop-off desk.
* **Keyword & Location Matching:** A matching engine scores overlapping keywords, categories, and campus building tags to suggest potential lost/found matches.
* **Ownership Verification:** To prevent someone from grabbing an item that isn't theirs, claimants must provide specific identifying details (like engravings, serial numbers, or stickers) that only the real owner would know.
* **Campus Desk Tracking:** Shows where found items are physically held (e.g., Library 1st Floor Desk, Central Security Room 102).
* **Role-Based Accounts:** Standard student accounts plus admin/staff accounts for reviewing claims and marking items as returned.

---

## Repository Folder Structure

```
College-Lost-and-found/
├── .gitignore                      # Git ignore rules for C++, CMake, SQLite, and IDEs
├── CMakeLists.txt                  # CMake build configuration with FetchContent
├── README.md                       # Main documentation, setup guide, and team credits
├── API_DOCS.md                     # Comprehensive REST API endpoint specification
│
├── backend/
│   ├── include/
│   │   ├── database/
│   │   │   └── Database.h          # SQLite manager header (schema, transactions, queries)
│   │   ├── models/
│   │   │   ├── User.h              # User model & JSON serializer
│   │   │   ├── Category.h          # Category model
│   │   │   ├── Item.h              # Lost & Found Item model
│   │   │   ├── Claim.h             # Ownership Claim model
│   │   │   └── Notification.h      # In-app Notification model
│   │   ├── controllers/
│   │   │   ├── AuthController.h    # Register, login, logout, profile
│   │   │   ├── ItemController.h    # Item CRUD, search, matching
│   │   │   ├── ClaimController.h   # Claim submission & review
│   │   │   ├── NotificationController.h # Notification retrieval & read status
│   │   │   └── AdminController.h   # Admin user & item moderation, system stats
│   │   ├── routes/
│   │   │   └── Router.h            # HTTP server routing & CORS middleware
│   │   └── utils/
│   │       ├── Auth.h              # Token extractor and session resolver
│   │       ├── Crypto.h            # SHA-256 and salt/token generator
│   │       ├── Validation.h        # Email, username, date, and input sanitizer
│   │       └── MatchingEngine.h    # Smart token-based matching algorithm
│   │
│   └── src/
│       ├── database/
│       │   └── Database.cpp        # SQLite migrations, prepared statements, CRUD
│       ├── controllers/
│       │   ├── AuthController.cpp
│       │   ├── ItemController.cpp
│       │   ├── ClaimController.cpp
│       │   ├── NotificationController.cpp
│       │   └── AdminController.cpp
│       ├── routes/
│       │   └── Router.cpp          # Endpoint bindings & CORS configuration
│       ├── utils/
│       │   ├── Auth.cpp
│       │   ├── Crypto.cpp
│       │   └── MatchingEngine.cpp
│       └── main.cpp                # Server entry point, database seeding, static file server
│
└── frontend/
    ├── css/
    │   └── styles.css              # Unified responsive design system & theme tokens
    ├── js/
    │   └── main.js                 # Vanilla JS API client helper
    ├── index.html                  # Home landing page (Hero, Search, How-it-Works, Recent Items, Stats)
    ├── about.html                  # About landing page (Mission, values, team bios)
    └── contact.html                # Contact landing page (Form, Campus drop-off directory)
```

---

## Build and Run Instructions

### Prerequisites
* A C++17 compatible compiler:
  * **Linux:** `g++` (version 7+) or `clang++`
  * **macOS:** Apple Clang / Xcode Command Line Tools
  * **Windows:** MSVC (Visual Studio 2019/2022) or MinGW-w64
* **CMake:** Version 3.14 or higher
* **Git:** Required by CMake `FetchContent` to download dependencies automatically

---

### Step 1: Clone the Repository
```bash
git clone https://github.com/Aadi4742/Back2You.git
cd College-Lost-and-found
```

---

### Step 2: Build with CMake

#### Linux / macOS:
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### Windows (Command Prompt / PowerShell with Visual Studio):
```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### Windows (with MinGW):
```powershell
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

---

### Step 3: Run the Server
From the root of the repository:
```bash
# Linux / macOS:
./build/bin/back2you_server

# Windows:
.\build\bin\Release\back2you_server.exe
# (or .\build\bin\back2you_server.exe depending on generator)
```

* The server will initialize SQLite database `back2you.db`.
* Automatic database seeding creates default categories, an admin user (`admin` / `Admin@123`), and sample lost/found items.
* Open your browser and navigate to:  
  **`http://localhost:8080/`** (serves the landing pages directly!)
* API Health Check:  
  **`http://localhost:8080/api/health`**

---

## API Documentation Summary

Exhaustive details for every endpoint, request body, headers, status codes, and JSON responses are provided in [`API_DOCS.md`](API_DOCS.md).

Quick summary of endpoints:
* **Auth:**
  * `POST /api/auth/register` &mdash; Create account
  * `POST /api/auth/login` &mdash; Authenticate & receive token
  * `POST /api/auth/logout` &mdash; Revoke session
  * `GET  /api/auth/me` &mdash; Current user profile
* **Categories & Items:**
  * `GET  /api/categories` &mdash; List item categories
  * `GET  /api/items` &mdash; Search and filter items
  * `POST /api/items/lost` &mdash; Report a lost item
  * `POST /api/items/found` &mdash; Report a found item
  * `GET  /api/items/:id` &mdash; View specific item details
  * `PUT  /api/items/:id` &mdash; Update item details
  * `DELETE /api/items/:id` &mdash; Remove item
  * `GET  /api/items/:id/matches` &mdash; Run smart AI matching engine
* **Claims:**
  * `POST /api/claims` &mdash; Submit an ownership claim
  * `GET  /api/claims/my` &mdash; View my claims
  * `GET  /api/claims/item/:id` &mdash; View all claims on an item
  * `POST /api/claims/:id/review` &mdash; Approve or reject a claim
* **Notifications:**
  * `GET  /api/notifications` &mdash; In-app alerts
  * `POST /api/notifications/:id/read` &mdash; Mark read
* **Admin:**
  * `GET  /api/admin/users`, `PUT /api/admin/users/:id/role`, `DELETE /api/admin/users/:id`
  * `GET  /api/admin/items`, `PUT /api/admin/items/:id/status`
  * `GET  /api/admin/claims`, `GET /api/admin/stats`

---

## Team & GitHub Workflow

This repository was collaboratively authored by two team members:

| Collaborator | Role | GitHub Profile | Commit Signature |
| :--- | :--- | :--- | :--- |
| **Sameer Chand** | Lead Systems & Data Architecture | [`@binsamdu`](https://github.com/binsamdu) | `Sameer Chand <259921961+binsamdu@users.noreply.github.com>` |
| **Aditya Rai** | Backend Architect & Security Lead | [`@Aadi4742`](https://github.com/Aadi4742) | `Aditya Rai <151811061+Aadi4742@users.noreply.github.com>` |

---

## Git Commit Plan & Authorship

The project was committed in small, logical, incremental milestones, split equally between both collaborators:

| # | Author | Commit Message | Scope / Deliverables |
| :-: | :--- | :--- | :--- |
| **1** | Sameer Chand | `chore: initial repository structure, .gitignore, and CMake configuration` | `.gitignore`, root `CMakeLists.txt` with FetchContent for httplib, json, and sqlite3 |
| **2** | Aditya Rai | `feat(db): implement SQLite database schema, migrations, and model definitions` | `User.h`, `Item.h`, `Claim.h`, `Category.h`, `Notification.h`, `Database.h`, `Database.cpp` |
| **3** | Sameer Chand | `feat(auth): implement user authentication, password hashing, and session management` | `Crypto.h/cpp` (SHA-256 + salt), `Validation.h`, `Auth.h/cpp`, `AuthController.h/cpp` |
| **4** | Aditya Rai | `feat(items): add lost and found item reporting, validation, and storage` | `ItemController.h/cpp` (Lost/Found creation, update, delete, categories) |
| **5** | Sameer Chand | `feat(search): add advanced filtering and smart matching engine for lost/found items` | `MatchingEngine.h/cpp` (weighted matching algorithm), `searchItems`, `getItemMatches` |
| **6** | Aditya Rai | `feat(claims): add claim submission, verification workflow, and item status tracking` | `ClaimController.h/cpp` (claim filing, review, approval/rejection lifecycle) |
| **7** | Sameer Chand | `feat(notifications): add notification system, admin management APIs, and HTTP router` | `NotificationController`, `AdminController`, `Router.h/cpp`, `main.cpp` |
| **8** | Aditya Rai | `docs: add comprehensive API_DOCS.md detailing all REST endpoints` | `API_DOCS.md` with complete documentation for all 24+ REST routes |
| **9** | Sameer Chand | `feat(frontend): create shared CSS design system, typography, and responsive layouts` | `frontend/css/styles.css` with blue & warm accent theme, cards, forms, and layout |
| **10** | Aditya Rai | `feat(frontend): build Home landing page with hero, search, features, and stats` | `frontend/index.html` with hero, search, how it works, recent item cards, stats |
| **11** | Sameer Chand | `feat(frontend): build About and Contact landing pages with team showcase` | `frontend/about.html`, `frontend/contact.html`, `frontend/js/main.js` |
| **12** | Aditya Rai | `docs: complete README.md with build guide, architecture, and team credits` | Complete project documentation, build guides, and Stage 2 roadmap |
| **13** | Sameer Chand | `chore: add MIT license and project governance` | Open-source MIT license, project contribution and governance guidelines (`LICENSE`, `CONTRIBUTING.md`) |
| **14** | Aditya Rai | `chore: add automated build scripts for Windows and Unix platforms` | Cross-platform build automation scripts for Windows (`build.bat`) and Linux/macOS (`build.sh`) |

---

## Next Stage Roadmap (Stage 2: Full Frontend)

Now that Stage 1 (complete working C++ REST backend + landing pages) is finished, the following tasks are planned for Stage 2:

1. **User Authentication UI Modals / Pages:**
   * Dynamic Sign In and Registration modal forms connecting to `/api/auth/login` and `/api/auth/register`.
   * Client-side token storage (`localStorage`) and auto-login session restore.
2. **Interactive "Report Lost" & "Report Found" Forms:**
   * Multi-step form wizard with image drag-and-drop preview.
   * Campus building dropdown with interactive building floor/room tagging.
3. **Interactive Claim Submission Modal:**
   * Dedicated claim verification questionnaire directly accessible from item cards.
4. **User Dashboard / Profile Page:**
   * "My Reported Items" (edit, mark as found/resolved).
   * "My Claims" with real-time status indicators (`Pending`, `Approved`, `Rejected`).
   * Live notifications bell with unread badge counter.
5. **Admin Moderation Portal:**
   * Web table to audit all reports, approve claims, and oversee user roles.
6. **Live Search & Filter Interactivity:**
   * Real-time debounced search bar querying `/api/items` with active category pills and instant card updates without page refresh.

---

### Credits
Designed and Developed by **Aditya and Sameer**

