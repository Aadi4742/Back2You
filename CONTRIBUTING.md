# Contributing to Back2You

Welcome to **Back2You**! We are building an intelligent, open, and reliable campus lost-and-found management platform. We appreciate contributions from the student and developer community.

---

## Team & Project Leads
* **Sameer Chand** ([@binsamdu](https://github.com/binsamdu)) &mdash; Lead Systems & Data Architecture
* **Aditya Rai** ([@Aadi4742](https://github.com/Aadi4742)) &mdash; Backend Architect & Security Lead

---

## Code of Conduct
* Be respectful, constructive, and collaborative.
* Focus on code quality, security, and student data privacy.

---

## Development Workflow

### 1. Branching Convention
* `feature/<feature-name>` &mdash; for new backend routes, algorithms, or frontend pages.
* `fix/<bug-name>` &mdash; for bug fixes or security patches.
* `docs/<topic>` &mdash; for documentation improvements.

### 2. Commit Message Standards
We follow Conventional Commits formatting:
* `feat:` A new user-facing feature or API endpoint.
* `fix:` A bug fix.
* `docs:` Documentation changes only.
* `style:` Code style, formatting, CSS adjustments.
* `refactor:` Code change that neither fixes a bug nor adds a feature.
* `chore:` Build process, dependency updates, or tool configurations.

### 3. Local Development & Building
Ensure you have CMake 3.14+ and a C++17 compiler:
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 4. Pull Requests
* Open a PR against the `main` branch.
* Describe the problem solved or the feature added.
* Ensure code compiles cleanly with zero warnings.

---

Thank you for helping make campus lost and found better for everyone!
